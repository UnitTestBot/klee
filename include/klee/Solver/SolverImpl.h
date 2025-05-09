//===-- SolverImpl.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SOLVERIMPL_H
#define KLEE_SOLVERIMPL_H

#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/SolverUtil.h"
#include "klee/System/Time.h"

#include <vector>

namespace klee {
class Array;
class ExecutionState;
class Expr;
struct Query;

/// SolverImpl - Abstract base class for solver implementations.
class SolverImpl {
public:
  SolverImpl() = default;

  SolverImpl(const SolverImpl &) = delete;
  SolverImpl &operator=(const SolverImpl &) = delete;

  virtual ~SolverImpl();

  enum SolverRunStatus {
    SOLVER_RUN_STATUS_SUCCESS_SOLVABLE,
    SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE,
    SOLVER_RUN_STATUS_FAILURE,
    SOLVER_RUN_STATUS_TIMEOUT,
    SOLVER_RUN_STATUS_FORK_FAILED,
    SOLVER_RUN_STATUS_INTERRUPTED,
    SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE,
    SOLVER_RUN_STATUS_WAITPID_FAILED
  };

  /// computeValidity - Compute a full validity result for the
  /// query.
  ///
  /// The query expression is guaranteed to be non-constant and have
  /// bool type.
  ///
  /// SolverImpl provides a default implementation which uses
  /// computeTruth. Clients should override this if a more efficient
  /// implementation is available.
  ///
  /// \param [out] result - if
  /// \f[ \forall X constraints(X) \to query(X) \f]
  /// then Solver::True,
  /// else if
  /// \f[ \forall X constraints(X) \to \lnot query(X) \f]
  /// then Solver::False,
  /// else
  /// Solver::Unknown
  ///
  /// \return True on success
  virtual bool computeValidity(const Query &query, PartialValidity &result);

  virtual bool computeValidity(const Query &query,
                               ref<SolverResponse> &queryResult,
                               ref<SolverResponse> &negatedQueryResult);

  /// computeTruth - Determine whether the given query expression is provably
  /// true given the constraints.
  ///
  /// The query expression is guaranteed to be non-constant and have
  /// bool type.
  ///
  /// This method should evaluate the logical formula:
  ///
  /// \f[ \forall X constraints(X) \to query(X) \f]
  ///
  /// Where \f$X\f$ is some assignment, \f$constraints(X)\f$ are the constraints
  /// in the query and \f$query(X)\f$ is the query expression.
  ///
  /// \param [out] isValid - On success, true iff the logical formula is true.
  /// \return True on success
  virtual bool computeTruth(const Query &query, bool &isValid) = 0;

  /// computeValue - Compute a feasible value for the expression.
  ///
  /// The query expression is guaranteed to be non-constant.
  ///
  /// \return True on success
  virtual bool computeValue(const Query &query, ref<Expr> &result) = 0;

  /// \sa Solver::getInitialValues()
  virtual bool
  computeInitialValues(const Query &query,
                       const std::vector<const Array *> &objects,
                       std::vector<SparseStorageImpl<unsigned char>> &values,
                       bool &hasSolution) = 0;

  virtual bool check(const Query &query, ref<SolverResponse> &result);

  virtual bool computeValidityCore(const Query &query,
                                   ValidityCore &validityCore, bool &isValid);

  virtual bool computeMinimalUnsignedValue(const Query &query,
                                           ref<ConstantExpr> &result);

  /// getOperationStatusCode - get the status of the last solver operation
  virtual SolverRunStatus getOperationStatusCode() = 0;

  /// getOperationStatusString - get string representation of the operation
  /// status code
  static const char *getOperationStatusString(SolverRunStatus statusCode);

  virtual std::string getConstraintLog(const Query &query) {
    // dummy
    return {};
  }

  virtual void setCoreSolverLimits(time::Span, unsigned) {}

  virtual void notifyStateTermination(std::uint32_t id) = 0;
};

} // namespace klee

#endif /* KLEE_SOLVERIMPL_H */
