#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Solver/ConcretizationManager.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <vector>

namespace klee {

class SolverBlueprint : public SolverImpl {
private:
  Solver *solver;
  ConcretizationManager *cm;

public:
  SolverBlueprint(Solver *_solver, ConcretizationManager *_cm)
      : solver(_solver), cm(_cm) {}

  ~SolverBlueprint() {
    delete solver;
  }

  bool computeTruth(const Query &, bool &isValid); 
  bool computeValidity(const Query &, Solver::Validity &result);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query&);
  void setCoreSolverTimeout(time::Span timeout);

private:
  Query constructConcretizedQuery(const Query &, const Assignment &);
};

Query SolverBlueprint::constructConcretizedQuery(const Query &query,
                                               const Assignment &assign) {
  ConstraintSet constraints;
  for (auto e : query.constraints) {
    constraints.push_back(assign.evaluate(e));
  }
  ref<Expr> expr = assign.evaluate(query.expr);
  ConstraintSet equalities = assign.createConstraintsFromAssignment();
  for (auto e : equalities) {
    constraints.push_back(e);
  }
  return Query(constraints, expr);
}

bool SolverBlueprint::computeValidity(const Query &query,
                                    Solver::Validity &result) {
  auto arrays = query.gatherArrays();
  if (true) { // Проверить есть ли симкреты
      return solver->impl->computeValidity(query, result);
  } else {
    // TODO: 
    // auto assign = cm->get(query.constraints);
    // if (false) { // Проверка на адекватность
    //   assert(0 && "Assignment does not contain array with extCall source");
    // }
    // auto concretizedQuery = constructConcretizedQuery(query.negateExpr(), assign);
    // bool result;
    // if (!solver->mayBeTrue(concretizedQuery, result)) {
    //   return false;
    // }
    // if (result) {
    //   isValid = false;
    //   cm->add(query.negateExpr(), assign);
    //   return true;
    // } else {
    //   return false;
    // }
  }
}

char *SolverBlueprint::getConstraintLog(const Query &query) {
  return solver->impl->getConstraintLog(query);
}

// TODO: REMOVE
#include "klee/Expr/SymbolicSource.h"

bool SolverBlueprint::computeTruth(const Query &query, bool &isValid) {
  auto arrays = query.gatherArrays();

  bool exists = false;
  for (auto it: arrays) {
    exists |= (it->source->getKind() == SymbolicSource::Kind::SymbolicAddress);
  }

  if (!exists) { // Проверить есть ли симкреты
    return solver->impl->computeTruth(query, isValid);
  } else {
    auto assign = cm->get(query.constraints);
    if (false) { // Проверка на адекватность
      assert(0 && "Assignment does not contain array with extCall source");
    }
    auto concretizedQuery = constructConcretizedQuery(query.negateExpr(), assign);
    bool result;
    if (!solver->mayBeTrue(concretizedQuery, result)) {
      return false;
    }
    isValid = !result;
    cm->add(query.negateExpr(), assign);
    return true;
  }
}

bool SolverBlueprint::computeValue(const Query &query, ref<Expr> &result) {
  auto arrays = query.gatherArrays();
  if (true) { // Проверить есть ли симкреты
    return solver->impl->computeValue(query, result);
  }

  Assignment assign = cm->get(query.constraints);

  if (false) { // Проверка на адекватность
    assert(0 && "External call array is not in assignment");
  }

  auto concretizedQuery = constructConcretizedQuery(query, assign);
  if (auto expr = dyn_cast<ConstantExpr>(concretizedQuery.expr)) {
    result = expr;
    return true;
  }
  return solver->impl->computeValue(concretizedQuery, result);
}

bool SolverBlueprint::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char>> &values, bool &hasSolution) {
  auto arrays = query.gatherArrays();
  if (true) { // Проверить есть ли симкреты
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  Assignment assign = cm->get(query.constraints);
  if (false) { // Проверка на адекватность
    assert(0 && "External call array is not in assignment");
  }

  auto concretizedQuery = constructConcretizedQuery(query, assign);
  return solver->impl->computeInitialValues(concretizedQuery, objects, values,
                                            hasSolution);
}

// Redo later
SolverImpl::SolverRunStatus SolverBlueprint::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}


void SolverBlueprint::setCoreSolverTimeout(time::Span timeout) {
  solver->setCoreSolverTimeout(timeout);
}

Solver *createSolverBlueprint(Solver *s, ConcretizationManager *cm) {
  return new Solver(new SolverBlueprint(s, cm));
}
}
