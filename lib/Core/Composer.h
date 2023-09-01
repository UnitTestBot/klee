#ifndef KLEE_COMPOSER_H
#define KLEE_COMPOSER_H

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Module/KModule.h"

#include "ExecutionState.h"
#include "Executor.h"
#include "Memory.h"
#include "TimingSolver.h"

namespace klee {
struct ComposeHelper {
private:
  Executor *executor;

public:
  ComposeHelper(Executor *_executor) : executor(_executor) {}

  bool getResponse(const ExecutionState &state, ref<Expr> expr,
                   ref<SolverResponse> &queryResult,
                   SolverQueryMetaData &metaData) {
    executor->solver->setTimeout(executor->coreSolverTimeout);
    bool success = executor->solver->getResponse(
        state.constraints.withAssumtions(state.assumptions), expr, queryResult,
        state.queryMetaData);
    executor->solver->setTimeout(time::Span());
    return success;
  }

  bool evaluate(const ExecutionState &state, ref<Expr> expr,
                PartialValidity &res, SolverQueryMetaData &metaData) {
    executor->solver->setTimeout(executor->coreSolverTimeout);
    bool success = executor->solver->evaluate(
        state.constraints.withAssumtions(state.assumptions), expr, res,
        state.queryMetaData);
    executor->solver->setTimeout(time::Span());
    return success;
  }

  bool evaluate(const ExecutionState &state, ref<Expr> expr,
                ref<SolverResponse> &queryResult,
                ref<SolverResponse> &negateQueryResult,
                SolverQueryMetaData &metaData) {
    executor->solver->setTimeout(executor->coreSolverTimeout);
    bool success = executor->solver->evaluate(
        state.constraints.withAssumtions(state.assumptions), expr, queryResult,
        negateQueryResult, state.queryMetaData);
    executor->solver->setTimeout(time::Span());
    return success;
  }

  bool resolveMemoryObjects(ExecutionState &state, ref<Expr> address,
                            KType *targetType, KInstruction *target,
                            unsigned bytes,
                            std::vector<IDType> &mayBeResolvedMemoryObjects,
                            bool &mayBeOutOfBound, bool &mayLazyInitialize,
                            bool &incomplete) {
    return executor->resolveMemoryObjects(
        state, address, targetType, target, bytes, mayBeResolvedMemoryObjects,
        mayBeOutOfBound, mayLazyInitialize, incomplete);
  }

  bool checkResolvedMemoryObjects(
      ExecutionState &state, ref<Expr> address, KInstruction *target,
      unsigned bytes, const std::vector<IDType> &mayBeResolvedMemoryObjects,
      bool hasLazyInitialized, std::vector<IDType> &resolvedMemoryObjects,
      std::vector<ref<Expr>> &resolveConditions,
      std::vector<ref<Expr>> &unboundConditions, ref<Expr> &checkOutOfBounds,
      bool &mayBeOutOfBound) {
    return executor->checkResolvedMemoryObjects(
        state, address, target, bytes, mayBeResolvedMemoryObjects,
        hasLazyInitialized, resolvedMemoryObjects, resolveConditions,
        unboundConditions, checkOutOfBounds, mayBeOutOfBound);
  }

  bool makeGuard(ExecutionState &state,
                 const std::vector<ref<Expr>> &resolveConditions,
                 const std::vector<ref<Expr>> &unboundConditions,
                 ref<Expr> checkOutOfBounds, bool hasLazyInitialized,
                 ref<Expr> &guard, bool &mayBeInBounds) {
    return executor->makeGuard(state, resolveConditions, unboundConditions,
                               checkOutOfBounds, hasLazyInitialized, guard,
                               mayBeInBounds);
  }

  bool collectConcretizations(ExecutionState &state,
                              const std::vector<ref<Expr>> &resolveConditions,
                              const std::vector<ref<Expr>> &unboundConditions,
                              const std::vector<IDType> &resolvedMemoryObjects,
                              ref<Expr> checkOutOfBounds,
                              bool hasLazyInitialized, ref<Expr> &guard,
                              std::vector<Assignment> &resolveConcretizations,
                              bool &mayBeInBounds) {
    return executor->collectConcretizations(
        state, resolveConditions, unboundConditions, resolvedMemoryObjects,
        checkOutOfBounds, hasLazyInitialized, guard, resolveConcretizations,
        mayBeInBounds);
  }

  Assignment computeConcretization(const ConstraintSet &constraints,
                                   ref<Expr> condition,
                                   SolverQueryMetaData &queryMetaData) {
    return executor->computeConcretization(constraints, condition,
                                           queryMetaData);
  }

  void updateStateWithSymcretes(ExecutionState &state,
                                const Assignment &assignment) {
    return executor->updateStateWithSymcretes(state, assignment);
  }

  bool collectMemoryObjects(ExecutionState &state, ref<Expr> address,
                            KType *targetType, KInstruction *target,
                            ref<Expr> &guard,
                            std::vector<ref<Expr>> &resolveConditions,
                            std::vector<ref<Expr>> &unboundConditions,
                            std::vector<IDType> &resolvedMemoryObjects);

  void collectReads(ExecutionState &state, ref<Expr> address, KType *targetType,
                    Expr::Width type, unsigned bytes,
                    const std::vector<IDType> &resolvedMemoryObjects,
                    const std::vector<Assignment> &resolveConcretizations,
                    std::vector<ref<Expr>> &results) {
    executor->collectReads(state, address, targetType, type, bytes,
                           resolvedMemoryObjects, resolveConcretizations,
                           results);
  }

  void
  collectObjectStates(ExecutionState &state, ref<Expr> address,
                      Expr::Width type, unsigned bytes,
                      const std::vector<IDType> &resolvedMemoryObjects,
                      const std::vector<Assignment> &resolveConcretizations,
                      std::vector<ref<ObjectState>> &results) {
    executor->collectObjectStates(state, address, type, bytes,
                                  resolvedMemoryObjects, resolveConcretizations,
                                  results);
  }

  bool tryResolveAddress(ExecutionState &state, ref<Expr> address,
                         std::pair<ref<Expr>, ref<Expr>> &result);
  bool tryResolveSize(ExecutionState &state, ref<Expr> address,
                      std::pair<ref<Expr>, ref<Expr>> &result);
  bool tryResolveContent(
      ExecutionState &state, ref<Expr> address, ref<Expr> offset,
      Expr::Width type, unsigned size,
      std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
          &result);

  ref<Expr> fillValue(ExecutionState &state, ref<ValueSource> valueSource,
                      ref<Expr> size) {
    return executor->fillValue(state, valueSource, size);
  }
  ref<ObjectState> fillMakeSymbolic(ExecutionState &state,
                                    ref<MakeSymbolicSource> makeSymbolicSource,
                                    ref<Expr> size, unsigned concreteSize) {
    return executor->fillMakeSymbolic(state, makeSymbolicSource, size,
                                      concreteSize);
  }

  ref<ObjectState> fillGlobal(ExecutionState &state,
                              ref<GlobalSource> globalSource) {
    return executor->fillGlobal(state, globalSource);
  }

  ref<ObjectState>
  fillIrreproducible(ExecutionState &state,
                     ref<IrreproducibleSource> irreproducibleSource,
                     ref<Expr> size, unsigned concreteSize) {
    return executor->fillIrreproducible(state, irreproducibleSource, size,
                                        concreteSize);
  }

  ref<ObjectState> fillConstant(ExecutionState &state,
                                ref<ConstantSource> constanSource,
                                ref<Expr> size) {
    return executor->fillConstant(state, constanSource, size);
  }

  ref<Expr> fillSymbolicSizeConstantAddress(
      ExecutionState &state,
      ref<SymbolicSizeConstantAddressSource> symbolicSizeConstantAddressSource,
      ref<Expr> size, Expr::Width width) {
    return executor->fillSymbolicSizeConstantAddress(
        state, symbolicSizeConstantAddressSource, size, width);
  }

  std::pair<ref<Expr>, ref<Expr>> getSymbolicSizeConstantSizeAddressPair(
      ExecutionState &state,
      ref<SymbolicSizeConstantAddressSource> symbolicSizeConstantAddressSource,
      ref<Expr> size, Expr::Width width) {
    return executor->getSymbolicSizeConstantSizeAddressPair(
        state, symbolicSizeConstantAddressSource, size, width);
  }

  ref<Expr> fillSizeAddressSymcretes(ExecutionState &state,
                                     ref<Expr> oldAddress, ref<Expr> newAddress,
                                     ref<Expr> size) {
    return executor->fillSizeAddressSymcretes(state, oldAddress, newAddress,
                                              size);
  }

  std::pair<ref<Expr>, ref<Expr>> fillLazyInitializationAddress(
      ExecutionState &state,
      ref<LazyInitializationAddressSource> lazyInitializationAddressSource,
      ref<Expr> pointer, Expr::Width width);
  std::pair<ref<Expr>, ref<Expr>> fillLazyInitializationSize(
      ExecutionState &state,
      ref<LazyInitializationSizeSource> lazyInitializationSizeSource,
      ref<Expr> pointer, Expr::Width width);
  std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
  fillLazyInitializationContent(
      ExecutionState &state,
      ref<LazyInitializationContentSource> lazyInitializationContentSource,
      ref<Expr> pointer, unsigned concreteSize, ref<Expr> offset,
      Expr::Width width);
};

class ComposeVisitor : public ExprVisitor {
private:
  const ExecutionState &original;
  ComposeHelper helper;
  ExprOrderedSet safetyConstraints;

public:
  ExecutionState &state;

  ComposeVisitor() = delete;
  explicit ComposeVisitor(const ExecutionState &_state, ComposeHelper _helper)
      : ExprVisitor(false), original(_state), helper(_helper),
        state(*original.copy()) {}
  ~ComposeVisitor() { delete &state; }

  std::pair<ref<Expr>, ref<Expr>> compose(ref<Expr> expr) {
    ref<Expr> result = visit(expr);
    ref<Expr> safetyCondition = Expr::createTrue();
    for (auto expr : safetyConstraints) {
      safetyCondition = AndExpr::create(safetyCondition, expr);
    }
    return std::make_pair(safetyCondition, result);
  }

private:
  ExprVisitor::Action visitRead(const ReadExpr &) override;
  ExprVisitor::Action visitConcat(const ConcatExpr &concat) override;
  ExprVisitor::Action visitSelect(const SelectExpr &) override;
  ref<Expr> processRead(const Array *root, const UpdateList &updates,
                        ref<Expr> index, Expr::Width width);
  ref<Expr> processSelect(ref<Expr> cond, ref<Expr> trueExpr,
                          ref<Expr> falseExpr);
  void shareUpdates(ref<ObjectState>, const UpdateList &updates);
};
} // namespace klee

#endif // KLEE_COMPOSITION_H
