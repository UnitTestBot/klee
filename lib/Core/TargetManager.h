//===-- TargetedManager.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to manage everything about targets
//
//===----------------------------------------------------------------------===//

#include "DistanceCalculator.h"

#include "ObjectManager.h"
#include "klee/Core/Interpreter.h"
#include "klee/Module/TargetHash.h"

#include <map>
#include <unordered_map>

#ifndef KLEE_TARGETMANAGER_H
#define KLEE_TARGETMANAGER_H

namespace klee {
class TargetCalculator;

class TargetManagerSubscriber {
public:
  using TargetHistoryTargetPair =
      std::pair<ref<const TargetsHistory>, ref<Target>>;
  using StatesVector = std::vector<ExecutionState *>;
  using TargetHistoryTargetPairToStatesMap =
      std::unordered_map<TargetHistoryTargetPair, StatesVector,
                         TargetHistoryTargetHash, TargetHistoryTargetCmp>;

  virtual ~TargetManagerSubscriber() = default;

  /// Selects a state for further exploration.
  /// \return The selected state.
  virtual void update(const TargetHistoryTargetPairToStatesMap &added,
                      const TargetHistoryTargetPairToStatesMap &removed) = 0;
};

class TargetManager final : public Subscriber {
private:
  using StatesSet = std::unordered_set<ExecutionState *>;
  using StateToDistanceMap =
      std::unordered_map<const ExecutionState *, TargetHashMap<DistanceResult>>;
  using TargetHistoryTargetPair =
      std::pair<ref<const TargetsHistory>, ref<Target>>;
  using StatesVector = std::vector<ExecutionState *>;
  using TargetHistoryTargetPairToStatesMap =
      std::unordered_map<TargetHistoryTargetPair, StatesVector,
                         TargetHistoryTargetHash, TargetHistoryTargetCmp>;
  using TargetForestHistoryTargetSet =
      std::unordered_set<TargetHistoryTargetPair, TargetHistoryTargetHash,
                         TargetHistoryTargetCmp>;

  Interpreter::GuidanceKind guidance;
  DistanceCalculator &distanceCalculator;
  TargetCalculator &targetCalculator;
  TargetHashSet reachedTargets;
  StatesSet states;
  StateToDistanceMap distances;
  StatesSet localStates;
  StatesSet changedStates;
  TargetManagerSubscriber *searcher = nullptr;
  TargetManagerSubscriber *branchSearcher = nullptr;
  TargetHistoryTargetPairToStatesMap addedTStates;
  TargetHistoryTargetPairToStatesMap removedTStates;
  TargetHashSet removedTargets;
  TargetHashSet addedTargets;

  // For guided mode check
  TargetHashMap<StatesSet> targetToStates;

  void setTargets(ExecutionState &state, const TargetHashSet &targets) {
    for (auto i : state.targets()) {
      if (!targets.count(i) && state.isolated) {
        targetToStates[i].erase(&state);
      }
    }
    for (auto i : targets) {
      if (state.isolated) {
        targetToStates[i].insert(&state);
      }
    }
    state.setTargets(targets);
    changedStates.insert(&state);
  }

  void setHistory(ExecutionState &state, ref<TargetsHistory> history) {
    state.setHistory(history);
    changedStates.insert(&state);
  }

  void updateMiss(ExecutionState &state, ref<Target> target);

  void updateContinue(ExecutionState &state, ref<Target> target);

  void updateDone(ExecutionState &state, ref<Target> target);

  void updateReached(ExecutionState &state);

  void updateTargets(ExecutionState &state);

  void updateMiss(ProofObligation &pob, ref<Target> target);

  void updateContinue(ProofObligation &pob, ref<Target> target);

  void updateDone(ProofObligation &pob, ref<Target> target);

  void updateTargets(ProofObligation &pob);

  void collect(ExecutionState &state);

  static bool isReachedTarget(const ExecutionState &state, ref<Target> target,
                              WeightResult &result);

public:
  TargetManager(Interpreter::GuidanceKind _guidance,
                DistanceCalculator &_distanceCalculator,
                TargetCalculator &_targetCalculator)
      : guidance(_guidance), distanceCalculator(_distanceCalculator),
        targetCalculator(_targetCalculator){};

  void update(ref<ObjectManager::Event> e) override;
  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates,
              bool isolated);
  void update(ExecutionState *context, const pobs_ty &addedPobs,
              const pobs_ty &removedPobs);

  DistanceResult distance(const ExecutionState &state, ref<Target> target) {

    WeightResult wresult;
    if (isReachedTarget(state, target, wresult)) {
      return DistanceResult(wresult);
    }

    if (!state.isTransfered() && distances[&state].count(target)) {
      return distances[&state][target];
    }

    DistanceResult result =
        distanceCalculator.getDistance(state, target->getBlock());

    if (Done == result.result && (!isa<ReachBlockTarget>(target) ||
                                  cast<ReachBlockTarget>(target)->isAtEnd())) {
      result.result = Continue;
    }

    distances[&state][target] = result;

    return result;
  }

  DistanceResult distance(const ProofObligation &pob, ref<Target> target) {
    return distanceCalculator.getDistance(pob, target->getBlock());
  }

  const TargetHashSet &targets(const ExecutionState &state) {
    return state.targets();
  }

  const TargetHashSet &targets(const ProofObligation &pob) {
    return pob.targetForest.getTargets();
  }

  ref<const TargetsHistory> history(const ExecutionState &state) {
    return state.history();
  }

  ref<const TargetsHistory> prevHistory(const ExecutionState &state) {
    return state.prevHistory();
  }

  const TargetHashSet &prevTargets(const ExecutionState &state) {
    return state.prevTargets();
  }

  TargetForest &targetForest(ExecutionState &state) {
    return state.targetForest;
  }

  TargetForest &targetForest(ProofObligation &pob) { return pob.targetForest; }

  void subscribeSearcher(TargetManagerSubscriber &subscriber) {
    searcher = &subscriber;
  }

  void subscribeBranchSearcher(TargetManagerSubscriber &subscriber) {
    branchSearcher = &subscriber;
  }

  bool isTargeted(const ExecutionState &state) { return state.isTargeted(); }

  bool isTargeted(const ProofObligation &pob) { return pob.isTargeted(); }

  static bool isReachedTarget(const ExecutionState &state, ref<Target> target);

  void setReached(ref<Target> target) { reachedTargets.insert(target); }

  bool hasTargetedStates(ref<Target> target) {
    return targetToStates.count(target) && !targetToStates.at(target).empty();
  }
};

} // namespace klee

#endif /* KLEE_TARGETMANAGER_H */
