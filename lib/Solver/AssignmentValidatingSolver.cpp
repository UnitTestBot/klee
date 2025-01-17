//===-- AssignmentValidatingSolver.cpp ------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

#include <memory>
#include <utility>
#include <vector>

namespace klee {

class AssignmentValidatingSolver : public SolverImpl {
private:
  std::unique_ptr<Solver> solver;
  void dumpAssignmentQuery(const Query &query, const Assignment &assignment);

public:
  AssignmentValidatingSolver(std::unique_ptr<Solver> solver)
      : solver(std::move(solver)) {}

  bool computeValidity(const Query &, PartialValidity &result);
  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool
  computeInitialValues(const Query &, const std::vector<const Array *> &objects,
                       std::vector<SparseStorageImpl<unsigned char>> &values,
                       bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  void
  validateAssignment(const Query &query,
                     const std::vector<const Array *> &objects,
                     std::vector<SparseStorageImpl<unsigned char>> &values);
  SolverRunStatus getOperationStatusCode();
  std::string getConstraintLog(const Query &) final;
  void setCoreSolverLimits(time::Span timeout, unsigned memoryLimit);
  void notifyStateTermination(std::uint32_t id);
};

// TODO: use computeInitialValues for all queries for more stress testing
bool AssignmentValidatingSolver::computeValidity(const Query &query,
                                                 PartialValidity &result) {
  return solver->impl->computeValidity(query, result);
}
bool AssignmentValidatingSolver::computeTruth(const Query &query,
                                              bool &isValid) {
  return solver->impl->computeTruth(query, isValid);
}
bool AssignmentValidatingSolver::computeValue(const Query &query,
                                              ref<Expr> &result) {
  return solver->impl->computeValue(query, result);
}

void AssignmentValidatingSolver::validateAssignment(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values) {
  // Use `_allowFreeValues` so that if we are missing an assignment
  // we can't compute a constant and flag this as a problem.
  Assignment assignment(objects, values);
  // Check computed assignment satisfies query
  for (const auto &constraint : query.constraints.cs()) {
    ref<Expr> constraintEvaluated = assignment.evaluate(constraint);
    ConstantExpr *CE = dyn_cast<ConstantExpr>(constraintEvaluated);
    if (CE == NULL) {
      llvm::errs() << "Constraint did not evalaute to a constant:\n";
      llvm::errs() << "Constraint:\n" << constraint << "\n";
      llvm::errs() << "Evaluated Constraint:\n" << constraintEvaluated << "\n";
      llvm::errs() << "Assignment:\n";
      assignment.dump();
      dumpAssignmentQuery(query, assignment);
      abort();
    }
    if (CE->isFalse()) {
      llvm::errs() << "Constraint evaluated to false when using assignment\n";
      llvm::errs() << "Constraint:\n" << constraint << "\n";
      llvm::errs() << "Assignment:\n";
      assignment.dump();
      dumpAssignmentQuery(query, assignment);
      abort();
    }
  }

  ref<Expr> queryExprEvaluated = assignment.evaluate(query.expr);
  ConstantExpr *CE = dyn_cast<ConstantExpr>(queryExprEvaluated);
  if (CE == NULL) {
    llvm::errs() << "Query expression did not evalaute to a constant:\n";
    llvm::errs() << "Expression:\n" << query.expr << "\n";
    llvm::errs() << "Evaluated expression:\n" << queryExprEvaluated << "\n";
    llvm::errs() << "Assignment:\n";
    assignment.dump();
    dumpAssignmentQuery(query, assignment);
    abort();
  }
  // KLEE queries are validity queries. A counter example to
  // ∀ x constraints(x) → query(x)
  // exists therefore
  // ¬∀ x constraints(x) → query(x)
  // Which is equivalent to
  // ∃ x constraints(x) ∧ ¬ query(x)
  // This means for the assignment we get back query expression should evaluate
  // to false.
  if (CE->isTrue()) {
    llvm::errs()
        << "Query Expression evaluated to true when using assignment\n";
    llvm::errs() << "Expression:\n" << query.expr << "\n";
    llvm::errs() << "Assignment:\n";
    assignment.dump();
    dumpAssignmentQuery(query, assignment);
    abort();
  }
}

bool AssignmentValidatingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution) {
  bool success =
      solver->impl->computeInitialValues(query, objects, values, hasSolution);
  if (!hasSolution)
    return success;

  validateAssignment(query, objects, values);

  return success;
}

bool AssignmentValidatingSolver::check(const Query &query,
                                       ref<SolverResponse> &result) {
  if (!solver->impl->check(query, result)) {
    return false;
  }
  if (isa<ValidResponse>(result)) {
    return true;
  }

  std::vector<const Array *> objects;
  findSymbolicObjects(query, objects);
  std::vector<SparseStorageImpl<unsigned char>> values;

  assert(isa<InvalidResponse>(result));
  cast<InvalidResponse>(result)->tryGetInitialValuesFor(objects, values);

  validateAssignment(query, objects, values);

  return true;
}

bool AssignmentValidatingSolver::computeValidityCore(const Query &query,
                                                     ValidityCore &validityCore,
                                                     bool &isValid) {
  return solver->impl->computeValidityCore(query, validityCore, isValid);
}

void AssignmentValidatingSolver::dumpAssignmentQuery(
    const Query &query, const Assignment &assignment) {
  // Create a Query that is augmented with constraints that
  // enforce the given assignment.
  ConstraintSet constraints(assignment.createConstraintsFromAssignment());

  // Add Constraints from `query`
  for (const auto &constraint : query.constraints.cs())
    constraints.addConstraint(constraint);

  Query augmentedQuery = query.withConstraints(constraints);

  // Ask the solver for the log for this query.
  llvm::errs() << "Query with assignment as constraints:\n"
               << solver->getConstraintLog(augmentedQuery) << "\n";
}

SolverImpl::SolverRunStatus
AssignmentValidatingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

std::string AssignmentValidatingSolver::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

void AssignmentValidatingSolver::setCoreSolverLimits(time::Span timeout,
                                                     unsigned memoryLimit) {
  return solver->impl->setCoreSolverLimits(timeout, memoryLimit);
}

void AssignmentValidatingSolver::notifyStateTermination(std::uint32_t id) {
  solver->impl->notifyStateTermination(id);
}

std::unique_ptr<Solver>
createAssignmentValidatingSolver(std::unique_ptr<Solver> s) {
  return std::make_unique<Solver>(
      std::make_unique<AssignmentValidatingSolver>(std::move(s)));
}
} // namespace klee
