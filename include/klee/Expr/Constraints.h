//===-- Constraints.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CONSTRAINTS_H
#define KLEE_CONSTRAINTS_H

#include "klee/ADT/Ref.h"

#include "klee/ADT/PersistentMap.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/Path.h"
#include "klee/Expr/Symcrete.h"

#include <vector>

namespace klee {

enum class RewriteEqualitiesPolicy { None, Simple, Full };

class MemoryObject;
struct KInstruction;

/// Resembles a set of constraints that can be passed around
///
class ConstraintSet {
private:
  /// Epoch counter used to control ownership of objects.
  mutable unsigned cowKey;

  constraints_ty _constraints;
  symcretes_ty _symcretes;
  mutable std::shared_ptr<Assignment> _concretization;
  std::shared_ptr<IndependentConstraintSetUnion> _independentElements;
  unsigned copyOnWriteOwner;

  void checkCopyOnWriteOwner();

public:
  ConstraintSet(constraints_ty cs, symcretes_ty symcretes,
                Assignment concretization);
  explicit ConstraintSet(ref<const IndependentConstraintSet> ics);
  explicit ConstraintSet(
      const std::vector<ref<const IndependentConstraintSet>> &ics,
      const ExprHashMap<ref<Expr>> &concretizedExprs);
  explicit ConstraintSet(constraints_ty cs);
  explicit ConstraintSet();
  ConstraintSet(const ConstraintSet &b)
      : cowKey(++b.cowKey), _constraints(b._constraints),
        _symcretes(b._symcretes), _concretization(b._concretization),
        _independentElements(b._independentElements),
        copyOnWriteOwner(b.copyOnWriteOwner) {}
  ConstraintSet &operator=(const ConstraintSet &b) {
    cowKey = ++b.cowKey;
    _constraints = b._constraints;
    _symcretes = b._symcretes;
    _concretization = b._concretization;
    _independentElements = b._independentElements;
    copyOnWriteOwner = b.copyOnWriteOwner;
    return *this;
  }

  void addConstraint(ref<Expr> e);
  void addSymcrete(ref<Symcrete> s);
  bool isSymcretized(ref<Expr> expr) const;

  void rewriteConcretization(const Assignment &a) const;
  ConstraintSet withExpr(ref<Expr> e) const {
    ConstraintSet copy = ConstraintSet(*this);
    copy.addConstraint(e);
    return copy;
  }

  std::vector<const Array *> gatherArrays() const;
  std::vector<const Array *> gatherSymcretizedArrays() const;

  bool operator==(const ConstraintSet &b) const {
    return _constraints == b._constraints && _symcretes == b._symcretes;
  }

  bool operator<(const ConstraintSet &b) const {
    return _constraints < b._constraints ||
           (_constraints == b._constraints && _symcretes < b._symcretes);
  }
  ConstraintSet getConcretizedVersion() const;
  ConstraintSet getConcretizedVersion(const Assignment &c) const;
  void dump() const;
  void print(llvm::raw_ostream &os) const;

  void changeCS(constraints_ty &cs);

  const constraints_ty &cs() const;
  const symcretes_ty &symcretes() const;
  const Assignment &concretization() const;
  const IndependentConstraintSetUnion &independentElements() const;

  void getAllIndependentConstraintsSets(
      ref<Expr> queryExpr,
      std::vector<ref<const IndependentConstraintSet>> &result) const;

  void getAllDependentConstraintsSets(
      ref<Expr> queryExpr,
      std::vector<ref<const IndependentConstraintSet>> &result) const;
};

class PathConstraints {
public:
  using ordered_constraints_ty =
      PersistentMap<Path::PathIndex, constraints_ty, Path::PathIndexCompare>;

  void advancePath(KInstruction *ki);
  void advancePath(const Path &path);

  ExprHashSet addConstraint(ref<Expr> e);
  bool isSymcretized(ref<Expr> expr) const;
  void addSymcrete(ref<Symcrete> s);
  void rewriteConcretization(const Assignment &a);

  const ConstraintSet &cs() const;
  const Path &path() const;

  static PathConstraints concat(const PathConstraints &l,
                                const PathConstraints &r);

private:
  Path _path;
  ConstraintSet constraints;
  unsigned long addingCounter = 0UL;
};

struct Conflict {
  Path path;
  constraints_ty core;
  Conflict() = default;
};

struct TargetedConflict {
  friend class ref<TargetedConflict>;

private:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

public:
  Conflict conflict;
  KBlock *target;

  TargetedConflict(Conflict &_conflict, KBlock *_target)
      : conflict(_conflict), target(_target) {}
};

class Simplificator {
public:
  struct ExprResult {
    ref<Expr> simplified;
    ExprHashSet dependency;
  };

  struct SetResult {
    constraints_ty simplified;
    ExprHashMap<ExprHashSet> dependency;
    bool wasSimplified;
  };

public:
  static ExprResult simplifyExpr(const constraints_ty &constraints,
                                 const ref<Expr> &expr);

  static ExprResult simplifyExpr(const ConstraintSet &constraints,
                                 const ref<Expr> &expr);

  static Simplificator::SetResult
  simplify(const constraints_ty &constraints,
           RewriteEqualitiesPolicy p = RewriteEqualitiesPolicy::Full);

  static ExprHashMap<ExprHashSet>
  composeExprDependencies(const ExprHashMap<ExprHashSet> &upper,
                          const ExprHashMap<ExprHashSet> &lower);

private:
  struct Replacements {
    ExprHashMap<ref<Expr>> equalities;
    ExprHashMap<ref<Expr>> equalitiesParents;
  };

private:
  static Replacements gatherReplacements(constraints_ty constraints);
  static void addReplacement(Replacements &replacements, ref<Expr> expr);
  static void removeReplacement(Replacements &replacements, ref<Expr> expr);
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ConstraintSet &constraints) {
  constraints.print(os);
  return os;
}

} // namespace klee

#endif /* KLEE_CONSTRAINTS_H */
