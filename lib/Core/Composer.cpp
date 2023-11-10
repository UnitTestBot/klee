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
    state.assumptions.insert(guard);
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
    state.assumptions.insert(guard);
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
    ExecutionState &state, ref<Expr> base, Expr::Width width,
    std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
        &result) {
  // rounded up
  unsigned byteWidth = width / CHAR_BIT + ((width % CHAR_BIT == 0) ? 0 : 1);
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
          state, address, target, byteWidth, mayBeResolvedMemoryObjects,
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
  collectObjectStates(state, resolvedMemoryObjects, resolveConcretizations,
                      resolvedObjectStates);

  result.first = guard;

  if (resolvedObjectStates.size() > 0) {
    Assignment concretization = computeConcretization(
        state.constraints.withAssumtions(state.assumptions), guard,
        state.queryMetaData);

    if (!concretization.isEmpty()) {
      // Update memory objects if arrays have affected them.
      executor->updateStateWithSymcretes(state, concretization);
    }
    state.assumptions.insert(guard);
  }

  for (unsigned int i = 0; i < resolvedObjectStates.size(); ++i) {
    result.second.push_back(
        std::make_pair(resolveConditions.at(i), resolvedObjectStates.at(i)));
  }
  return true;
}

std::pair<ref<Expr>, ref<Expr>> ComposeHelper::fillLazyInitializationAddress(
    ExecutionState &state, ref<Expr> pointer) {
  std::pair<ref<Expr>, ref<Expr>> result;
  if (!tryResolveAddress(state, pointer, result)) {
    return std::make_pair(
        Expr::createFalse(),
        ConstantExpr::create(0, Context::get().getPointerWidth()));
  }
  return result;
}

std::pair<ref<Expr>, ref<Expr>> ComposeHelper::fillLazyInitializationSize(
    ExecutionState &state, ref<Expr> pointer) {
  std::pair<ref<Expr>, ref<Expr>> result;
  if (!tryResolveSize(state, pointer, result)) {
    return std::make_pair(
        Expr::createFalse(),
        ConstantExpr::create(0, Context::get().getPointerWidth()));
  }
  return result;
}

std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
ComposeHelper::fillLazyInitializationContent(ExecutionState &state,
                                             ref<Expr> pointer,
                                             Expr::Width width) {
  std::pair<ref<Expr>, std::vector<std::pair<ref<Expr>, ref<ObjectState>>>>
      result;
  if (!tryResolveContent(state, pointer, width, result)) {
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

ref<ObjectState> ComposeVisitor::shareUpdates(ref<ObjectState> os,
                                              const UpdateList &updates) {
  ref<ObjectState> copy(new ObjectState(*os.get()));
  std::stack<ref<UpdateNode>> forward;

  for (auto it = updates.head; !it.isNull(); it = it->next) {
    forward.push(it);
  }

  while (!forward.empty()) {
    ref<UpdateNode> UNode = forward.top();
    forward.pop();
    ref<Expr> newIndex = visit(UNode->index);
    ref<Expr> newValue = visit(UNode->value);
    copy->write(newIndex, newValue);
  }

  return copy;
}

ref<Expr> ComposeVisitor::processRead(const Array *root,
                                      const UpdateList &updates,
                                      ref<Expr> index, Expr::Width width) {
  index = visit(index);
  auto arraySize = visit(root->getSize());

  ComposedResult composedArray;

  // First compose the array itself, the result of composition is one of:
  // 1. An Expr that expresses some value such as an llvm register.
  // 2. An ObjectState that expresses some memory object.
  // 3. A resolution list with resolution conditions that express a set
  //    of objects this array might correspond to after composition.
  if (composedArrays.count(root)) {
    composedArray = composedArrays.at(root);
  } else {
    switch (root->source->getKind()) {
    case SymbolicSource::Kind::Argument:
    case SymbolicSource::Kind::Instruction: {
      composedArray =
          helper.fillValue(state, cast<ValueSource>(root->source), arraySize);
      break;
    }
    case SymbolicSource::Kind::Global: {
      composedArray =
          helper.fillGlobal(state, cast<GlobalSource>(root->source));
      break;
    }
    case SymbolicSource::Kind::MakeSymbolic: {
      composedArray = helper.fillMakeSymbolic(
          state, cast<MakeSymbolicSource>(root->source), arraySize);
      break;
    }
    case SymbolicSource::Kind::Irreproducible: {
      composedArray = helper.fillIrreproducible(
          state, cast<IrreproducibleSource>(root->source), arraySize);
      break;
    }
    case SymbolicSource::Kind::Constant: {
      composedArray = helper.fillConstant(
          state, cast<ConstantSource>(root->source), arraySize);
      break;
    }
    // case SymbolicSource::Kind::SymbolicSizeConstant: {
    //   composedArray = helper.fillSymbolicSizeConstant(
    //       state, cast<SymbolicSizeConstantSource>(root->source), arraySize);
    //   break;
    // }
    case SymbolicSource::Kind::SymbolicSizeConstantAddress: {
      auto source = cast<SymbolicSizeConstantAddressSource>(root->source);
      auto size = visit(source->size);
      auto address = helper.fillSymbolicSizeConstantAddress(state, source,
                                                            arraySize, size);
      assert(!state.constraints.isSymcretized(address));
      auto oldAddress =
          Expr::createTempRead(root, Context::get().getPointerWidth());
      address =
          helper.fillSizeAddressSymcretes(state, oldAddress, address, size);
      composedArray = address;
      break;
    }
    case SymbolicSource::Kind::LazyInitializationAddress: {
      auto pointer =
          visit(cast<LazyInitializationSource>(root->source)->pointer);
      auto guardedAddress =
          helper.fillLazyInitializationAddress(state, pointer);
      safetyConstraints.insert(guardedAddress.first);
      composedArray = guardedAddress.second;
      break;
    }
    case SymbolicSource::Kind::LazyInitializationSize: {
      auto pointer =
          visit(cast<LazyInitializationSource>(root->source)->pointer);
      auto guardedSize = helper.fillLazyInitializationSize(state, pointer);
      safetyConstraints.insert(guardedSize.first);
      composedArray = guardedSize.second;
      break;
    }
    case SymbolicSource::Kind::LazyInitializationContent: {
      auto pointer =
          visit(cast<LazyInitializationSource>(root->source)->pointer);
      // index is not used because there are conditions composed before
      // that act as the index check
      auto guardedContent =
          helper.fillLazyInitializationContent(state, pointer, width);
      safetyConstraints.insert(guardedContent.first);
      composedArray = guardedContent.second;
      break;
    }
    default: {
      assert(0 && "not implemented");
    }
    }

    // LIContent arrays are not cached for now.
    if (shouldCacheArray(root)) {
      composedArrays.insert({root, composedArray});
    }
  }

  // Use the array composition result to form the composed
  // version of the read being composed
  switch (root->source->getKind()) {
  case SymbolicSource::Kind::Argument:
  case SymbolicSource::Kind::Instruction:
  case SymbolicSource::Kind::SymbolicSizeConstantAddress:
  case SymbolicSource::Kind::LazyInitializationAddress:
  case SymbolicSource::Kind::LazyInitializationSize: {
    assert(isa<ConstantExpr>(index));
    auto value = std::get<ref<Expr>>(composedArray);
    auto concreteIndex = cast<ConstantExpr>(index)->getZExtValue();
    return ExtractExpr::create(value, concreteIndex * 8, width);
  }

  case SymbolicSource::Kind::Global:
  case SymbolicSource::Kind::MakeSymbolic:
  case SymbolicSource::Kind::Irreproducible:
  case SymbolicSource::Kind::Constant: {
    auto os = std::get<ref<ObjectState>>(composedArray);
    os = shareUpdates(os, updates);
    return os->read(index, width);
  }

  case SymbolicSource::Kind::LazyInitializationContent: {
    auto objects = std::get<ResolutionVector>(composedArray);
    return formSelectRead(objects, updates, index, width);
  }
  default: {
    assert(0 && "not implemented");
  }
  }
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
  case PValidity::MustBeTrue:
  case PValidity::MayBeTrue: {
    return visit(trueExpr);
  }

  case PValidity::MustBeFalse:
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
  default:
    {
      assert(0);
    }
  }
}

bool ComposeVisitor::shouldCacheArray(const Array *array) {
  switch (array->source->getKind()) {
  case SymbolicSource::Kind::LazyInitializationContent: {
    return false;
  }
  default: {
    return true;
  }
  }
}

ref<Expr> ComposeVisitor::formSelectRead(ResolutionVector &rv,
                                         const UpdateList &updates,
                                         ref<Expr> index, Expr::Width width) {
  std::vector<ref<Expr>> results;
  std::vector<ref<Expr>> guards;
  for (unsigned int i = 0; i < rv.size(); ++i) {
    ref<Expr> guard = rv[i].first;
    ref<ObjectState> os = rv[i].second;
    os = shareUpdates(os, updates);

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
