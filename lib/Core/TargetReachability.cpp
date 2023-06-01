//===-- TargetReachability.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetReachability.h"
#include "DistanceCalculator.h"
#include "TargetCalculator.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/Target.h"
#include "klee/Support/ErrorHandling.h"

using namespace klee;
using namespace llvm;

namespace {

void collectTargets(ExecutionState *es,
                    TargetReachability::TargetHashSet &targets) {
  targets.clear();

  for (auto &t : *es->targetForest.getTargets()) {
    targets.insert(t.first);
  }
}

void collectTargets(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates,
    TargetReachability::TargetHashSet ExecutionState::*targets) {
  if (current) {
    collectTargets(current, current->*targets);
  }

  for (const auto state : addedStates) {
    collectTargets(state, state->*targets);
  }

  for (const auto state : removedStates) {
    collectTargets(state, state->*targets);
  }
}

unsigned int ulog2(unsigned int val) {
  if (val == 0)
    return UINT_MAX;
  if (val == 1)
    return 0;
  unsigned int ret = 0;
  while (val > 1) {
    val >>= 1;
    ret++;
  }
  return ret;
}

} // anonymous namespace

void TargetReachability::addReachableStateForTarget(ExecutionState *es,
                                                    const ref<Target> &target) {
  if (es) {
    reachableStatesOfTarget[target].insert(es);
  }
}

void TargetReachability::updateReachibilityOfStateForTarget(
    ExecutionState *es, const ref<Target> &target) {
  if (es) {
    auto distRes = distanceCalculator.getDistance(es, target.get());
    if (distRes.result != Miss) {
      reachableStatesOfTarget[target].insert(es);
    }
  }
}

void TargetReachability::updateReachabilityOfPotentialStateForTarget(
    unsigned stateId, KInstIterator pc, KInstIterator prevPC,
    KInstIterator initPC, const ExecutionState::stack_ty &stack,
    ReachWithError error, llvm::BasicBlock *pcBlock,
    llvm::BasicBlock *prevPCBlock, const ref<Target> &target) {
  auto distRes = distanceCalculator.getDistance(
      pc, prevPC, initPC, stack, error, pcBlock, prevPCBlock, target.get());
  if (distRes.result != Miss) {
    reachablePotentialStatesOfTarget[target].insert(stateId);
  }
}

void TargetReachability::updateConfidencesInState(ExecutionState *es) {
  if (es) {
    es->targetForest.divideConfidenceBy(reachableStatesOfTarget,
                                        reachablePotentialStatesOfTarget);
  }
}

void TargetReachability::clear() {
  reachedOnLastUpdate.clear();
  reachableStatesOfTarget.clear();
  reachablePotentialStatesOfTarget.clear();
}

void TargetReachability::updateConfidences(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  updateConfidencesInState(current);
  for (const auto state : addedStates) {
    updateConfidencesInState(state);
  }
}

void TargetReachability::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  innerUpdate(current, addedStates, removedStates);
  if (isCoverageGuided) {
    handleTargetlessStates(current, addedStates);
  }
  updateConfidences(current, addedStates, removedStates);
  stepTo(current, addedStates, removedStates);
  clear();
}

void TargetReachability::innerUpdate(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  collectTargets(current, addedStates, removedStates,
                 &ExecutionState::prevTargets);

  if (current) {
    updateDistance(current);
  }

  for (const auto state : addedStates) {
    updateDistance(state);
  }

  for (const auto state : removedStates) {
    updateDistance(state, true);
  }
}

void TargetReachability::stepTo(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  for (const auto &p : reachedOnLastUpdate) {
    const auto state = p.first;
    for (const auto &t : p.second) {
      if (t->shouldFailOnThisTarget()) {
        reachedTargets.insert(t);
      }
      state->targetForest.stepTo(t);
    }
  }

  collectTargets(current, addedStates, removedStates,
                 &ExecutionState::currTargets);
}

bool TargetReachability::updateDistance(ExecutionState *es,
                                        const ref<Target> &target,
                                        bool isStateRemoved) {
  weight_type weight = 0;
  bool canReach = false;

  if (isStateRemoved && target->atReturn() &&
      !target->shouldFailOnThisTarget() &&
      es->prevPC == target->getBlock()->getLastInstruction()) {
    canReach = true;
    reachedOnLastUpdate[es].insert(target);
    return canReach;
  }

  if (reachedTargets.count(target) == 0) {
    switch (tryGetWeight(es, target, weight)) {
    case Continue:
      if (!isStateRemoved) {
        calculatedDistance[es][target] = weight;
      }
      canReach = true;
      break;
    case Done:
      reachedOnLastUpdate[es].insert(target);
      removeDistance(es, target);
      canReach = true;
      break;
    case Miss:
      es->targetForest.remove(target);
      removeDistance(es, target);
      break;
    }
  } else {
    canReach = true;
    if (!isStateRemoved) {
      es->targetForest.remove(target);
    }
  }

  return canReach;
}

void TargetReachability::updateDistance(ExecutionState *es,
                                        bool isStateRemoved) {
  for (const auto &t : es->prevTargets) {
    bool canReach = updateDistance(es, t, isStateRemoved);
    if (canReach) {
      addReachableStateForTarget(es, t);
    }
  }

  if (isStateRemoved) {
    calculatedDistance.erase(es);
  } else {
    for (const auto &t : reachedTargets) {
      es->targetForest.block(t);
    }
  }
}

WeightResult TargetReachability::tryGetWeight(ExecutionState *es,
                                              const ref<Target> &target,
                                              weight_type &weight) {
  if (target->atReturn() && !target->shouldFailOnThisTarget()) {
    if (es->prevPC->parent == target->getBlock() &&
        es->prevPC == target->getBlock()->getLastInstruction()) {
      return Done;
    } else if (es->pc->parent == target->getBlock()) {
      weight = 0;
      return Continue;
    }
  }

  if (target->shouldFailOnThisTarget() && target->isTheSameAsIn(es->prevPC) &&
      target->isThatError(es->error)) {
    return Done;
  }

  BasicBlock *bb = es->getPCBlock();
  KBlock *kb = es->pc->parent->parent->blockMap[bb];
  KInstruction *ki = es->pc;
  if (!target->shouldFailOnThisTarget() && kb->numInstructions &&
      !isa<KCallBlock>(kb) && kb->getFirstInstruction() != ki &&
      isCalculated(es, target)) {
    return Continue;
  }

  auto distRes = distanceCalculator.getDistance(es, target.get());
  weight = ulog2(distRes.weight + es->steppedMemoryInstructions); // [0, 32]
  if (!distRes.isInsideFunction) {
    weight += 32; // [32, 64]
  }

  return distRes.result;
}

bool TargetReachability::isCalculated(ExecutionState *es,
                                      const ref<Target> &target) {
  if (calculatedDistance.count(es) != 0) {
    return calculatedDistance[es].count(target) != 0;
  }

  return false;
}

void TargetReachability::removeDistance(ExecutionState *es,
                                        const ref<Target> &target) {
  if (calculatedDistance.count(es) != 0) {
    calculatedDistance[es].erase(target);
    if (calculatedDistance[es].size() == 0) {
      calculatedDistance.erase(es);
    }
  }
}

void TargetReachability::updateTargetlessState(ExecutionState *es) {
  if (es->isStuck()) {
    ref<Target> target(stateHistory.calculate(*es));
    if (target) {
      es->targetForest.add(target);
    }
  }
}

void TargetReachability::handleTargetlessStates(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates) {
  if (current->targetForest.getTargets()->size() == 0) {
    updateTargetlessState(current);
  }

  for (const auto state : addedStates) {
    if (state->targetForest.getTargets()->size() == 0) {
      updateTargetlessState(state);
    }
  }
}
