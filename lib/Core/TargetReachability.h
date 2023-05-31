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

#include "DistanceCalculator.h"
#include "ExecutionState.h"

namespace klee {
class DistanceCalculator;
struct DistanceResult;
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

  using TargetHashSet =
      std::unordered_set<ref<Target>, RefTargetHash, RefTargetCmp>;
  template <class T>
  class TargetHashMap
      : public std::unordered_map<ref<Target>, T, RefTargetHash, RefTargetCmp> {
  };

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

  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);

private:
  TargetToStateUnorderedSetMap reachableStatesOfTarget;
  TargetToPotentialStateMap reachablePotentialStatesOfTarget;
  DistanceCalculator &distanceCalculator;
  TargetHashSet reachedTargets;
  std::unordered_map<ExecutionState *, TargetHashSet> reachedOnLastUpdate;
  std::unordered_map<ExecutionState *, TargetHashMap<weight_type>>
      calculatedDistance;

  void innerUpdate(ExecutionState *current,
                   const std::vector<ExecutionState *> &addedStates,
                   const std::vector<ExecutionState *> &removedStates);
  void stepTo(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);
  bool updateDistance(ExecutionState *es, const ref<Target> &target,
                      bool isStateRemoved);
  void updateDistance(ExecutionState *es, bool isStateRemoved = false);
  WeightResult tryGetWeight(ExecutionState *es, const ref<Target> &target,
                            weight_type &weight);
  bool isCalculated(ExecutionState *es, const ref<Target> &target);
  void updateConfidences(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates);
  void removeDistance(ExecutionState *es, const ref<Target> &target);
};

} // namespace klee

#endif /* KLEE_TARGET_REACHABILITY_H */