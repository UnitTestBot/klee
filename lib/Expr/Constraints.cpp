//===-- Constraints.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Constraints.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Expr/IndependentConstraintSetUnion.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/Path.h"
#include "klee/Expr/SourceBuilder.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Symcrete.h"
#include "klee/Module/KModule.h"
#include "klee/Support/CompilerWarning.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

using namespace klee;

namespace {
llvm::cl::opt<RewriteEqualitiesPolicy> RewriteEqualities(
    "rewrite-equalities",
    llvm::cl::desc("Rewrite existing constraints when an equality with a "
                   "constant is added (default=simple)"),
    llvm::cl::values(clEnumValN(RewriteEqualitiesPolicy::None, "none",
                                "Don't rewrite"),
                     clEnumValN(RewriteEqualitiesPolicy::Simple, "simple",
                                "Use lightweight visitor"),
                     clEnumValN(RewriteEqualitiesPolicy::Full, "full",
                                "Use more powerful visitor")),
    llvm::cl::init(RewriteEqualitiesPolicy::Simple), llvm::cl::cat(SolvingCat));

llvm::cl::opt<bool> UseIntermittentRewriter(
    "use-intermittent-equalities-rewriter",
    llvm::cl::desc(
        "Rewrite existing constraints every few additions (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(SolvingCat));
} // namespace

class ExprReplaceVisitor : public ExprVisitor {
private:
  ref<Expr> src, dst;

public:
  ExprReplaceVisitor(const ref<Expr> &_src, const ref<Expr> &_dst)
      : src(_src), dst(_dst) {}

  Action visitExpr(const Expr &e) override {
    if (e == *src) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }

  Action visitExprPost(const Expr &e) override {
    if (e == *src) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }
};

class ExprReplaceVisitor2 : public ExprVisitor {
private:
  std::vector<std::reference_wrapper<const ExprHashMap<ref<Expr>>>>
      replacements;
  const ExprHashMap<ref<Expr>> &replacementParents;

public:
  explicit ExprReplaceVisitor2(const ExprHashMap<ref<Expr>> &_replacements,
                               const ExprHashMap<ref<Expr>> &_parents)
      : ExprVisitor(true), replacements({_replacements}),
        replacementParents(_parents) {}

  Action visitExpr(const Expr &e) override {
    for (auto i = replacements.rbegin(); i != replacements.rend(); i++) {
      if (i->get().count(ref<Expr>(const_cast<Expr *>(&e)))) {
        auto replacement = i->get().at(ref<Expr>(const_cast<Expr *>(&e)));
        if (replacementParents.count(const_cast<Expr *>(&e))) {
          replacementDependency.insert(
              replacementParents.at(const_cast<Expr *>(&e)));
        }
        return Action::changeTo(replacement);
      }
    }
    return Action::doChildren();
  }

  Action visitExprPost(const Expr &e) override {
    for (auto i = replacements.rbegin(); i != replacements.rend(); i++) {
      if (i->get().count(ref<Expr>(const_cast<Expr *>(&e)))) {
        auto replacement = i->get().at(ref<Expr>(const_cast<Expr *>(&e)));
        if (replacementParents.count(const_cast<Expr *>(&e))) {
          replacementDependency.insert(
              replacementParents.at(const_cast<Expr *>(&e)));
        }
        return Action::changeTo(replacement);
      }
    }
    return Action::doChildren();
  }

  Action visitSelect(const SelectExpr &sexpr) override {
    auto cond = visit(sexpr.cond);
    if (auto CE = dyn_cast<ConstantExpr>(cond)) {
      return CE->isTrue() ? Action::changeTo(visit(sexpr.trueExpr))
                          : Action::changeTo(visit(sexpr.falseExpr));
    }

    auto trueExpr = visit(sexpr.trueExpr);

    auto falseExpr = visit(sexpr.falseExpr);

    if (trueExpr != sexpr.trueExpr || falseExpr != sexpr.falseExpr) {
      ref<Expr> seres = SelectExpr::create(cond, trueExpr, falseExpr);

      auto res = visitExprPost(*seres.get());
      if (res.kind == Action::ChangeTo) {
        seres = res.argument;
      }
      return Action::changeTo(seres);
    } else {
      return Action::skipChildren();
    }
  }

  ref<SymbolicSource> visitSource(ref<SymbolicSource> source) {
    if (ref<LazyInitializationSource> liSource =
            dyn_cast<LazyInitializationSource>(source)) {
      ref<Expr> pointer = visit(liSource->pointer);
      switch (source->getKind()) {
      case SymbolicSource::Kind::LazyInitializationAddress: {
        return SourceBuilder::lazyInitializationAddress(pointer);
      }
      case SymbolicSource::Kind::LazyInitializationSize: {
        return SourceBuilder::lazyInitializationSize(pointer);
      }
      case SymbolicSource::Kind::LazyInitializationContent: {
        return SourceBuilder::lazyInitializationContent(pointer);
      }
      default:
        assert(0 && "unreachable");
        unreachable();
      }
    } else {
      return source;
    }
  }

  const Array *visitArray(const Array *arr) {
    ref<SymbolicSource> source = visitSource(arr->source);
    ref<Expr> size = visit(arr->getSize());
    if (source != arr->source || size != arr->getSize()) {
      return Array::create(size, source, arr->getDomain(), arr->getRange());
    } else {
      return arr;
    }
  }

  UpdateList visitUpdateList(UpdateList u) {
    const Array *root = visitArray(u.root);
    std::vector<ref<UpdateNode>> updates;

    for (auto un = u.head; un; un = un->next) {
      updates.push_back(un);
    }

    updates.push_back(nullptr);

    for (int i = updates.size() - 2; i >= 0; i--) {
      ref<Expr> index = visit(updates[i]->index);
      ref<Expr> value = visit(updates[i]->value);
      updates[i] = new UpdateNode(updates[i + 1], index, value);
    }
    return UpdateList(root, updates[0]);
  }

  Action visitRead(const ReadExpr &) override { return Action::doChildren(); }

public:
  ExprHashSet replacementDependency;
};

class ExprReplaceVisitor3 : public ExprVisitor {
private:
  std::vector<std::reference_wrapper<const ExprHashMap<ref<Expr>>>>
      replacements;
  const ExprHashMap<ref<Expr>> &replacementParents;

public:
  explicit ExprReplaceVisitor3(const ExprHashMap<ref<Expr>> &_replacements,
                               const ExprHashMap<ref<Expr>> &_parents)
      : ExprVisitor(true), replacements({_replacements}),
        replacementParents(_parents) {}

  Action visitExpr(const Expr &e) override {
    for (auto i = replacements.rbegin(); i != replacements.rend(); i++) {
      if (i->get().count(ref<Expr>(const_cast<Expr *>(&e)))) {
        auto replacement = i->get().at(ref<Expr>(const_cast<Expr *>(&e)));
        if (replacementParents.count(const_cast<Expr *>(&e))) {
          replacementDependency.insert(
              replacementParents.at(const_cast<Expr *>(&e)));
        }
        return Action::changeTo(replacement);
      }
    }
    return Action::doChildren();
  }

  Action visitExprPost(const Expr &e) override {
    for (auto i = replacements.rbegin(); i != replacements.rend(); i++) {
      if (i->get().count(ref<Expr>(const_cast<Expr *>(&e)))) {
        auto replacement = i->get().at(ref<Expr>(const_cast<Expr *>(&e)));
        if (replacementParents.count(const_cast<Expr *>(&e))) {
          replacementDependency.insert(
              replacementParents.at(const_cast<Expr *>(&e)));
        }
        return Action::changeTo(replacement);
      }
    }
    return Action::doChildren();
  }

  ExprHashSet replacementDependency;
};

ConstraintSet::ConstraintSet(constraints_ty cs, symcretes_ty symcretes,
                             Assignment concretization)
    : cowKey(1), _constraints(cs), _symcretes(symcretes),
      _concretization(new Assignment(concretization)),
      _independentElements(new IndependentConstraintSetUnion(
          _constraints, _symcretes, *_concretization)),
      copyOnWriteOwner(cowKey) {}

ConstraintSet::ConstraintSet(ref<const IndependentConstraintSet> ics)
    : cowKey(1), _constraints(ics->getConstraints()),
      _symcretes(ics->getSymcretes()),
      _concretization(new Assignment(ics->concretization)),
      _independentElements(new IndependentConstraintSetUnion(ics)),
      copyOnWriteOwner(cowKey) {}

ConstraintSet::ConstraintSet(
    const std::vector<ref<const IndependentConstraintSet>> &factors,
    const ExprHashMap<ref<Expr>> &concretizedExprs)
    : cowKey(1), _concretization(new Assignment()),
      _independentElements(new IndependentConstraintSetUnion(
          _constraints, _symcretes, *_concretization)),
      copyOnWriteOwner(cowKey) {
  for (auto ics : factors) {
    constraints_ty constraints = ics->getConstraints();
    SymcreteOrderedSet symcretes = ics->getSymcretes();
    IndependentConstraintSetUnion icsu(ics);
    _constraints.insert(constraints.begin(), constraints.end());
    _symcretes.insert(symcretes.begin(), symcretes.end());
    _concretization->addIndependentAssignment(ics->concretization);
    _independentElements->addIndependentConstraintSetUnion(icsu);
  }
  _independentElements->concretizedExprs = concretizedExprs;
}

ConstraintSet::ConstraintSet(constraints_ty cs) : ConstraintSet(cs, {}, {}) {}

ConstraintSet::ConstraintSet()
    : cowKey(1), _concretization(new Assignment()),
      _independentElements(new IndependentConstraintSetUnion()),
      copyOnWriteOwner(cowKey) {}

void ConstraintSet::checkCopyOnWriteOwner() {
  if (cowKey != copyOnWriteOwner) {
    _independentElements = std::make_shared<IndependentConstraintSetUnion>(
        IndependentConstraintSetUnion(*_independentElements));
    _concretization =
        std::make_shared<Assignment>(Assignment(*_concretization));
  }
}

void ConstraintSet::addConstraint(ref<Expr> e) {
  checkCopyOnWriteOwner();
  _constraints.insert(e);
  _independentElements->addExpr(e);
}

IDType Symcrete::idCounter = 0;

void ConstraintSet::addSymcrete(ref<Symcrete> s) {
  checkCopyOnWriteOwner();
  _symcretes.insert(s);
  _independentElements->addSymcrete(s);
}

bool ConstraintSet::isSymcretized(ref<Expr> expr) const {
  for (auto symcrete : _symcretes) {
    if (symcrete->symcretized == expr) {
      return true;
    }
  }
  return false;
}

void ConstraintSet::rewriteConcretization(const Assignment &a) const {
  for (auto i : a.bindings) {
    if (concretization().bindings.count(i.first)) {
      _concretization->bindings.replace({i.first, i.second});
    }
  }
  _independentElements->updateConcretization(a);
}

ConstraintSet ConstraintSet::getConcretizedVersion() const {
  ConstraintSet cs;
  cs._independentElements = std::make_shared<IndependentConstraintSetUnion>(
      _independentElements->getConcretizedVersion());

  for (auto &e : cs._independentElements->is()) {
    if (isa<ExprOrSymcrete::left>(e)) {
      cs._constraints.insert(cast<ExprOrSymcrete::left>(e)->value());
    }
  }
  return cs;
}

ConstraintSet ConstraintSet::getConcretizedVersion(
    const Assignment &newConcretization) const {
  ConstraintSet cs;
  cs._independentElements = std::make_shared<IndependentConstraintSetUnion>(
      _independentElements->getConcretizedVersion(newConcretization));
  for (auto &e : cs._independentElements->is()) {
    cs._constraints.insert(cast<ExprOrSymcrete::left>(e)->value());
  }
  return cs;
}

void ConstraintSet::print(llvm::raw_ostream &os) const {
  os << "Constraints [\n";
  for (const auto &constraint : _constraints) {
    constraint->print(os);
    os << "\n";
  }

  os << "]\n";
  os << "Symcretes [\n";
  for (const auto &symcrete : _symcretes) {
    symcrete->symcretized->print(os);
    os << "\n";
  }
  os << "]\n";
}

void ConstraintSet::dump() const { this->print(llvm::errs()); }

void ConstraintSet::changeCS(constraints_ty &cs) {
  _constraints = cs;
  _independentElements = std::make_shared<IndependentConstraintSetUnion>(
      IndependentConstraintSetUnion(_constraints, _symcretes,
                                    *_concretization));
}

const constraints_ty &ConstraintSet::cs() const { return _constraints; }

const symcretes_ty &ConstraintSet::symcretes() const { return _symcretes; }

const IndependentConstraintSetUnion &
ConstraintSet::independentElements() const {
  return *_independentElements;
}

const Path &PathConstraints::path() const { return _path; }

const Assignment &ConstraintSet::concretization() const {
  return *_concretization;
}

const ConstraintSet &PathConstraints::cs() const { return constraints; }

void PathConstraints::advancePath(KInstruction *ki) { _path.advance(ki); }

ExprHashSet PathConstraints::addConstraint(ref<Expr> e) {
  auto expr = Simplificator::simplifyExpr(constraints, e);
  if (auto ce [[maybe_unused]] = dyn_cast<ConstantExpr>(expr.simplified)) {
    assert(ce->isTrue() && "Attempt to add invalid constraint");
    return {};
  }
  ExprHashSet added;
  std::vector<ref<Expr>> exprs;
  Expr::splitAnds(expr.simplified, exprs);
  for (auto expr : exprs) {
    if (auto ce [[maybe_unused]] = dyn_cast<ConstantExpr>(expr)) {
      assert(ce->isTrue() && "Expression simplified to false");
    } else {
      added.insert(expr);
      constraints.addConstraint(expr);
    }
  }
  addingCounter += 1;

  if (RewriteEqualities != RewriteEqualitiesPolicy::None &&
      (!UseIntermittentRewriter || (addingCounter & 0x3FFU) == 0)) {
    auto simplified =
        Simplificator::simplify(constraints.cs(), RewriteEqualities);
    if (simplified.wasSimplified) {
      constraints.changeCS(simplified.simplified);
    }
  }

  return added;
}

bool PathConstraints::isSymcretized(ref<Expr> expr) const {
  return constraints.isSymcretized(expr);
}

void PathConstraints::addSymcrete(ref<Symcrete> s) {
  constraints.addSymcrete(s);
}

void PathConstraints::rewriteConcretization(const Assignment &a) {
  constraints.rewriteConcretization(a);
}

Simplificator::ExprResult
Simplificator::simplifyExpr(const constraints_ty &constraints,
                            const ref<Expr> &expr) {
  if (isa<ConstantExpr>(expr))
    return {expr, {}};

  ExprHashMap<ref<Expr>> equalities;
  ExprHashMap<ref<Expr>> equalitiesParents;

  for (auto &constraint : constraints) {
    if (const EqExpr *ee = dyn_cast<EqExpr>(constraint)) {
      ref<Expr> small = ee->left;
      ref<Expr> big = ee->right;
      if (!isa<ConstantExpr>(small)) {
        auto hr = big->height(), hl = small->height();
        if (hr < hl || (hr == hl && big < small))
          std::swap(small, big);
        equalities.emplace(constraint, Expr::createTrue());
        equalitiesParents.emplace(constraint, constraint);
      }
      equalities.emplace(big, small);
      equalitiesParents.emplace(big, constraint);
    } else {
      equalities.emplace(constraint, Expr::createTrue());
      equalitiesParents.emplace(constraint, constraint);
      if (const NotExpr *ne = dyn_cast<NotExpr>(constraint)) {
        equalities.emplace(ne->expr, Expr::createFalse());
        equalitiesParents.emplace(ne->expr, constraint);
      }
    }
  }

  ExprReplaceVisitor2 visitor(equalities, equalitiesParents);
  auto visited = visitor.visit(expr);
  return {visited, visitor.replacementDependency};
}

Simplificator::ExprResult
Simplificator::simplifyExpr(const ConstraintSet &constraints,
                            const ref<Expr> &expr) {
  return simplifyExpr(constraints.cs(), expr);
}

Simplificator::SetResult
Simplificator::simplify(const constraints_ty &constraints,
                        RewriteEqualitiesPolicy policy) {
  // Initialization
  constraints_ty simplified;
  ExprHashMap<ExprHashSet> dependencies;
  for (auto constraint : constraints) {
    simplified.insert(constraint);
    dependencies.insert({constraint, {constraint}});
  }

  bool actuallyChanged = false;
  bool changed = true;
  while (changed) {
    changed = false;
    Replacements replacements = gatherReplacements(simplified);
    constraints_ty currentSimplified;
    ExprHashMap<ExprHashSet> currentDependencies;

    for (auto &constraint : simplified) {
      removeReplacement(replacements, constraint);
      ref<Expr> simplifiedConstraint;
      ExprHashSet dependency;
      if (policy == RewriteEqualitiesPolicy::Simple) {
        auto visitor = ExprReplaceVisitor3(replacements.equalities,
                                           replacements.equalitiesParents);
        simplifiedConstraint = visitor.visit(constraint);
        dependency = visitor.replacementDependency;
      } else {
        assert(policy != RewriteEqualitiesPolicy::None);
        auto visitor = ExprReplaceVisitor2(replacements.equalities,
                                           replacements.equalitiesParents);
        simplifiedConstraint = visitor.visit(constraint);
        dependency = visitor.replacementDependency;
      }
      addReplacement(replacements, constraint);
      std::vector<ref<Expr>> andsSplit;
      Expr::splitAnds(simplifiedConstraint, andsSplit);
      for (auto part : andsSplit) {
        currentSimplified.insert(part);
        currentDependencies.insert({part, dependency});
        currentDependencies[part].insert(constraint);
      }
      if (constraint != simplifiedConstraint || andsSplit.size() > 1) {
        actuallyChanged = true;
        changed = true;
      }
    }

    if (changed) {
      simplified = currentSimplified;
      dependencies = composeExprDependencies(dependencies, currentDependencies);
    }
  }

  simplified.erase(ConstantExpr::createTrue());
  dependencies.erase(ConstantExpr::createTrue());

  return {simplified, dependencies, actuallyChanged};
}

Simplificator::Replacements
Simplificator::gatherReplacements(constraints_ty constraints) {
  Replacements result;
  for (auto &constraint : constraints) {
    if (const EqExpr *ee = dyn_cast<EqExpr>(constraint)) {
      if (isa<ConstantExpr>(ee->left)) {
        result.equalities.insert(std::make_pair(ee->right, ee->left));
        result.equalitiesParents.insert({ee->right, constraint});
      } else {
        result.equalities.insert(
            std::make_pair(constraint, Expr::createTrue()));
        result.equalitiesParents.insert({constraint, constraint});
      }
    } else {
      result.equalities.insert(std::make_pair(constraint, Expr::createTrue()));
      result.equalitiesParents.insert({constraint, constraint});
    }
  }
  return result;
}

void Simplificator::addReplacement(Replacements &replacements, ref<Expr> expr) {
  if (const EqExpr *ee = dyn_cast<EqExpr>(expr)) {
    if (isa<ConstantExpr>(ee->left)) {
      replacements.equalities.insert(std::make_pair(ee->right, ee->left));
      replacements.equalitiesParents.insert({ee->right, expr});
    } else {
      replacements.equalities.insert(std::make_pair(expr, Expr::createTrue()));
      replacements.equalitiesParents.insert({expr, expr});
    }
  } else {
    replacements.equalities.insert(std::make_pair(expr, Expr::createTrue()));
    replacements.equalitiesParents.insert({expr, expr});
  }
}

void Simplificator::removeReplacement(Replacements &replacements,
                                      ref<Expr> expr) {
  if (const EqExpr *ee = dyn_cast<EqExpr>(expr)) {
    if (isa<ConstantExpr>(ee->left)) {
      replacements.equalities.erase(ee->right);
      replacements.equalitiesParents.erase(ee->right);
    } else {
      replacements.equalities.erase(expr);
      replacements.equalitiesParents.erase(expr);
    }
  } else {
    replacements.equalities.erase(expr);
    replacements.equalitiesParents.erase(expr);
  }
}

ExprHashMap<ExprHashSet>
Simplificator::composeExprDependencies(const ExprHashMap<ExprHashSet> &upper,
                                       const ExprHashMap<ExprHashSet> &lower) {
  ExprHashMap<ExprHashSet> result;
  for (const auto &dependent : lower) {
    for (const auto &dependency : dependent.second) {
      for (const auto &upperDependency : upper.at(dependency)) {
        result[dependent.first].insert(upperDependency);
      }
    }
  }
  return result;
}

void ConstraintSet::getAllIndependentConstraintsSets(
    ref<Expr> queryExpr,
    std::vector<ref<const IndependentConstraintSet>> &result) const {
  _independentElements->getAllIndependentConstraintSets(queryExpr, result);
}

void ConstraintSet::getAllDependentConstraintsSets(
    ref<Expr> queryExpr,
    std::vector<ref<const IndependentConstraintSet>> &result) const {
  _independentElements->getAllDependentConstraintSets(queryExpr, result);
}

std::vector<const Array *> ConstraintSet::gatherArrays() const {
  std::vector<const Array *> arrays;
  findObjects(_constraints.begin(), _constraints.end(), arrays);
  return arrays;
}

std::vector<const Array *> ConstraintSet::gatherSymcretizedArrays() const {
  std::unordered_set<const Array *> arrays;
  for (const ref<Symcrete> &symcrete : _symcretes) {
    arrays.insert(symcrete->dependentArrays().begin(),
                  symcrete->dependentArrays().end());
  }
  return std::vector<const Array *>(arrays.begin(), arrays.end());
}
