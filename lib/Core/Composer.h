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
#include <variant>

namespace klee {
struct ComposeHelper {
private:
  Executor *executor;

public:
  ComposeHelper(Executor *_executor) : executor(_executor) {}

  bool getResponse(const ExecutionState &state, ref<Expr> expr,
                   ref<SolverResponse> &queryResult, SolverQueryMetaData &) {
    executor->solver->setLimits(executor->coreSolverTimeout, -1);
    bool success = executor->solver->getResponse(
        state.constraints.withAssumptions(state.assumptions), expr, queryResult,
        state.queryMetaData);
    executor->solver->setLimits(time::Span(), -1);
    return success;
  }

  bool evaluate(const ExecutionState &state, ref<Expr> expr,
                PartialValidity &res, SolverQueryMetaData &) {
    executor->solver->setLimits(executor->coreSolverTimeout, -1);
    bool success = executor->solver->evaluate(
        state.constraints.withAssumptions(state.assumptions), expr, res,
        state.queryMetaData);
    executor->solver->setLimits(time::Span(), -1);
    return success;
  }

  bool evaluate(const ExecutionState &state, ref<Expr> expr,
                ref<SolverResponse> &queryResult,
                ref<SolverResponse> &negateQueryResult, SolverQueryMetaData &) {
    executor->solver->setLimits(executor->coreSolverTimeout, -1);
    bool success = executor->solver->evaluate(
        state.constraints.withAssumptions(state.assumptions), expr, queryResult,
        negateQueryResult, state.queryMetaData);
    executor->solver->setLimits(time::Span(), -1);
    return success;
  }

  bool resolveMemoryObjects(ExecutionState &state, ref<PointerExpr> address,
                            KInstruction *target, unsigned bytes,
                            ObjectResolutionList &mayBeResolvedMemoryObjects,
                            bool &mayBeOutOfBound, bool &mayLazyInitialize,
                            bool &incomplete) {
    return executor->resolveMemoryObjects(
        state, address, target, bytes, mayBeResolvedMemoryObjects,
        mayBeOutOfBound, mayLazyInitialize, incomplete, false);
  }

  bool checkResolvedMemoryObjects(
      ExecutionState &state, ref<PointerExpr> address, unsigned bytes,
      const ObjectResolutionList &mayBeResolvedMemoryObjects,
      bool hasLazyInitialized, ObjectResolutionList &resolvedMemoryObjects,
      std::vector<ref<Expr>> &resolveConditions,
      std::vector<ref<Expr>> &unboundConditions, ref<Expr> &checkOutOfBounds,
      bool &mayBeOutOfBound) {
    return executor->checkResolvedMemoryObjects(
        state, address, bytes, mayBeResolvedMemoryObjects, hasLazyInitialized,
        resolvedMemoryObjects, resolveConditions, unboundConditions,
        checkOutOfBounds, mayBeOutOfBound);
  }

  bool makeGuard(ExecutionState &state,
                 const std::vector<ref<Expr>> &resolveConditions,
                 ref<Expr> &guard, bool &mayBeInBounds) {
    return executor->makeGuard(state, resolveConditions, guard, mayBeInBounds);
  }

  bool collectMemoryObjects(ExecutionState &state, ref<PointerExpr> address,
                            KInstruction *target, ref<Expr> &guard,
                            std::vector<ref<Expr>> &resolveConditions,
                            std::vector<ref<Expr>> &unboundConditions,
                            ObjectResolutionList &resolvedMemoryObjects);

  void collectReads(ExecutionState &state, ref<PointerExpr> address,
                    Expr::Width type,
                    const ObjectResolutionList &resolvedMemoryObjects,
                    std::vector<ref<Expr>> &results) {
    executor->collectReads(state, address, type, resolvedMemoryObjects,
                           results);
  }

  bool tryResolveAddress(ExecutionState &state, ref<PointerExpr> address,
                         std::pair<ref<Expr>, ref<Expr>> &result);

  bool tryResolveSize(ExecutionState &state, ref<PointerExpr> address,
                      std::pair<ref<Expr>, ref<Expr>> &result);

  bool tryResolveContent(
      ExecutionState &state, ref<PointerExpr> address, Expr::Width width,
      std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
          &result);

  ref<Expr> fillValue(ExecutionState &state, ref<ValueSource> valueSource,
                      ref<Expr> size) {
    return executor->fillValue(state, valueSource, size);
  }

  ref<ObjectState> fillMakeSymbolic(ExecutionState &state,
                                    ref<MakeSymbolicSource> makeSymbolicSource,
                                    ref<Expr> size) {
    return executor->fillMakeSymbolic(state, makeSymbolicSource, size);
  }

  ref<ObjectState>
  fillUninitialized(ExecutionState &state,
                    ref<UninitializedSource> uninitializedSource,
                    ref<Expr> size) {
    return executor->fillUninitialized(state, uninitializedSource, size);
  }

  ref<ObjectState> fillGlobal(ExecutionState &state,
                              ref<GlobalSource> globalSource) {
    return executor->fillGlobal(state, globalSource);
  }

  ref<ObjectState>
  fillIrreproducible(ExecutionState &state,
                     ref<IrreproducibleSource> irreproducibleSource,
                     ref<Expr> size) {
    return executor->fillIrreproducible(state, irreproducibleSource, size);
  }

  ref<ObjectState> fillConstant(ExecutionState &state,
                                ref<ConstantSource> constanSource,
                                ref<Expr> size) {
    return executor->fillConstant(state, constanSource, size);
  }

  ref<Expr> fillSymbolicSizeConstantAddress(
      ExecutionState &state,
      ref<SymbolicSizeConstantAddressSource> symbolicSizeConstantAddressSource,
      ref<Expr> arraySize, ref<Expr> size) {
    return executor->fillSymbolicSizeConstantAddress(
        state, symbolicSizeConstantAddressSource, arraySize, size);
  }

  ref<Expr> fillSizeAddressSymcretes(ExecutionState &state,
                                     ref<Expr> oldAddress, ref<Expr> newAddress,
                                     ref<Expr> size) {
    return executor->fillSizeAddressSymcretes(state, oldAddress, newAddress,
                                              size);
  }

  std::pair<ref<Expr>, ref<Expr>>
  fillLazyInitializationAddress(ExecutionState &state,
                                ref<PointerExpr> pointer);

  std::pair<ref<Expr>, ref<Expr>>
  fillLazyInitializationSize(ExecutionState &state, ref<PointerExpr> pointer);

  std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
  fillLazyInitializationContent(ExecutionState &state, ref<PointerExpr> pointer,
                                Expr::Width width);
};

class ComposeVisitor : public ExprVisitor {
private:
  using ResolutionVector = std::vector<std::pair<ref<Expr>, ref<ObjectState>>>;
  using ComposedResult = std::variant<std::monostate, ref<ObjectState>,
                                      ref<Expr>, ResolutionVector>;
  const ExecutionState &original;
  ComposeHelper helper;
  ExprOrderedSet safetyConstraints;
  std::map<const Array *, ComposedResult> composedArrays;

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
  ExprVisitor::Action visitPointer(const PointerExpr &) override;

  ref<Expr> processPointer(ref<Expr> base, ref<Expr> value);
  ref<Expr> processRead(const Array *root, const UpdateList &updates,
                        ref<Expr> index, Expr::Width width);
  ref<Expr> processSelect(ref<Expr> cond, ref<Expr> trueExpr,
                          ref<Expr> falseExpr);
  ref<ObjectState> shareUpdates(ref<ObjectState>, const UpdateList &updates);

  bool shouldCacheArray(const Array *array);

  ref<Expr> formSelectRead(ResolutionVector &rv, const UpdateList &updates,
                           ref<Expr> index, Expr::Width width);
};
} // namespace klee

#endif // KLEE_COMPOSITION_H
