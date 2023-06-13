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
class TargetCalculator;
class ExecutionState;

class TargetReachability {
public:
  enum Guidance { Error, Coverage };

  explicit TargetReachability(DistanceCalculator &distanceCalculator_,
                              Guidance guidance_,
                              TargetCalculator &stateHistory_)
      : distanceCalculator(distanceCalculator_), stateHistory(stateHistory_),
        guidance(guidance_) {}

  using TargetToStateUnorderedSetMap =
      std::unordered_map<ref<Target>, std::unordered_set<uint32_t>,
                         RefTargetHash, RefTargetCmp>;

  using TargetHashSet =
      std::unordered_set<ref<Target>, RefTargetHash, RefTargetCmp>;
  template <class T>
  class TargetHashMap
      : public std::unordered_map<ref<Target>, T, RefTargetHash, RefTargetCmp> {
  };

  void addReachableStateForTarget(ExecutionState *es, ref<Target> target);
  void updateReachibilityOfStateForTarget(ExecutionState *es,
                                          ref<Target> target);
  void updateReachabilityOfSpeculativeStateForTarget(
      uint32_t stateId, KInstIterator pc, KInstIterator prevPC,
      KInstIterator initPC, const ExecutionState::stack_ty &stack,
      ReachWithError error, llvm::BasicBlock *pcBlock,
      llvm::BasicBlock *prevPCBlock, ref<Target> target);
  void updateConfidencesInState(ExecutionState *es);
  void clear();

  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);

  weight_type getDistance(ExecutionState *es, ref<Target> target);

private:
  TargetToStateUnorderedSetMap reachableStatesOfTarget;
  TargetToStateUnorderedSetMap reachableSpeculativeStatesOfTarget;
  DistanceCalculator &distanceCalculator;
  TargetCalculator &stateHistory;
  TargetHashSet reachedTargets;
  std::unordered_map<ExecutionState *, TargetHashSet> reachedOnLastUpdate;
  std::unordered_map<ExecutionState *, TargetHashMap<weight_type>>
      calculatedDistance;

  Guidance guidance;

  void innerUpdate(ExecutionState *current,
                   const std::vector<ExecutionState *> &addedStates,
                   const std::vector<ExecutionState *> &removedStates);
  void stepTo(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);
  bool updateDistance(ExecutionState *es, ref<Target> target,
                      bool isStateRemoved);
  void updateDistance(ExecutionState *es, bool isStateRemoved = false);
  WeightResult tryGetWeight(ExecutionState *es, ref<Target> target,
                            weight_type &weight);
  bool isCalculated(ExecutionState *es, ref<Target> target);
  void updateConfidences(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates);
  void removeDistance(ExecutionState *es, ref<Target> target);
  void updateTargetlessState(ExecutionState *es);
  void handleTargetlessStates(ExecutionState *current,
                              const std::vector<ExecutionState *> &addedStates);
};

} // namespace klee

#endif /* KLEE_TARGET_REACHABILITY_H */