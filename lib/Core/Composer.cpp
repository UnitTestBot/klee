#include "Composer.h"

#include "klee/Expr/ArrayExprVisitor.h"

#include "TypeManager.h"

using namespace klee;
using namespace llvm;

bool ComposeHelper::collectMemoryObjects(
    ExecutionState &state, ref<Expr> address, KType *targetType,
    KInstruction *target, ref<Expr> &guard,
    std::vector<ref<Expr>> &resolveConditions,
    std::vector<ref<Expr>> &unboundConditions,
    std::vector<IDType> &resolvedMemoryObjects) {
  bool mayBeOutOfBound = true;
  bool hasLazyInitialized = false;
  bool incomplete = false;
  std::vector<IDType> mayBeResolvedMemoryObjects;

  if (!resolveMemoryObjects(state, address, targetType, target, 0,
                            mayBeResolvedMemoryObjects, mayBeOutOfBound,
                            hasLazyInitialized, incomplete)) {
    return false;
  }

  ref<Expr> checkOutOfBounds;
  if (!checkResolvedMemoryObjects(
          state, address, target, 0, mayBeResolvedMemoryObjects,
          hasLazyInitialized, resolvedMemoryObjects, resolveConditions,
          unboundConditions, checkOutOfBounds, mayBeOutOfBound)) {
    return false;
  }

  bool mayBeInBounds;
  if (!makeGuard(state, resolveConditions, unboundConditions, checkOutOfBounds,
                 hasLazyInitialized, guard, mayBeInBounds)) {
    return false;
  }
  return true;
}

bool ComposeHelper::tryResolveAddress(ExecutionState &state, ref<Expr> address,
                                      std::pair<ref<Expr>, ref<Expr>> &result) {
  ref<Expr> guard;
  std::vector<ref<Expr>> resolveConditions;
  std::vector<ref<Expr>> unboundConditions;
  std::vector<IDType> resolvedMemoryObjects;
  KType *targetType = executor->typeSystemManager->getUnknownType();
  KInstruction *target = nullptr;

  if (!collectMemoryObjects(state, address, targetType, target, guard,
                            resolveConditions, unboundConditions,
                            resolvedMemoryObjects)) {
    return false;
  }

  result.first = guard;
  if (resolvedMemoryObjects.size() > 0) {
    Assignment concretization = computeConcretization(
        state.constraints.withAssumtions(state.assumptions), guard,
        state.queryMetaData);

    if (!concretization.isEmpty()) {
      // Update memory objects if arrays have affected them.
      executor->updateStateWithSymcretes(state, concretization);
    }
    ref<Expr> resultAddress =
        state.addressSpace
            .findObject(resolvedMemoryObjects.at(resolveConditions.size() - 1))
            .first->getBaseExpr();
    for (unsigned int i = 0; i < resolveConditions.size(); ++i) {
      unsigned int index = resolveConditions.size() - 1 - i;
      const MemoryObject *mo =
          state.addressSpace.findObject(resolvedMemoryObjects.at(index)).first;
      resultAddress = SelectExpr::create(resolveConditions[index],
                                         mo->getBaseExpr(), resultAddress);
    }
    result.second = resultAddress;
  } else {
    result.second = Expr::createPointer(0);
  }
  return true;
}

bool ComposeHelper::tryResolveSize(ExecutionState &state, ref<Expr> address,
                                   std::pair<ref<Expr>, ref<Expr>> &result) {
  ref<Expr> guard;
  std::vector<ref<Expr>> resolveConditions;
  std::vector<ref<Expr>> unboundConditions;
  std::vector<IDType> resolvedMemoryObjects;
  KType *targetType = executor->typeSystemManager->getUnknownType();
  KInstruction *target = nullptr;

  if (!collectMemoryObjects(state, address, targetType, target, guard,
                            resolveConditions, unboundConditions,
                            resolvedMemoryObjects)) {
    return false;
  }

  result.first = guard;
  if (resolvedMemoryObjects.size() > 0) {
    Assignment concretization = computeConcretization(
        state.constraints.withAssumtions(state.assumptions), guard,
        state.queryMetaData);

    if (!concretization.isEmpty()) {
      // Update memory objects if arrays have affected them.
      executor->updateStateWithSymcretes(state, concretization);
    }
    ref<Expr> resultSize =
        state.addressSpace
            .findObject(resolvedMemoryObjects.at(resolveConditions.size() - 1))
            .first->getSizeExpr();
    for (unsigned int i = 0; i < resolveConditions.size(); ++i) {
      unsigned int index = resolveConditions.size() - 1 - i;
      const MemoryObject *mo =
          state.addressSpace.findObject(resolvedMemoryObjects.at(index)).first;
      resultSize = SelectExpr::create(resolveConditions[index],
                                      mo->getSizeExpr(), resultSize);
    }
    result.second = resultSize;
  } else {
    result.second = Expr::createPointer(0);
  }
  return true;
}

bool ComposeHelper::tryResolveContent(
    ExecutionState &state, ref<Expr> base, ref<Expr> offset, Expr::Width type,
    unsigned size,
    std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
        &result) {
  bool mayBeOutOfBound = true;
  bool hasLazyInitialized = false;
  bool incomplete = false;
  std::vector<IDType> mayBeResolvedMemoryObjects;
  KType *baseType = executor->typeSystemManager->getUnknownType();
  KInstruction *target = nullptr;

  if (!resolveMemoryObjects(state, base, baseType, target, 0,
                            mayBeResolvedMemoryObjects, mayBeOutOfBound,
                            hasLazyInitialized, incomplete)) {
    return false;
  }

  ref<Expr> checkOutOfBounds;
  std::vector<ref<Expr>> resolveConditions;
  std::vector<ref<Expr>> unboundConditions;
  std::vector<IDType> resolvedMemoryObjects;
  ref<Expr> address = base;

  if (!checkResolvedMemoryObjects(
          state, address, target, size, mayBeResolvedMemoryObjects,
          hasLazyInitialized, resolvedMemoryObjects, resolveConditions,
          unboundConditions, checkOutOfBounds, mayBeOutOfBound)) {
    return false;
  }

  ref<Expr> guard;
  std::vector<Assignment> resolveConcretizations;
  bool mayBeInBounds;

  if (!collectConcretizations(state, resolveConditions, unboundConditions,
                              resolvedMemoryObjects, checkOutOfBounds,
                              hasLazyInitialized, guard, resolveConcretizations,
                              mayBeInBounds)) {
    return false;
  }

  std::vector<ref<ObjectState>> resolvedObjectStates;
  collectObjectStates(state, address, type, size, resolvedMemoryObjects,
                      resolveConcretizations, resolvedObjectStates);

  result.first = guard;

  if (resolvedObjectStates.size() > 0) {
    Assignment concretization = computeConcretization(
        state.constraints.withAssumtions(state.assumptions), guard,
        state.queryMetaData);

    if (!concretization.isEmpty()) {
      // Update memory objects if arrays have affected them.
      executor->updateStateWithSymcretes(state, concretization);
    }
  }

  for (unsigned int i = 0; i < resolvedObjectStates.size(); ++i) {
    result.second.push_back(
        std::make_pair(resolveConditions.at(i), resolvedObjectStates.at(i)));
  }
  return true;
}

std::pair<ref<Expr>, ref<Expr>> ComposeHelper::fillLazyInitializationAddress(
    ExecutionState &state,
    ref<LazyInitializationAddressSource> lazyInitializationAddressSource,
    ref<Expr> pointer, Expr::Width width) {
  std::pair<ref<Expr>, ref<Expr>> result;
  if (!tryResolveAddress(state, pointer, result)) {
    return std::make_pair(Expr::createFalse(), ConstantExpr::create(0, width));
  }
  return result;
}

std::pair<ref<Expr>, ref<Expr>> ComposeHelper::fillLazyInitializationSize(
    ExecutionState &state,
    ref<LazyInitializationSizeSource> lazyInitializationSizeSource,
    ref<Expr> pointer, Expr::Width width) {
  std::pair<ref<Expr>, ref<Expr>> result;
  if (!tryResolveSize(state, pointer, result)) {
    return std::make_pair(Expr::createFalse(), ConstantExpr::create(0, width));
  }
  return result;
}

std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
ComposeHelper::fillLazyInitializationContent(
    ExecutionState &state,
    ref<LazyInitializationContentSource> lazyInitializationContentSource,
    ref<Expr> pointer, unsigned concreteSize, ref<Expr> offset,
    Expr::Width width) {
  std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
      result;
  if (!tryResolveContent(state, pointer, offset, width, concreteSize, result)) {
    return std::make_pair(
        Expr::createFalse(),
        std::vector<std::pair<ref<Expr>, ref<ObjectState>>>());
  }
  return result;
}

ExprVisitor::Action ComposeVisitor::visitRead(const ReadExpr &read) {
  return Action::changeTo(processRead(read.updates.root, read.updates,
                                      read.index, read.getWidth()));
}

ExprVisitor::Action ComposeVisitor::visitConcat(const ConcatExpr &concat) {
  const ReadExpr *base = ArrayExprHelper::hasOrderedReads(concat);
  if (base) {
    return Action::changeTo(processRead(base->updates.root, base->updates,
                                        base->index, concat.getWidth()));
  } else {
    return Action::doChildren();
  }
}

ExprVisitor::Action ComposeVisitor::visitSelect(const SelectExpr &select) {
  return Action::changeTo(
      processSelect(select.cond, select.trueExpr, select.falseExpr));
}

void ComposeVisitor::shareUpdates(ref<ObjectState> os,
                                  const UpdateList &updates) {
  std::stack<ref<UpdateNode>> forward;

  for (auto it = updates.head; !it.isNull(); it = it->next) {
    forward.push(it);
  }

  while (!forward.empty()) {
    ref<UpdateNode> UNode = forward.top();
    forward.pop();
    ref<Expr> newIndex = visit(UNode->index);
    ref<Expr> newValue = visit(UNode->value);
    os->write(newIndex, newValue);
  }
}

ref<Expr> ComposeVisitor::processRead(const Array *root,
                                      const UpdateList &updates,
                                      ref<Expr> index, Expr::Width width) {
  index = visit(index);
  ref<Expr> size = visit(root->getSize());
  Expr::Width concreteSizeInBits = 0;
  concreteSizeInBits = width;
  unsigned concreteSize = concreteSizeInBits / CHAR_BIT;
  concreteSize += (concreteSizeInBits % CHAR_BIT == 0) ? 0 : 1;
  ref<Expr> res;
  switch (root->source->getKind()) {
  case SymbolicSource::Kind::Argument:
  case SymbolicSource::Kind::Instruction: {
    assert(updates.getSize() == 0);
    auto value = helper.fillValue(state, cast<ValueSource>(root->source), size);
    assert(isa<ConstantExpr>(index));
    auto concreteIndex = dyn_cast<ConstantExpr>(index)->getZExtValue();
    return ExtractExpr::create(value, concreteIndex * 8, width);
  }

  case SymbolicSource::Kind::Global: {
    ref<ObjectState> os =
        helper.fillGlobal(state, cast<GlobalSource>(root->source));
    shareUpdates(os, updates);
    return os->read(index, width);
  }

  case SymbolicSource::Kind::MakeSymbolic: {
    ref<ObjectState> os = helper.fillMakeSymbolic(
        state, cast<MakeSymbolicSource>(root->source), size, concreteSize);
    shareUpdates(os, updates);
    return os->read(index, width);
  }
  case SymbolicSource::Kind::Irreproducible: {
    ref<ObjectState> os = helper.fillIrreproducible(
        state, cast<IrreproducibleSource>(root->source), size, concreteSize);
    shareUpdates(os, updates);
    return os->read(index, width);
  }
  case SymbolicSource::Kind::Constant: {
    ref<ObjectState> os =
        helper.fillConstant(state, cast<ConstantSource>(root->source), size);
    shareUpdates(os, updates);
    return os->read(index, width);
  }
  case SymbolicSource::Kind::SymbolicSizeConstantAddress: {
    assert(updates.getSize() == 0);
    ref<Expr> address = helper.fillSymbolicSizeConstantAddress(
        state, cast<SymbolicSizeConstantAddressSource>(root->source), size,
        width);
    if (!state.constraints.isSymcretized(address)) {
      std::pair<ref<Expr>, ref<Expr>> sizeAddress =
          helper.getSymbolicSizeConstantSizeAddressPair(
              state, cast<SymbolicSizeConstantAddressSource>(root->source),
              size, width);
      ref<Expr> oldAddress = Expr::createTempRead(root, width);
      address = helper.fillSizeAddressSymcretes(
          state, oldAddress, sizeAddress.first, sizeAddress.second);
    }
    return address;
  }
  case SymbolicSource::Kind::LazyInitializationAddress: {
    assert(updates.getSize() == 0);
    ref<Expr> pointer =
        visit(cast<LazyInitializationSource>(root->source)->pointer);
    std::pair<ref<Expr>, ref<Expr>> guardedAddress =
        helper.fillLazyInitializationAddress(
            state, cast<LazyInitializationAddressSource>(root->source), pointer,
            width);
    safetyConstraints.insert(guardedAddress.first);
    return guardedAddress.second;
  }
  case SymbolicSource::Kind::LazyInitializationSize: {
    assert(updates.getSize() == 0);
    ref<Expr> pointer =
        visit(cast<LazyInitializationSource>(root->source)->pointer);
    std::pair<ref<Expr>, ref<Expr>> guardedSize =
        helper.fillLazyInitializationSize(
            state, cast<LazyInitializationSizeSource>(root->source), pointer,
            width);
    safetyConstraints.insert(guardedSize.first);
    return guardedSize.second;
  }
  case SymbolicSource::Kind::LazyInitializationContent: {
    ref<Expr> pointer =
        visit(cast<LazyInitializationSource>(root->source)->pointer);
    std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
        guardedContent = helper.fillLazyInitializationContent(
            state, cast<LazyInitializationContentSource>(root->source), pointer,
            concreteSize, Expr::createZExtToPointerWidth(index), width);
    safetyConstraints.insert(guardedContent.first);

    std::vector<ref<Expr>> results;
    std::vector<ref<Expr>> guards;
    for (unsigned int i = 0; i < guardedContent.second.size(); ++i) {
      ref<Expr> guard = guardedContent.second[i].first;
      ref<ObjectState> os = guardedContent.second[i].second;
      shareUpdates(os, updates);

      ref<Expr> result = os->read(index, width);
      results.push_back(result);
      guards.push_back(guard);
    }

    ref<Expr> result;
    if (results.size() > 0) {
      result = results[guards.size() - 1];
      for (unsigned int i = 0; i < guards.size(); ++i) {
        unsigned int index = guards.size() - 1 - i;
        result = SelectExpr::create(guards[index], results[index], result);
      }
    } else {
      result = ConstantExpr::create(0, width);
    }

    return result;
  }

  default:
    assert(0 && "not implemented");
  }
  return res;
}

ref<Expr> ComposeVisitor::processSelect(ref<Expr> cond, ref<Expr> trueExpr,
                                        ref<Expr> falseExpr) {
  cond = visit(cond);
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(cond)) {
    return CE->isTrue() ? visit(trueExpr) : visit(falseExpr);
  }
  PartialValidity res;
  if (!helper.evaluate(state, cond, res, state.queryMetaData)) {
    safetyConstraints.insert(Expr::createFalse());
    return ConstantExpr::create(0, trueExpr->getWidth());
  }
  switch (res) {
  case PValidity::MustBeTrue: {
    return visit(trueExpr);
  }
  case PValidity::MayBeTrue: {
    return visit(trueExpr);
  }

  case PValidity::MustBeFalse: {
    return visit(falseExpr);
  }
  case PValidity::MayBeFalse: {
    return visit(falseExpr);
  }

  case PValidity::TrueOrFalse: {
    ExprHashSet savedAssumtions = state.assumptions;

    ExprOrderedSet savedSafetyConstraints = safetyConstraints;
    safetyConstraints.clear();

    {
      Assignment concretization = helper.computeConcretization(
          state.constraints.withAssumtions(state.assumptions), cond,
          state.queryMetaData);

      if (!concretization.isEmpty()) {
        // Update memory objects if arrays have affected them.
        Assignment delta =
            state.constraints.cs().concretization().diffWith(concretization);
        helper.updateStateWithSymcretes(state, delta);
        state.constraints.rewriteConcretization(delta);
      }
    }

    state.assumptions.insert(cond);
    visited.pushFrame();
    trueExpr = visit(trueExpr);
    visited.popFrame();
    state.assumptions = savedAssumtions;

    ExprOrderedSet trueSafetyConstraints = safetyConstraints;
    safetyConstraints.clear();
    ref<Expr> trueSafe = Expr::createTrue();
    for (auto sc : trueSafetyConstraints) {
      trueSafe = AndExpr::create(trueSafe, sc);
    }

    {
      Assignment concretization = helper.computeConcretization(
          state.constraints.withAssumtions(state.assumptions),
          Expr::createIsZero(cond), state.queryMetaData);

      if (!concretization.isEmpty()) {
        // Update memory objects if arrays have affected them.
        Assignment delta =
            state.constraints.cs().concretization().diffWith(concretization);
        helper.updateStateWithSymcretes(state, delta);
        state.constraints.rewriteConcretization(delta);
      }
    }

    state.assumptions.insert(Expr::createIsZero(cond));
    visited.pushFrame();
    falseExpr = visit(falseExpr);
    visited.popFrame();
    state.assumptions = savedAssumtions;

    ExprOrderedSet falseSafetyConstraints = safetyConstraints;
    safetyConstraints.clear();
    ref<Expr> falseSafe = Expr::createTrue();
    for (auto sc : falseSafetyConstraints) {
      falseSafe = AndExpr::create(falseSafe, sc);
    }

    safetyConstraints = savedSafetyConstraints;
    safetyConstraints.insert(OrExpr::create(trueSafe, falseSafe));

    ref<Expr> result = SelectExpr::create(cond, trueExpr, falseExpr);
    return result;
  }
  default: {
    assert(0);
  }
  }
}
