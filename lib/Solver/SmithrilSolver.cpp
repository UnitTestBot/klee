
//===-- SmithrilSolver.cpp ---------------------------------------*-C++-*-====//
//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/config.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Statistics/TimerStatIncrementer.h"

#ifdef ENABLE_SMITHRIL

#include "SmithrilBuilder.h"
#include "SmithrilSolver.h"

#include "klee/ADT/Incremental.h"
#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Support/OptionCategories.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/ErrorHandling.h"

namespace smithril {
#include <smithril.h>
}

#include <optional>
#include <string>

namespace {
llvm::cl::opt<unsigned> SmithrilVerbosityLevel(
    "debug-smithril-verbosity", llvm::cl::init(0),
    llvm::cl::desc("smithril verbosity level (default=0)"),
    llvm::cl::cat(klee::SolvingCat));
}

namespace klee {

using ConstraintFrames = inc_vector<ref<Expr>>;
using ExprIncMap = inc_umap<smithril::SmithrilTerm, ref<Expr>, SmithrilTermHash,
                            SmithrilTermCmp>;
using SmithrilASTIncMap =
    inc_umap<smithril::SmithrilTerm, smithril::SmithrilTerm, SmithrilTermHash,
             SmithrilTermCmp>;
using ExprIncSet =
    inc_uset<ref<Expr>, klee::util::ExprHash, klee::util::ExprCmp>;
using SmithrilASTIncSet =
    inc_uset<smithril::SmithrilTerm, SmithrilTermHash, SmithrilTermCmp>;

extern void dump(const ConstraintFrames &);

class ConstraintQuery {
private:
  // this should be used when only query is needed, se comment below
  ref<Expr> expr;

public:
  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Smithril works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  // so this `constraints` field contains: Constraints(X) ∧ ¬ query(X)
  ConstraintFrames constraints;

  explicit ConstraintQuery() {}

  explicit ConstraintQuery(const ConstraintFrames &frames, const ref<Expr> &e)
      : expr(e), constraints(frames) {}
  explicit ConstraintQuery(ConstraintFrames &&frames, ref<Expr> &&e)
      : expr(std::move(e)), constraints(std::move(frames)) {}

  explicit ConstraintQuery(const Query &q, bool incremental) : expr(q.expr) {
    if (incremental) {
      for (auto &constraint : q.constraints.cs()) {
        constraints.v.push_back(constraint);
        constraints.push();
      }
    } else {
      const auto &other = q.constraints.cs();
      constraints.v.reserve(other.size());
      constraints.v.insert(constraints.v.end(), other.begin(), other.end());
    }
    if (q.expr->getWidth() == Expr::Bool && !q.expr->isFalse())
      constraints.v.push_back(NotExpr::create(q.expr));
  }

  size_t size() const { return constraints.v.size(); }

  ref<Expr> getOriginalQueryExpr() const { return expr; }

  ConstraintQuery withFalse() const {
    return ConstraintQuery(ConstraintFrames(constraints), Expr::createFalse());
  }

  std::vector<const Array *> gatherArrays() const {
    std::vector<const Array *> arrays;
    findObjects(constraints.v.begin(), constraints.v.end(), arrays);
    return arrays;
  }
};

enum class ObjectAssignment {
  NotNeeded,
  NeededForObjectsFromEnv,
  NeededForObjectsFromQuery
};

struct SmithrilSolverEnv {
  using arr_vec = std::vector<const Array *>;
  inc_vector<const Array *> objects;
  arr_vec objectsForGetModel;
  inc_vector<smithril::SmithrilTerm> smithril_ast_expr_constraints;
  ExprIncMap smithril_ast_expr_to_klee_expr;
  SmithrilASTIncSet expr_to_track;
  inc_umap<const Array *, ExprHashSet> usedArrayBytes;
  ExprIncSet symbolicObjects;

  explicit SmithrilSolverEnv() = default;
  explicit SmithrilSolverEnv(const arr_vec &objects);

  void pop(size_t popSize);
  void push();
  void clear();

  const arr_vec *getObjectsForGetModel(ObjectAssignment oa) const;
};

SmithrilSolverEnv::SmithrilSolverEnv(const arr_vec &objects)
    : objectsForGetModel(objects) {}

void SmithrilSolverEnv::pop(size_t popSize) {
  if (popSize == 0)
    return;
  objects.pop(popSize);
  objectsForGetModel.clear();
  smithril_ast_expr_constraints.pop(popSize);
  smithril_ast_expr_to_klee_expr.pop(popSize);
  expr_to_track.pop(popSize);
  usedArrayBytes.pop(popSize);
  symbolicObjects.pop(popSize);
}

void SmithrilSolverEnv::push() {
  objects.push();
  smithril_ast_expr_constraints.push();
  smithril_ast_expr_to_klee_expr.push();
  expr_to_track.push();
  usedArrayBytes.push();
  symbolicObjects.push();
}

void SmithrilSolverEnv::clear() {
  objects.clear();
  objectsForGetModel.clear();
  smithril_ast_expr_constraints.clear();
  smithril_ast_expr_to_klee_expr.clear();
  expr_to_track.clear();
  usedArrayBytes.clear();
  symbolicObjects.clear();
}

const SmithrilSolverEnv::arr_vec *
SmithrilSolverEnv::getObjectsForGetModel(ObjectAssignment oa) const {
  switch (oa) {
  case ObjectAssignment::NotNeeded:
    return nullptr;
  case ObjectAssignment::NeededForObjectsFromEnv:
    return &objectsForGetModel;
  case ObjectAssignment::NeededForObjectsFromQuery:
    return &objects.v;
  default:
    llvm_unreachable("unknown object assignment");
  }
}

class SmithrilSolverImpl : public SolverImpl {
protected:
  std::unique_ptr<SmithrilBuilder> builder;
  smithril::SmithrilOptions solverParameters;

private:
  time::Span timeout;
  SolverImpl::SolverRunStatus runStatusCode;

  bool internalRunSolver(const ConstraintQuery &query, SmithrilSolverEnv &env,
                         ObjectAssignment needObjects,
                         std::vector<SparseStorageImpl<unsigned char>> *values,
                         ValidityCore *validityCore, bool &hasSolution);

  SolverImpl::SolverRunStatus handleSolverResponse(
      smithril::SmithrilSolver theSolver, smithril::SolverResult satisfiable,
      const SmithrilSolverEnv &env, ObjectAssignment needObjects,
      std::vector<SparseStorageImpl<unsigned char>> *values, bool &hasSolution);

protected:
  SmithrilSolverImpl();
  ~SmithrilSolverImpl();

  virtual smithril::SmithrilSolver
  initNativeSmithril(const ConstraintQuery &query,
                     SmithrilASTIncSet &assertions) = 0;
  virtual void deinitNativeSmithril(smithril::SmithrilSolver theSolver) = 0;
  virtual void push(smithril::SmithrilSolver s) = 0;

  bool computeTruth(const ConstraintQuery &, SmithrilSolverEnv &env,
                    bool &isValid);
  bool computeValue(const ConstraintQuery &, SmithrilSolverEnv &env,
                    ref<Expr> &result);
  bool
  computeInitialValues(const ConstraintQuery &, SmithrilSolverEnv &env,
                       std::vector<SparseStorageImpl<unsigned char>> &values,
                       bool &hasSolution);
  bool check(const ConstraintQuery &query, SmithrilSolverEnv &env,
             ref<SolverResponse> &result);
  bool computeValidityCore(const ConstraintQuery &query, SmithrilSolverEnv &env,
                           ValidityCore &validityCore, bool &isValid);

public:
  std::string getConstraintLog(const Query &) final;
  SolverImpl::SolverRunStatus getOperationStatusCode() final;
  void setCoreSolverTimeout(time::Span _timeout) final {
    timeout = _timeout;
    auto timeoutInSeconds = static_cast<unsigned>((timeout.toSeconds()));
    if (timeoutInSeconds) {
      std::string timeoutInSecondsString =
          std::to_string(timeoutInSeconds) + "s";
      smithril::smithril_set_timeout(solverParameters,
                                     timeoutInSecondsString.c_str());
    }
  }
  void enableUnsatCore() {
    smithril::smithril_set_produce_unsat_core(solverParameters, true);
  }
  void disableUnsatCore() {
    smithril::smithril_set_produce_unsat_core(solverParameters, false);
  }

  // pass virtual functions to children
  using SolverImpl::check;
  using SolverImpl::computeInitialValues;
  using SolverImpl::computeTruth;
  using SolverImpl::computeValidityCore;
  using SolverImpl::computeValue;
};

void deleteNativeSmithril(smithril::SmithrilSolver theSolver) {
  smithril::smithril_delete_solver(theSolver);
}

SmithrilSolverImpl::SmithrilSolverImpl()
    : runStatusCode(SolverImpl::SOLVER_RUN_STATUS_FAILURE) {
  builder = std::unique_ptr<SmithrilBuilder>(new SmithrilBuilder(
      /*autoClearConstructCache=*/false));
  assert(builder && "unable to create SmithrilBuilder");
  solverParameters = smithril::smithril_new_options();

  setCoreSolverTimeout(timeout);

  if (ProduceUnsatCore) {
    enableUnsatCore();
  } else {
    disableUnsatCore();
  }

  // Set verbosity
  // if (SmithrilVerbosityLevel > 0) {
  //   solverParameters.set(Option::VERBOSITY,
  //                        SmithrilVerbosityLevel); // mixa117 no such thing?
  // }

  // if (SmithrilVerbosityLevel) {
  //   solverParameters.set(Option::DBG_CHECK_MODEL,
  //                        true); // mixa117 no such thing?
  // }
}

SmithrilSolverImpl::~SmithrilSolverImpl() {
  smithril::smithril_delete_options(solverParameters);
}

std::string SmithrilSolver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

std::string SmithrilSolverImpl::getConstraintLog(const Query &) { return ""; }

bool SmithrilSolverImpl::computeTruth(const ConstraintQuery &query,
                                      SmithrilSolverEnv &env, bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status = internalRunSolver(query, /*env=*/env,
                                  ObjectAssignment::NotNeeded, /*values=*/NULL,
                                  /*validityCore=*/NULL, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool SmithrilSolverImpl::computeValue(const ConstraintQuery &query,
                                      SmithrilSolverEnv &env,
                                      ref<Expr> &result) {
  std::vector<SparseStorageImpl<unsigned char>> values;
  bool hasSolution;

  // Find the object used in the expression, and compute an assignment
  // for them.
  findSymbolicObjects(query.getOriginalQueryExpr(), env.objectsForGetModel);
  if (!computeInitialValues(query.withFalse(), env, values, hasSolution))
    return false;
  assert(hasSolution && "state has invalid constraint set");

  // Evaluate the expression with the computed assignment.
  Assignment a(env.objectsForGetModel, values);
  result = a.evaluate(query.getOriginalQueryExpr());

  return true;
}

bool SmithrilSolverImpl::computeInitialValues(
    const ConstraintQuery &query, SmithrilSolverEnv &env,
    std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution) {
  return internalRunSolver(query, env,
                           ObjectAssignment::NeededForObjectsFromEnv, &values,
                           /*validityCore=*/NULL, hasSolution);
}

bool SmithrilSolverImpl::check(const ConstraintQuery &query,
                               SmithrilSolverEnv &env,
                               ref<SolverResponse> &result) {
  std::vector<SparseStorageImpl<unsigned char>> values;
  ValidityCore validityCore;
  bool hasSolution = false;
  bool status =
      internalRunSolver(query, env, ObjectAssignment::NeededForObjectsFromQuery,
                        &values, &validityCore, hasSolution);
  if (status) {
    result = hasSolution
                 ? (SolverResponse *)new InvalidResponse(env.objects.v, values)
                 : (SolverResponse *)new ValidResponse(validityCore);
  }
  return status;
}

bool SmithrilSolverImpl::computeValidityCore(const ConstraintQuery &query,
                                             SmithrilSolverEnv &env,
                                             ValidityCore &validityCore,
                                             bool &isValid) {
  bool hasSolution = false; // to remove compiler warning
  bool status =
      internalRunSolver(query, /*env=*/env, ObjectAssignment::NotNeeded,
                        /*values=*/NULL, &validityCore, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool SmithrilSolverImpl::internalRunSolver(
    const ConstraintQuery &query, SmithrilSolverEnv &env,
    ObjectAssignment needObjects,
    std::vector<SparseStorageImpl<unsigned char>> *values,
    ValidityCore *validityCore, bool &hasSolution) {
  TimerStatIncrementer t(stats::queryTime);
  runStatusCode = SolverImpl::SOLVER_RUN_STATUS_FAILURE;

  std::unordered_set<const Array *> all_constant_arrays_in_query;
  SmithrilASTIncSet exprs;

  for (size_t i = 0; i < query.constraints.framesSize();
       i++, env.push(), exprs.push()) {
    ConstantArrayFinder constant_arrays_in_query;
    env.symbolicObjects.insert(query.constraints.begin(i),
                               query.constraints.end(i));
    // FIXME: findSymbolicObjects template does not support inc_uset::iterator
    //  findSymbolicObjects(env.symbolicObjects.begin(-1),
    //  env.symbolicObjects.end(-1), env.objects.v);
    std::vector<ref<Expr>> tmp(env.symbolicObjects.begin(-1),
                               env.symbolicObjects.end(-1));
    findSymbolicObjects(tmp.begin(), tmp.end(), env.objects.v);
    for (auto cs_it = query.constraints.begin(i),
              cs_ite = query.constraints.end(i);
         cs_it != cs_ite; cs_it++) {
      const auto &constraint = *cs_it;
      smithril::SmithrilTerm smithrilConstraint =
          builder->construct(constraint);
      if (ProduceUnsatCore && validityCore) {
        env.smithril_ast_expr_to_klee_expr.insert(
            {smithrilConstraint, constraint});
        env.smithril_ast_expr_constraints.v.push_back(smithrilConstraint);
        env.expr_to_track.insert(smithrilConstraint);
      }

      exprs.insert(smithrilConstraint);

      constant_arrays_in_query.visit(constraint);

      std::vector<ref<ReadExpr>> reads;
      findReads(constraint, true, reads);
      for (const auto &readExpr : reads) {
        auto readFromArray = readExpr->updates.root;
        assert(readFromArray);
        env.usedArrayBytes[readFromArray].insert(readExpr->index);
      }
    }

    for (auto constant_array : constant_arrays_in_query.results) {
      if (all_constant_arrays_in_query.count(constant_array))
        continue;
      all_constant_arrays_in_query.insert(constant_array);
      const auto &cas = builder->constant_array_assertions[constant_array];
      exprs.insert(cas.begin(), cas.end());
    }

    // Assert an generated side constraints we have to this last so that all
    // other constraints have been traversed so we have all the side constraints
    // needed.
    exprs.insert(builder->sideConstraints.begin(),
                 builder->sideConstraints.end());
  }
  exprs.pop(1); // drop last empty frame

  ++stats::solverQueries;
  if (!env.objects.v.empty())
    ++stats::queryCounterexamples;

  smithril::SmithrilSolver theSolver = initNativeSmithril(query, exprs);

  for (size_t i = 0; i < exprs.framesSize(); i++) {
    push(theSolver);
    for (auto it = exprs.begin(i), ie = exprs.end(i); it != ie; ++it) {
      smithril_assert(theSolver, (*it));
    }
  }

  smithril::SolverResult satisfiable = smithril_check_sat(theSolver);

  runStatusCode = handleSolverResponse(theSolver, satisfiable, env, needObjects,
                                       values, hasSolution);

  if (ProduceUnsatCore && validityCore &&
      satisfiable == smithril::SolverResult::Unsat) {
    constraints_ty unsatCore;
    const smithril::SmithrilTermVector smithril_unsat_core_vec =
        smithril_unsat_core(theSolver);
    unsigned size = smithril_unsat_core_size(smithril_unsat_core_vec);

    std::unordered_set<smithril::SmithrilTerm, SmithrilTermHash,
                       SmithrilTermCmp>
        smithril_ast_expr_unsat_core;

    for (unsigned index = 0; index < size; ++index) {
      smithril::SmithrilTerm constraint =
          smithril_unsat_core_get(builder->ctx, smithril_unsat_core_vec, index);
      smithril_ast_expr_unsat_core.insert(constraint);
    }

    for (const auto &z3_constraint : env.smithril_ast_expr_constraints.v) {
      if (smithril_ast_expr_unsat_core.find(z3_constraint) !=
          smithril_ast_expr_unsat_core.end()) {
        ref<Expr> constraint =
            env.smithril_ast_expr_to_klee_expr[z3_constraint];
        if (Expr::createIsZero(constraint) != query.getOriginalQueryExpr()) {
          unsatCore.insert(constraint);
        }
      }
    }

    assert(validityCore && "validityCore cannot be nullptr");
    *validityCore = ValidityCore(unsatCore, query.getOriginalQueryExpr());

    // Z3_ast_vector assertions =
    //     Z3_solver_get_assertions(builder->ctx, theSolver);
    // Z3_ast_vector_inc_ref(builder->ctx, assertions);
    // unsigned assertionsCount = Z3_ast_vector_size(builder->ctx, assertions);
    unsigned assertionsCount = 0; // mixa117 impl smithril_solver_get_assertions

    stats::validQueriesSize += assertionsCount;
    stats::validityCoresSize += size;
    ++stats::queryValidityCores;
  }

  deinitNativeSmithril(theSolver);

  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Term expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();
  builder->clearSideConstraints();
  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE ||
      runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE) {
    if (hasSolution) {
      ++stats::queriesInvalid;
    } else {
      ++stats::queriesValid;
    }
    return true; // success
  }

  return false; // failed
}

// mixa117 handleSolverResponse - double check (for me)
SolverImpl::SolverRunStatus SmithrilSolverImpl::handleSolverResponse(
    smithril::SmithrilSolver theSolver, smithril::SolverResult satisfiable,
    const SmithrilSolverEnv &env, ObjectAssignment needObjects,
    std::vector<SparseStorageImpl<unsigned char>> *values, bool &hasSolution) {
  switch (satisfiable) {
  case smithril::SolverResult::Sat: {
    hasSolution = true;
    auto objects = env.getObjectsForGetModel(needObjects);
    if (!objects) {
      // No assignment is needed
      assert(!values);
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }
    assert(values && "values cannot be nullptr");

    values->reserve(objects->size());
    for (auto array : *objects) {
      SparseStorageImpl<unsigned char> data;

      if (env.usedArrayBytes.count(array)) {
        std::unordered_set<uint64_t> offsetValues;
        for (const ref<Expr> &offsetExpr : env.usedArrayBytes.at(array)) {
          std::string arrayElementOffsetExpr =
              smithril_eval(theSolver, builder->construct(offsetExpr));

          uint64_t concretizedOffsetValue = std::stoull(arrayElementOffsetExpr);
          offsetValues.insert(concretizedOffsetValue);
        }

        for (unsigned offset : offsetValues) {
          // We can't use Term here so have to do ref counting manually
          smithril::SmithrilTerm initial_read =
              builder->getInitialRead(array, offset);

          std::string initial_read_expr =
              smithril_eval(theSolver, initial_read);

          uint64_t arrayElementValue = std::stoull(initial_read_expr);
          data.store(offset, arrayElementValue);
        }
      }

      values->emplace_back(std::move(data));
    }

    assert(values->size() == objects->size());

    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
  }
  case smithril::SolverResult::Unsat:
    hasSolution = false;
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
  case smithril::SolverResult::Unknown: {
    return SolverImpl::SOLVER_RUN_STATUS_FAILURE;
  }
  default:
    llvm_unreachable("unhandled smithril result");
  }
}

SolverImpl::SolverRunStatus SmithrilSolverImpl::getOperationStatusCode() {
  return runStatusCode;
}

class SmithrilNonIncSolverImpl final : public SmithrilSolverImpl {
private:
public:
  SmithrilNonIncSolverImpl() = default;

  /// implementation of BitwuzlaSolverImpl interface
  smithril::SmithrilSolver initNativeSmithril(const ConstraintQuery &,
                                              SmithrilASTIncSet &) override {
    auto ctx = builder->ctx;

    smithril::SmithrilSolver theSolver =
        smithril_new_solver(ctx, solverParameters);
    return theSolver;
  }

  void deinitNativeSmithril(smithril::SmithrilSolver theSolver) override {
    deleteNativeSmithril(theSolver);
  }

  void push(smithril::SmithrilSolver) override {}

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid) override {
    SmithrilSolverEnv env;
    return SmithrilSolverImpl::computeTruth(ConstraintQuery(query, false), env,
                                            isValid);
  }
  bool computeValue(const Query &query, ref<Expr> &result) override {
    SmithrilSolverEnv env;
    return SmithrilSolverImpl::computeValue(ConstraintQuery(query, false), env,
                                            result);
  }
  bool
  computeInitialValues(const Query &query,
                       const std::vector<const Array *> &objects,
                       std::vector<SparseStorageImpl<unsigned char>> &values,
                       bool &hasSolution) override {
    SmithrilSolverEnv env(objects);
    return SmithrilSolverImpl::computeInitialValues(
        ConstraintQuery(query, false), env, values, hasSolution);
  }
  bool check(const Query &query, ref<SolverResponse> &result) override {
    SmithrilSolverEnv env;
    return SmithrilSolverImpl::check(ConstraintQuery(query, false), env,
                                     result);
  }
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid) override {
    SmithrilSolverEnv env;
    return SmithrilSolverImpl::computeValidityCore(
        ConstraintQuery(query, false), env, validityCore, isValid);
  }
  void notifyStateTermination(std::uint32_t) override {}
};

SmithrilSolver::SmithrilSolver()
    : Solver(std::make_unique<SmithrilNonIncSolverImpl>()) {}

struct ConstraintDistance {
  size_t toPopSize = 0;
  ConstraintQuery toPush;

  explicit ConstraintDistance() {}
  ConstraintDistance(const ConstraintQuery &q) : toPush(q) {}
  explicit ConstraintDistance(size_t toPopSize, const ConstraintQuery &q)
      : toPopSize(toPopSize), toPush(q) {}

  size_t getDistance() const { return toPopSize + toPush.size(); }

  bool isOnlyPush() const { return toPopSize == 0; }

  void dump() const {
    llvm::errs() << "ConstraintDistance: pop: " << toPopSize << "; push:\n";
    klee::dump(toPush.constraints);
  }
};

class SmithrilIncNativeSolver {
private:
  std::optional<smithril::SmithrilSolver> nativeSolver;
  smithril::SmithrilContext solverContext;

  smithril::SmithrilOptions solverParameters;
  /// underlying solver frames
  /// saved only for calculating distances from next queries
  ConstraintFrames frames;

  void pop(size_t popSize);

public:
  SmithrilSolverEnv env;
  std::uint32_t stateID = 0;
  bool isRecycled = false;

  SmithrilIncNativeSolver(smithril::SmithrilOptions solverParameters)
      : solverParameters(solverParameters) {}
  ~SmithrilIncNativeSolver();

  void clear();

  void distance(const ConstraintQuery &query, ConstraintDistance &delta) const;

  void popPush(ConstraintDistance &delta);

  smithril::SmithrilSolver getOrInit();

  bool isConsistent() const {
    return frames.framesSize() == env.objects.framesSize();
  }

  void dump() const { ::klee::dump(frames); }
};

void SmithrilIncNativeSolver::pop(size_t popSize) {
  if (!nativeSolver || !popSize)
    return;
  smithril::smithril_pop(nativeSolver.value(), popSize);
}

void SmithrilIncNativeSolver::popPush(ConstraintDistance &delta) {
  env.pop(delta.toPopSize);
  pop(delta.toPopSize);
  frames.pop(delta.toPopSize);
  frames.extend(delta.toPush.constraints);
}

smithril::SmithrilSolver SmithrilIncNativeSolver::getOrInit() {
  if (!nativeSolver.has_value()) {
    nativeSolver =
        smithril::smithril_new_solver(solverContext, solverParameters);
  }
  return nativeSolver.value();
}

SmithrilIncNativeSolver::~SmithrilIncNativeSolver() {
  if (nativeSolver.has_value()) {
    deleteNativeSmithril(nativeSolver.value());
  }
}

void SmithrilIncNativeSolver::clear() {
  if (!nativeSolver.has_value())
    return;
  env.clear();
  frames.clear();
  smithril_reset(nativeSolver.value());
  isRecycled = false;
}

void SmithrilIncNativeSolver::distance(const ConstraintQuery &query,
                                       ConstraintDistance &delta) const {
  auto sit = frames.v.begin();
  auto site = frames.v.end();
  auto qit = query.constraints.v.begin();
  auto qite = query.constraints.v.end();
  auto it = frames.begin();
  auto ite = frames.end();
  size_t intersect = 0;
  for (; it != ite && sit != site && qit != qite && *sit == *qit; it++) {
    size_t frame_size = *it;
    for (size_t i = 0;
         i < frame_size && sit != site && qit != qite && *sit == *qit;
         i++, sit++, qit++, intersect++) {
    }
  }
  for (; sit != site && qit != qite && *sit == *qit;
       sit++, qit++, intersect++) {
  }
  size_t toPop, extraTakeFromOther;
  ConstraintFrames d;
  if (sit == site) { // solver frames ended
    toPop = 0;
    extraTakeFromOther = 0;
  } else {
    frames.takeBefore(intersect, toPop, extraTakeFromOther);
  }
  query.constraints.takeAfter(intersect - extraTakeFromOther, d);
  ConstraintQuery q(std::move(d), query.getOriginalQueryExpr());
  delta = ConstraintDistance(toPop, std::move(q));
}

class SmithrilTreeSolverImpl final : public SmithrilSolverImpl {
private:
  using solvers_ty = std::vector<std::unique_ptr<SmithrilIncNativeSolver>>;
  using solvers_it = solvers_ty::iterator;

  const size_t maxSolvers;
  std::unique_ptr<SmithrilIncNativeSolver> currentSolver = nullptr;
  solvers_ty solvers;

  void findSuitableSolver(const ConstraintQuery &query,
                          ConstraintDistance &delta);
  void setSolver(solvers_it &it, bool recycle = false);
  ConstraintQuery prepare(const Query &q);

public:
  SmithrilTreeSolverImpl(size_t maxSolvers) : maxSolvers(maxSolvers){};

  /// implementation of SmithrilSolverImpl interface
  smithril::SmithrilSolver initNativeSmithril(const ConstraintQuery &,
                                              SmithrilASTIncSet &) override {
    return currentSolver->getOrInit();
  }

  void deinitNativeSmithril(smithril::SmithrilSolver) override {
    assert(currentSolver->isConsistent());
    solvers.push_back(std::move(currentSolver));
  }
  void push(smithril::SmithrilSolver s) override { smithril_push(s); }

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid) override;
  bool computeValue(const Query &query, ref<Expr> &result) override;
  bool
  computeInitialValues(const Query &query,
                       const std::vector<const Array *> &objects,
                       std::vector<SparseStorageImpl<unsigned char>> &values,
                       bool &hasSolution) override;
  bool check(const Query &query, ref<SolverResponse> &result) override;
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid) override;

  void notifyStateTermination(std::uint32_t id) override;
};

void SmithrilTreeSolverImpl::setSolver(solvers_it &it, bool recycle) {
  assert(it != solvers.end());
  currentSolver = std::move(*it);
  solvers.erase(it);
  currentSolver->isRecycled = false;
  if (recycle)
    currentSolver->clear();
}

void SmithrilTreeSolverImpl::findSuitableSolver(const ConstraintQuery &query,
                                                ConstraintDistance &delta) {
  ConstraintDistance min_delta;
  auto min_distance = std::numeric_limits<size_t>::max();
  auto min_it = solvers.end();
  auto free_it = solvers.end();
  for (auto it = solvers.begin(), ite = min_it; it != ite; it++) {
    if ((*it)->isRecycled)
      free_it = it;
    (*it)->distance(query, delta);
    if (delta.isOnlyPush()) {
      setSolver(it);
      return;
    }
    auto distance = delta.getDistance();
    if (distance < min_distance) {
      min_delta = delta;
      min_distance = distance;
      min_it = it;
    }
  }
  if (solvers.size() < maxSolvers) {
    delta = ConstraintDistance(query);
    if (delta.getDistance() < min_distance) {
      // it is cheaper to create new solver
      if (free_it == solvers.end())
        currentSolver =
            std::make_unique<SmithrilIncNativeSolver>(solverParameters);
      else
        setSolver(free_it, /*recycle=*/true);
      return;
    }
  }
  assert(min_it != solvers.end());
  delta = min_delta;
  setSolver(min_it);
}

ConstraintQuery SmithrilTreeSolverImpl::prepare(const Query &q) {
  ConstraintDistance delta;
  ConstraintQuery query(q, true);
  findSuitableSolver(query, delta);
  assert(currentSolver->isConsistent());
  currentSolver->stateID = q.id;
  currentSolver->popPush(delta);
  return delta.toPush;
}

bool SmithrilTreeSolverImpl::computeTruth(const Query &query, bool &isValid) {
  auto q = prepare(query);
  return SmithrilSolverImpl::computeTruth(q, currentSolver->env, isValid);
}

bool SmithrilTreeSolverImpl::computeValue(const Query &query,
                                          ref<Expr> &result) {
  auto q = prepare(query);
  return SmithrilSolverImpl::computeValue(q, currentSolver->env, result);
}

bool SmithrilTreeSolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution) {
  auto q = prepare(query);
  currentSolver->env.objectsForGetModel = objects;
  return SmithrilSolverImpl::computeInitialValues(q, currentSolver->env, values,
                                                  hasSolution);
}

bool SmithrilTreeSolverImpl::check(const Query &query,
                                   ref<SolverResponse> &result) {
  auto q = prepare(query);
  return SmithrilSolverImpl::check(q, currentSolver->env, result);
}

bool SmithrilTreeSolverImpl::computeValidityCore(const Query &query,
                                                 ValidityCore &validityCore,
                                                 bool &isValid) {
  auto q = prepare(query);
  return SmithrilSolverImpl::computeValidityCore(q, currentSolver->env,
                                                 validityCore, isValid);
}

void SmithrilTreeSolverImpl::notifyStateTermination(std::uint32_t id) {
  for (auto &s : solvers)
    if (s->stateID == id)
      s->isRecycled = true;
}

SmithrilTreeSolver::SmithrilTreeSolver(unsigned maxSolvers)
    : Solver(std::make_unique<SmithrilTreeSolverImpl>(maxSolvers)) {}

} // namespace klee
#endif // ENABLE_SMITHRIL
