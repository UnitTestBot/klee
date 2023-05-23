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
#include "klee/Module/Target.h"

using namespace klee;

void TargetReachability::addReachableStateForTarget(ExecutionState *es,
                                                    ref<Target> target) {
  if (es) {
    reachableStatesOfTarget[target].insert(es);
  }
}

void TargetReachability::updateReachibilityOfStateForTarget(
    ExecutionState *es, ref<Target> target) {
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
    llvm::BasicBlock *prevPCBlock, ref<Target> target) {
  auto distRes = distanceCalculator.getDistance(
      pc, prevPC, initPC, stack, error, pcBlock, prevPCBlock, target.get());
  if (distRes.result != Miss) {
    reachablePotentialStatesOfTarget[target].insert(stateId);
  }
}

void TargetReachability::updateConfidencesInState(ExecutionState *es) {
  if (es) {
    es->targetForest.divideConfidenceBy(reachableStatesOfTarget);
  }
}

void TargetReachability::clear() {
  reachableStatesOfTarget.clear();
  reachablePotentialStatesOfTarget.clear();
}
