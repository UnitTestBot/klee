//===-- TargetReachability.h ------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGET_REACHABILITY_H
#define KLEE_TARGET_REACHABILITY_H

#include <unordered_map>
#include <unordered_set>

#include "ExecutionState.h"

namespace klee {
class DistanceCalculator;
struct Target;
class ExecutionState;

class TargetReachability {
public:
  explicit TargetReachability(DistanceCalculator &distanceCalculator_)
      : distanceCalculator(distanceCalculator_) {}

  using TargetToStateUnorderedSetMap =
      std::unordered_map<ref<Target>, std::unordered_set<ExecutionState *>,
                         RefTargetHash, RefTargetCmp>;
  using TargetToPotentialStateMap =
      std::unordered_map<ref<Target>, std::unordered_set<unsigned>,
                         RefTargetHash, RefTargetCmp>;

  void addReachableStateForTarget(ExecutionState *es,
                                  const ref<Target> &target);
  void updateReachibilityOfStateForTarget(ExecutionState *es,
                                          const ref<Target> &target);
  void updateReachabilityOfPotentialStateForTarget(
      unsigned stateId, KInstIterator pc, KInstIterator prevPC,
      KInstIterator initPC, const ExecutionState::stack_ty &stack,
      ReachWithError error, llvm::BasicBlock *pcBlock,
      llvm::BasicBlock *prevPCBlock, const ref<Target> &target);
  void updateConfidencesInState(ExecutionState *es);
  void clear();

private:
  TargetToStateUnorderedSetMap reachableStatesOfTarget;
  TargetToPotentialStateMap reachablePotentialStatesOfTarget;
  DistanceCalculator &distanceCalculator;
};

} // namespace klee

#endif /* KLEE_TARGET_REACHABILITY_H */