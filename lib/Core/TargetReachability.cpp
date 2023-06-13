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

void collectTargets(ExecutionState &es,
                    TargetReachability::TargetHashSet &targets,
                    ref<TargetForest::History> &history) {
  targets.clear();

  history = es.targetForest.getHistory();
  for (auto &t : *es.targetForest.getTargets()) {
    targets.insert(t.first);
  }
}

void collectTargets(ExecutionState *current,
                    const std::vector<ExecutionState *> &addedStates,
                    const std::vector<ExecutionState *> &removedStates,
                    TargetReachability::TargetHashSet ExecutionState::*targets,
                    ref<TargetForest::History> ExecutionState::*history) {
  if (current) {
    collectTargets(*current, current->*targets, current->*history);
  }

  for (const auto state : addedStates) {
    collectTargets(*state, state->*targets, state->*history);
  }

  for (const auto state : removedStates) {
    collectTargets(*state, state->*targets, state->*history);
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

void TargetReachability::addReachableStateForTarget(ExecutionState &es,
                                                    ref<Target> target) {
  reachableStatesOfTarget[target].insert(es.getID());
}

void TargetReachability::updateReachibilityOfStateForTarget(
    ExecutionState &es, ref<Target> target) {
  auto distRes = distanceCalculator.getDistance(es, target);
  if (distRes.result != Miss) {
    reachableStatesOfTarget[target].insert(es.getID());
  }
}

void TargetReachability::updateReachabilityOfSpeculativeStateForTarget(
    uint32_t stateId, KInstruction *pc, KInstruction *prevPC,
    KInstruction *initPC, const ExecutionState::stack_ty &stack,
    ReachWithError error, ref<Target> target) {
  auto distRes = distanceCalculator.getDistance(
      pc, prevPC, initPC, stack, error, target);
  if (distRes.result != Miss) {
    reachableSpeculativeStatesOfTarget[target].insert(stateId);
  }
}

void TargetReachability::updateConfidencesInState(ExecutionState &es) {
  es.targetForest.divideConfidenceBy(reachableStatesOfTarget,
                                      reachableSpeculativeStatesOfTarget);
}

void TargetReachability::clear() {
  reachedOnLastUpdate.clear();
  reachableStatesOfTarget.clear();
  reachableSpeculativeStatesOfTarget.clear();
}

void TargetReachability::updateConfidences(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  if (current) {
    updateConfidencesInState(*current);
  }
  for (const auto state : addedStates) {
    updateConfidencesInState(*state);
  }
}

void TargetReachability::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  innerUpdate(current, addedStates, removedStates);
  if (guidance == Coverage) {
    handleTargetlessStates(current, addedStates);
  }
  updateConfidences(current, addedStates, removedStates);
  stepTo(current, addedStates, removedStates);
  clear();
}

weight_type TargetReachability::getDistance(ExecutionState &es,
                                            ref<Target> target) {
  assert(calculatedDistance.count(&es) != 0 &&
         calculatedDistance[&es].count(target) != 0);
  return calculatedDistance[&es][target];
}

void TargetReachability::innerUpdate(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  collectTargets(current, addedStates, removedStates,
                 &ExecutionState::prevTargets, &ExecutionState::prevHistory);

  if (current) {
    updateDistance(*current);
  }

  for (const auto state : removedStates) {
    updateDistance(*state, true);
  }

  for (const auto state : addedStates) {
    updateDistance(*state);
  }
}

void TargetReachability::stepTo(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  for (const auto &stateAndTargets : reachedOnLastUpdate) {
    const auto state = stateAndTargets.first;
    for (const auto &t : stateAndTargets.second) {
      if (t->shouldFailOnThisTarget()) {
        reachedTargets.insert(t);
      }
      state->targetForest.stepTo(t);
    }

    for (const auto &targets : *state->targetForest.getTargets()) {
      const auto target = targets.first;
      if (calculatedDistance[state].count(target) == 0) {
        updateDistance(*state, target, false);
      }
    }
  }

  collectTargets(current, addedStates, removedStates,
                 &ExecutionState::currTargets, &ExecutionState::currHistory);
}

bool TargetReachability::updateDistance(ExecutionState &es, ref<Target> target,
                                        bool isStateRemoved) {
  weight_type weight = 0;
  bool canReach = false;

  if (isStateRemoved && target->atReturn() &&
      !target->shouldFailOnThisTarget() &&
      es.prevPC == target->getBlock()->getLastInstruction()) {
    canReach = true;
    return canReach;
  }

  if (reachedTargets.count(target) == 0) {
    switch (tryGetWeight(es, target, weight)) {
    case Continue:
      if (!isStateRemoved) {
        calculatedDistance[&es][target] = weight;
      }
      canReach = true;
      break;
    case Done:
      if (reachedOnLastUpdate[&es].count(target) == 0) {
        if (!isStateRemoved) {
          reachedOnLastUpdate[&es].insert(target);
        }
        removeDistance(es, target);
      } else {
        calculatedDistance[&es][target] = weight;
      }
      canReach = true;
      break;
    case Miss:
      es.targetForest.remove(target);
      removeDistance(es, target);
      break;
    }
  } else {
    canReach = true;
    if (!isStateRemoved) {
      es.targetForest.remove(target);
    }
  }

  return canReach;
}

void TargetReachability::updateDistance(ExecutionState &es,
                                        bool isStateRemoved) {
  for (const auto &t : es.prevTargets) {
    bool canReach = updateDistance(es, t, isStateRemoved);
    if (canReach) {
      addReachableStateForTarget(es, t);
    }
  }

  if (isStateRemoved) {
    calculatedDistance.erase(&es);
  } else {
    for (const auto &t : reachedTargets) {
      es.targetForest.block(t);
    }
  }
}

WeightResult TargetReachability::tryGetWeight(ExecutionState &es,
                                              ref<Target> target,
                                              weight_type &weight) {
  BasicBlock *bb = es.getPCBlock();
  KBlock *kb = es.pc->parent->parent->blockMap[bb];
  KInstruction *ki = es.pc;
  if (!target->atReturn() && !target->shouldFailOnThisTarget() && kb->numInstructions &&
      !isa<KCallBlock>(kb) && kb->getFirstInstruction() != ki &&
      isCalculated(es, target)) {
    weight = getDistance(es, target);
    if (weight != UINT_MAX) {
      return Continue;
    } else {
      return Done;
    }
  }

  auto distRes = distanceCalculator.getDistance(es, target);
  weight = ulog2(distRes.weight + es.steppedMemoryInstructions); // [0, 32]
  if (!distRes.isInsideFunction) {
    weight += 32; // [32, 64]
  }

  return distRes.result;
}

bool TargetReachability::isCalculated(ExecutionState &es, ref<Target> target) {
  if (calculatedDistance.count(&es) != 0) {
    return calculatedDistance[&es].count(target) != 0;
  }

  return false;
}

void TargetReachability::removeDistance(ExecutionState &es,
                                        ref<Target> target) {
  if (calculatedDistance.count(&es) != 0) {
    calculatedDistance[&es].erase(target);
    if (calculatedDistance[&es].size() == 0) {
      calculatedDistance.erase(&es);
    }
  }
}

void TargetReachability::updateTargetlessState(ExecutionState &es) {
  if (es.isStuck()) {
    ref<Target> target(stateHistory.calculate(es));
    if (target) {
      es.targetForest.add(target);
      updateDistance(es, target, false);
    }
  }
}

void TargetReachability::handleTargetlessStates(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates) {
  if (current && current->targetForest.getTargets()->empty()) {
    updateTargetlessState(*current);
  }

  for (const auto state : addedStates) {
    if (state->targetForest.getTargets()->empty()) {
      updateTargetlessState(*state);
    }
  }
}
