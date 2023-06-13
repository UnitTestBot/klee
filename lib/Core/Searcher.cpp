//===-- Searcher.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Searcher.h"

#include "CoreStats.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "MergeHandler.h"
#include "PTree.h"
#include "StatsTracker.h"
#include "TargetCalculator.h"
#include "TargetReachability.h"

#include "klee/ADT/DiscretePDF.h"
#include "klee/ADT/RNG.h"
#include "klee/ADT/WeightedQueue.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/System/Time.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"

#include <cassert>
#include <cmath>
#include <set>

using namespace klee;
using namespace llvm;

///

ExecutionState &DFSSearcher::selectState() { return *states.back(); }

void DFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    if (state == states.back()) {
      states.pop_back();
    } else {
      auto it = std::find(states.begin(), states.end(), state);
      assert(it != states.end() && "invalid state removed");
      states.erase(it);
    }
  }
}

bool DFSSearcher::empty() { return states.empty(); }

void DFSSearcher::printName(llvm::raw_ostream &os) { os << "DFSSearcher\n"; }

///

ExecutionState &BFSSearcher::selectState() { return *states.front(); }

void BFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // update current state
  // Assumption: If new states were added KLEE forked, therefore states evolved.
  // constraints were added to the current state, it evolved.
  if (!addedStates.empty() && current &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end()) {
    auto pos = std::find(states.begin(), states.end(), current);
    assert(pos != states.end());
    states.erase(pos);
    states.push_back(current);
  }

  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    if (state == states.front()) {
      states.pop_front();
    } else {
      auto it = std::find(states.begin(), states.end(), state);
      assert(it != states.end() && "invalid state removed");
      states.erase(it);
    }
  }
}

bool BFSSearcher::empty() { return states.empty(); }

void BFSSearcher::printName(llvm::raw_ostream &os) { os << "BFSSearcher\n"; }

///

RandomSearcher::RandomSearcher(RNG &rng) : theRNG{rng} {}

ExecutionState &RandomSearcher::selectState() {
  return *states[theRNG.getInt32() % states.size()];
}

void RandomSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    auto it = std::find(states.begin(), states.end(), state);
    assert(it != states.end() && "invalid state removed");
    states.erase(it);
  }
}

bool RandomSearcher::empty() { return states.empty(); }

void RandomSearcher::printName(llvm::raw_ostream &os) {
  os << "RandomSearcher\n";
}

///

static unsigned int ulog2(unsigned int val) {
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

///

TargetedSearcher::TargetedSearcher(ref<Target> target,
                                   DistanceCalculator *distanceCalculator_)
    : states(std::make_unique<
             WeightedQueue<ExecutionState *, ExecutionStateIDCompare>>()),
      target(target), distanceCalculator(distanceCalculator_) {}

ExecutionState &TargetedSearcher::selectState() { return *states->choose(0); }

WeightResult TargetedSearcher::tryGetWeight(ExecutionState *es,
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
      states->tryGetWeight(es, weight)) {
    return Continue;
  }

  auto distRes = distanceCalculator->getDistance(*es, target);
  weight = ulog2(distRes.weight + es->steppedMemoryInstructions); // [0, 32]
  if (!distRes.isInsideFunction) {
    weight += 32; // [32, 64]
  }

  return distRes.result;
}

void TargetedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  assert(distanceCalculator);
  updateCheckCanReach(current, addedStates, removedStates);
}

bool TargetedSearcher::updateCheckCanReach(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  weight_type weight = 0;
  bool canReach = false;

  // update current
  if (current && std::find(removedStates.begin(), removedStates.end(),
                           current) == removedStates.end()) {
    switch (tryGetWeight(current, weight)) {
    case Continue:
      states->update(current, weight);
      canReach = true;
      break;
    case Done:
      reachedOnLastUpdate.insert(current);
      canReach = true;
      break;
    case Miss:
      current->targetForest.remove(target);
      states->remove(current);
      break;
    }
  }

  // insert states
  for (const auto state : addedStates) {
    switch (tryGetWeight(state, weight)) {
    case Continue:
      states->insert(state, weight);
      canReach = true;
      break;
    case Done:
      states->insert(state, weight);
      reachedOnLastUpdate.insert(state);
      canReach = true;
      break;
    case Miss:
      state->targetForest.remove(target);
      break;
    }
  }

  // remove states
  for (const auto state : removedStates) {
    if (target->atReturn() && !target->shouldFailOnThisTarget() &&
        state->prevPC == target->getBlock()->getLastInstruction()) {
      canReach = true;
      reachedOnLastUpdate.insert(state);
    } else {
      switch (tryGetWeight(state, weight)) {
      case Done:
        reachedOnLastUpdate.insert(state);
        canReach = true;
        break;
      case Miss:
        state->targetForest.remove(target);
        states->remove(state);
        break;
      case Continue:
        states->remove(state);
        canReach = true;
        break;
      }
    }
  }
  return canReach;
}

bool TargetedSearcher::empty() { return states->empty(); }

void TargetedSearcher::printName(llvm::raw_ostream &os) {
  os << "TargetedSearcher";
}

TargetedSearcher::~TargetedSearcher() {
  while (!states->empty()) {
    auto &state = selectState();
    states->remove(&state);
  }
}

void TargetedSearcher::updateWeight(ExecutionState *es, weight_type weight) {
  states->update(es, weight);
}

void TargetedSearcher::addWeight(ExecutionState *es, weight_type weight) {
  states->insert(es, weight);
}

void TargetedSearcher::removeWeight(ExecutionState *es) { states->remove(es); }

///

GuidedSearcher::GuidedSearcher(
    Searcher *baseSearcher, TargetReachability &targetReachability_,
    std::set<ExecutionState *, ExecutionStateIDCompare> &pausedStates, RNG &rng)
    : guidance(CoverageGuidance), baseSearcher(baseSearcher),
      targetReachability(targetReachability_), pausedStates(pausedStates),
      theRNG(rng) {}

GuidedSearcher::GuidedSearcher(
    TargetReachability &targetReachability_,
    std::set<ExecutionState *, ExecutionStateIDCompare> &pausedStates, RNG &rng)
    : guidance(ErrorGuidance), baseSearcher(nullptr),
      targetReachability(targetReachability_), pausedStates(pausedStates),
      theRNG(rng) {}

ExecutionState &GuidedSearcher::selectState() {
  unsigned size = historiesAndTargets.size();
  index = theRNG.getInt32() % (size + 1);
  ExecutionState *state = nullptr;
  if (CoverageGuidance == guidance && index == size) {
    assert(baseSearcher);
    state = &baseSearcher->selectState();
  } else {
    index = index % size;
    auto &historyTargetPair = historiesAndTargets[index];
    ref<TargetForest::History> history = historyTargetPair.first;
    ref<Target> target = historyTargetPair.second;
    assert(targetedSearchers.find(history) != targetedSearchers.end() &&
           targetedSearchers.at(history).find(target) !=
               targetedSearchers.at(history).end() &&
           !targetedSearchers.at(history)[target]->empty());
    state = &targetedSearchers.at(history).at(target)->selectState();
  }
  return *state;
}

TargetedSearcher *
GuidedSearcher::getTargetedSearcher(const ref<TargetForest::History> &history,
                                    const ref<Target> &target) {
  auto &historiedTargetedSearchers = targetedSearchers[history];

  if (historiedTargetedSearchers.count(target) == 0) {
    tryAddTarget(history, target);
  }

  return historiedTargetedSearchers[target].get();
}

void GuidedSearcher::updateForState(ExecutionState &es, bool isAdded,
                                    bool isRemoved) {
  if (isAdded) {
    const auto &history = es.currHistory;
    for (const auto &target : es.currTargets) {
      auto *targetedSearcher = getTargetedSearcher(history, target);
      auto weight = targetReachability.getDistance(es, target);
      targetedSearcher->addWeight(&es, weight);
    }
  } else if (isRemoved) {
    const auto &history = es.prevHistory;
    for (const auto &target : es.prevTargets) {
      auto *targetedSearcher = getTargetedSearcher(history, target);
      targetedSearcher->removeWeight(&es);
      if (targetedSearcher->empty()) {
        removeTarget(history, target);
      }
    }
  } else {
    if (es.prevHistory.get() != es.currHistory.get()) {
      for (const auto &target : es.prevTargets) {
        auto *targetedSearcher = getTargetedSearcher(es.prevHistory, target);
        targetedSearcher->removeWeight(&es);
        if (targetedSearcher->empty()) {
          removeTarget(es.prevHistory, target);
        }
      }

      for (const auto &target : es.currTargets) {
        auto *targetedSeacher = getTargetedSearcher(es.currHistory, target);
        auto weight = targetReachability.getDistance(es, target);
        targetedSeacher->addWeight(&es, weight);
      }
    } else {
      for (const auto &target : es.prevTargets) {
        if (es.currTargets.count(target) == 0) {
          auto *targetedSearcher = getTargetedSearcher(es.currHistory, target);
          targetedSearcher->removeWeight(&es);
          if (targetedSearcher->empty()) {
            removeTarget(es.currHistory, target);
          }
        }
      }

      for (const auto &target : es.currTargets) {
        if (es.prevTargets.count(target) == 0) {
          auto *targetedSearcher = getTargetedSearcher(es.currHistory, target);
          auto weight = targetReachability.getDistance(es, target);
          targetedSearcher->addWeight(&es, weight);
        } else {
          auto *targetedSearcher = getTargetedSearcher(es.currHistory, target);
          auto weight = targetReachability.getDistance(es, target);
          targetedSearcher->updateWeight(&es, weight);
        }
      }
    }
  }
}

void GuidedSearcher::innerUpdate(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  bool isCurrentRemoved = false;
  baseAddedStates.insert(baseAddedStates.end(), addedStates.begin(),
                         addedStates.end());
  baseRemovedStates.insert(baseRemovedStates.end(), removedStates.begin(),
                           removedStates.end());

  std::vector<ExecutionState *> addedStuckStates;
  if (ErrorGuidance == guidance) {
    if (current &&
        (std::find(baseRemovedStates.begin(), baseRemovedStates.end(),
                   current) == baseRemovedStates.end()) &&
        current->isStuck()) {
      pausedStates.insert(current);
      isCurrentRemoved = true;
    }
    for (const auto state : addedStates) {
      if (state->isStuck()) {
        pausedStates.insert(state);
        addedStuckStates.push_back(state);
        auto is =
            std::find(baseAddedStates.begin(), baseAddedStates.end(), state);
        assert(is != baseAddedStates.end());
        baseAddedStates.erase(is);
      }
    }
  }

  for (const auto state : baseAddedStates) {
    if (!state->currTargets.empty()) {
      targetedAddedStates.push_back(state);
    } else {
      targetlessStates.push_back(state);
    }
  }

  if (current && current->currTargets.empty() &&
      std::find(baseRemovedStates.begin(), baseRemovedStates.end(), current) ==
          baseRemovedStates.end()) {
    targetlessStates.push_back(current);
  }

  if (!baseRemovedStates.empty()) {
    std::vector<ExecutionState *> alt = baseRemovedStates;
    for (const auto state : alt) {
      auto it = pausedStates.find(state);
      if (it != pausedStates.end()) {
        pausedStates.erase(it);
        baseRemovedStates.erase(std::remove(baseRemovedStates.begin(),
                                            baseRemovedStates.end(), state),
                                baseRemovedStates.end());
      }
    }
  }

  if (CoverageGuidance == guidance) {
    for (const auto state : targetlessStates) {
      if (state->isStuck()) {
        pausedStates.insert(state);
        if (std::find(baseAddedStates.begin(), baseAddedStates.end(), state) !=
            baseAddedStates.end()) {
          auto is =
              std::find(baseAddedStates.begin(), baseAddedStates.end(), state);
          baseAddedStates.erase(is);
        } else {
          baseRemovedStates.push_back(state);
        }
      }
    }
  }

  targetlessStates.clear();

  if (current && std::find(baseRemovedStates.begin(), baseRemovedStates.end(),
                           current) == baseRemovedStates.end()) {
    updateForState(*current, false, isCurrentRemoved);
  }

  for (const auto state : targetedAddedStates) {
    updateForState(*state, true, false);
  }
  targetedAddedStates.clear();

  for (const auto state : baseRemovedStates) {
    updateForState(*state, false, true);
  }

  if (CoverageGuidance == guidance) {
    assert(baseSearcher);
    baseSearcher->update(current, baseAddedStates, baseRemovedStates);
  }

  baseAddedStates.clear();
  baseRemovedStates.clear();
}

void GuidedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  targetReachability.update(current, addedStates, removedStates);
  innerUpdate(current, addedStates, removedStates);
}

bool GuidedSearcher::empty() {
  return CoverageGuidance == guidance ? baseSearcher->empty()
                                      : targetedSearchers.empty();
}

void GuidedSearcher::printName(llvm::raw_ostream &os) {
  os << "GuidedSearcher\n";
}

bool GuidedSearcher::tryAddTarget(ref<TargetForest::History> history,
                                  ref<Target> target) {
  assert(targetedSearchers.count(history) == 0 ||
         targetedSearchers.at(history).count(target) == 0);
  targetedSearchers[history][target] =
      std::make_unique<TargetedSearcher>(target, nullptr);
  auto it = std::find_if(
      historiesAndTargets.begin(), historiesAndTargets.end(),
      [&history, &target](
          const std::pair<ref<TargetForest::History>, ref<Target>> &element) {
        return element.first.get() == history.get() &&
               element.second.get() == target.get();
      });
  assert(it == historiesAndTargets.end());
  historiesAndTargets.push_back({history, target});
  return true;
}

GuidedSearcher::TargetForestHisoryTargetVector::iterator
GuidedSearcher::removeTarget(ref<TargetForest::History> history,
                             ref<Target> target) {
  targetedSearchers.at(history).erase(target);
  auto it = std::find_if(
      historiesAndTargets.begin(), historiesAndTargets.end(),
      [&history, &target](
          const std::pair<ref<TargetForest::History>, ref<Target>> &element) {
        return element.first.get() == history.get() &&
               element.second.get() == target.get();
      });
  assert(it != historiesAndTargets.end());
  if (targetedSearchers.at(history).empty()) {
    targetedSearchers.erase(history);
  }
  return historiesAndTargets.erase(it);
}

///

WeightedRandomSearcher::WeightedRandomSearcher(WeightType type, RNG &rng)
    : states(std::make_unique<
             DiscretePDF<ExecutionState *, ExecutionStateIDCompare>>()),
      theRNG{rng}, type(type) {

  switch (type) {
  case Depth:
  case RP:
    updateWeights = false;
    break;
  case InstCount:
  case CPInstCount:
  case QueryCost:
  case MinDistToUncovered:
  case CoveringNew:
    updateWeights = true;
    break;
  default:
    assert(0 && "invalid weight type");
  }
}

ExecutionState &WeightedRandomSearcher::selectState() {
  return *states->choose(theRNG.getDoubleL());
}

double WeightedRandomSearcher::getWeight(ExecutionState *es) {
  switch (type) {
  default:
  case Depth:
    return es->depth;
  case RP:
    return std::pow(0.5, es->depth);
  case InstCount: {
    uint64_t count = theStatisticManager->getIndexedValue(stats::instructions,
                                                          es->pc->info->id);
    double inv = 1. / std::max((uint64_t)1, count);
    return inv * inv;
  }
  case CPInstCount: {
    StackFrame &sf = es->stack.back();
    uint64_t count = sf.callPathNode->statistics.getValue(stats::instructions);
    double inv = 1. / std::max((uint64_t)1, count);
    return inv;
  }
  case QueryCost:
    return (es->queryMetaData.queryCost.toSeconds() < .1)
               ? 1.
               : 1. / es->queryMetaData.queryCost.toSeconds();
  case CoveringNew:
  case MinDistToUncovered: {
    uint64_t md2u = computeMinDistToUncovered(
        es->pc, es->stack.back().minDistToUncoveredOnReturn);

    double invMD2U = 1. / (md2u ? md2u : 10000);
    if (type == CoveringNew) {
      double invCovNew = 0.;
      if (es->instsSinceCovNew)
        invCovNew = 1. / std::max(1, (int)es->instsSinceCovNew - 1000);
      return (invCovNew * invCovNew + invMD2U * invMD2U);
    } else {
      return invMD2U * invMD2U;
    }
  }
  }
}

void WeightedRandomSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  // update current
  if (current && updateWeights &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end())
    states->update(current, getWeight(current));

  // insert states
  for (const auto state : addedStates)
    states->insert(state, getWeight(state));

  // remove states
  for (const auto state : removedStates)
    states->remove(state);
}

bool WeightedRandomSearcher::empty() { return states->empty(); }

void WeightedRandomSearcher::printName(llvm::raw_ostream &os) {
  os << "WeightedRandomSearcher::";
  switch (type) {
  case Depth:
    os << "Depth\n";
    return;
  case RP:
    os << "RandomPath\n";
    return;
  case QueryCost:
    os << "QueryCost\n";
    return;
  case InstCount:
    os << "InstCount\n";
    return;
  case CPInstCount:
    os << "CPInstCount\n";
    return;
  case MinDistToUncovered:
    os << "MinDistToUncovered\n";
    return;
  case CoveringNew:
    os << "CoveringNew\n";
    return;
  default:
    os << "<unknown type>\n";
    return;
  }
}

///

// Check if n is a valid pointer and a node belonging to us
#define IS_OUR_NODE_VALID(n)                                                   \
  (((n).getPointer() != nullptr) && (((n).getInt() & idBitMask) != 0))

RandomPathSearcher::RandomPathSearcher(PForest &processForest, RNG &rng)
    : processForest{processForest}, theRNG{rng},
      idBitMask{processForest.getNextId()} {};

ExecutionState &RandomPathSearcher::selectState() {
  unsigned flips = 0, bits = 0, range = 0;
  PTreeNodePtr *root = nullptr;
  while (!root || !IS_OUR_NODE_VALID(*root))
    root = &processForest.getPTrees()
                .at(range++ % processForest.getPTrees().size() + 1)
                ->root;
  assert(root->getInt() & idBitMask && "Root should belong to the searcher");
  PTreeNode *n = root->getPointer();
  while (!n->state) {
    if (!IS_OUR_NODE_VALID(n->left)) {
      assert(IS_OUR_NODE_VALID(n->right) &&
             "Both left and right nodes invalid");
      assert(n != n->right.getPointer());
      n = n->right.getPointer();
    } else if (!IS_OUR_NODE_VALID(n->right)) {
      assert(IS_OUR_NODE_VALID(n->left) && "Both right and left nodes invalid");
      assert(n != n->left.getPointer());
      n = n->left.getPointer();
    } else {
      if (bits == 0) {
        flips = theRNG.getInt32();
        bits = 32;
      }
      --bits;
      n = ((flips & (1U << bits)) ? n->left : n->right).getPointer();
    }
  }

  return *n->state;
}

void RandomPathSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // insert states
  for (auto &es : addedStates) {
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;
    PTreeNodePtr &root = processForest.getPTrees().at(pnode->getTreeID())->root;
    PTreeNodePtr *childPtr;

    childPtr = parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                              : &parent->right)
                      : &root;
    while (pnode && !IS_OUR_NODE_VALID(*childPtr)) {
      childPtr->setInt(childPtr->getInt() | idBitMask);
      pnode = parent;
      if (pnode)
        parent = pnode->parent;

      childPtr = parent
                     ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                             : &parent->right)
                     : &root;
    }
  }

  // remove states
  for (auto es : removedStates) {
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;
    PTreeNodePtr &root = processForest.getPTrees().at(pnode->getTreeID())->root;

    while (pnode && !IS_OUR_NODE_VALID(pnode->left) &&
           !IS_OUR_NODE_VALID(pnode->right)) {
      auto childPtr =
          parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                         : &parent->right)
                 : &root;
      assert(IS_OUR_NODE_VALID(*childPtr) && "Removing pTree child not ours");
      childPtr->setInt(childPtr->getInt() & ~idBitMask);
      pnode = parent;
      if (pnode)
        parent = pnode->parent;
    }
  }
}

bool RandomPathSearcher::empty() {
  bool res = true;
  for (const auto &ntree : processForest.getPTrees())
    res = res && !IS_OUR_NODE_VALID(ntree.second->root);
  return res;
}

void RandomPathSearcher::printName(llvm::raw_ostream &os) {
  os << "RandomPathSearcher\n";
}

///

MergingSearcher::MergingSearcher(Searcher *baseSearcher)
    : baseSearcher{baseSearcher} {};

void MergingSearcher::pauseState(ExecutionState &state) {
  assert(std::find(pausedStates.begin(), pausedStates.end(), &state) ==
         pausedStates.end());
  pausedStates.push_back(&state);
  baseSearcher->update(nullptr, {}, {&state});
}

void MergingSearcher::continueState(ExecutionState &state) {
  auto it = std::find(pausedStates.begin(), pausedStates.end(), &state);
  assert(it != pausedStates.end());
  pausedStates.erase(it);
  baseSearcher->update(nullptr, {&state}, {});
}

ExecutionState &MergingSearcher::selectState() {
  assert(!baseSearcher->empty() && "base searcher is empty");

  if (!UseIncompleteMerge)
    return baseSearcher->selectState();

  // Iterate through all MergeHandlers
  for (auto cur_mergehandler : mergeGroups) {
    // Find one that has states that could be released
    if (!cur_mergehandler->hasMergedStates()) {
      continue;
    }
    // Find a state that can be prioritized
    ExecutionState *es = cur_mergehandler->getPrioritizeState();
    if (es) {
      return *es;
    } else {
      if (DebugLogIncompleteMerge) {
        llvm::errs() << "Preemptively releasing states\n";
      }
      // If no state can be prioritized, they all exceeded the amount of time we
      // are willing to wait for them. Release the states that already arrived
      // at close_merge.
      cur_mergehandler->releaseStates();
    }
  }
  // If we were not able to prioritize a merging state, just return some state
  return baseSearcher->selectState();
}

void MergingSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // We have to check if the current execution state was just deleted, as to
  // not confuse the nurs searchers
  if (std::find(pausedStates.begin(), pausedStates.end(), current) ==
      pausedStates.end()) {
    baseSearcher->update(current, addedStates, removedStates);
  }
}

bool MergingSearcher::empty() { return baseSearcher->empty(); }

void MergingSearcher::printName(llvm::raw_ostream &os) {
  os << "MergingSearcher\n";
}

///

BatchingSearcher::BatchingSearcher(Searcher *baseSearcher,
                                   time::Span timeBudget,
                                   unsigned instructionBudget)
    : baseSearcher{baseSearcher}, timeBudget{timeBudget},
      instructionBudget{instructionBudget} {};

ExecutionState &BatchingSearcher::selectState() {
  if (!lastState ||
      (((timeBudget.toSeconds() > 0) &&
        (time::getWallTime() - lastStartTime) > timeBudget)) ||
      ((instructionBudget > 0) &&
       (stats::instructions - lastStartInstructions) > instructionBudget)) {
    if (lastState) {
      time::Span delta = time::getWallTime() - lastStartTime;
      auto t = timeBudget;
      t *= 1.1;
      if (delta > t) {
        klee_message("increased time budget from %f to %f\n",
                     timeBudget.toSeconds(), delta.toSeconds());
        timeBudget = delta;
      }
    }
    lastState = &baseSearcher->selectState();
    lastStartTime = time::getWallTime();
    lastStartInstructions = stats::instructions;
    return *lastState;
  } else {
    return *lastState;
  }
}

void BatchingSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  // drop memoized state if it is marked for deletion
  if (std::find(removedStates.begin(), removedStates.end(), lastState) !=
      removedStates.end())
    lastState = nullptr;
  // update underlying searcher
  baseSearcher->update(current, addedStates, removedStates);
}

bool BatchingSearcher::empty() { return baseSearcher->empty(); }

void BatchingSearcher::printName(llvm::raw_ostream &os) {
  os << "<BatchingSearcher> timeBudget: " << timeBudget
     << ", instructionBudget: " << instructionBudget << ", baseSearcher:\n";
  baseSearcher->printName(os);
  os << "</BatchingSearcher>\n";
}

///

IterativeDeepeningTimeSearcher::IterativeDeepeningTimeSearcher(
    Searcher *baseSearcher)
    : baseSearcher{baseSearcher} {};

ExecutionState &IterativeDeepeningTimeSearcher::selectState() {
  ExecutionState &res = baseSearcher->selectState();
  startTime = time::getWallTime();
  return res;
}

void IterativeDeepeningTimeSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  const auto elapsed = time::getWallTime() - startTime;

  // update underlying searcher (filter paused states unknown to underlying
  // searcher)
  if (!removedStates.empty()) {
    std::vector<ExecutionState *> alt = removedStates;
    for (const auto state : removedStates) {
      auto it = pausedStates.find(state);
      if (it != pausedStates.end()) {
        pausedStates.erase(it);
        alt.erase(std::remove(alt.begin(), alt.end(), state), alt.end());
      }
    }
    baseSearcher->update(current, addedStates, alt);
  } else {
    baseSearcher->update(current, addedStates, removedStates);
  }

  // update current: pause if time exceeded
  if (current &&
      std::find(removedStates.begin(), removedStates.end(), current) ==
          removedStates.end() &&
      elapsed > time) {
    pausedStates.insert(current);
    baseSearcher->update(nullptr, {}, {current});
  }

  // no states left in underlying searcher: fill with paused states
  if (baseSearcher->empty()) {
    time *= 2U;
    klee_message("increased time budget to %f\n", time.toSeconds());
    std::vector<ExecutionState *> ps(pausedStates.begin(), pausedStates.end());
    baseSearcher->update(nullptr, ps, std::vector<ExecutionState *>());
    pausedStates.clear();
  }
}

bool IterativeDeepeningTimeSearcher::empty() {
  return baseSearcher->empty() && pausedStates.empty();
}

void IterativeDeepeningTimeSearcher::printName(llvm::raw_ostream &os) {
  os << "IterativeDeepeningTimeSearcher\n";
}

///

InterleavedSearcher::InterleavedSearcher(
    const std::vector<Searcher *> &_searchers) {
  searchers.reserve(_searchers.size());
  for (auto searcher : _searchers)
    searchers.emplace_back(searcher);
}

ExecutionState &InterleavedSearcher::selectState() {
  Searcher *s = searchers[--index].get();
  if (index == 0)
    index = searchers.size();
  return s->selectState();
}

void InterleavedSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  // update underlying searchers
  for (auto &searcher : searchers)
    searcher->update(current, addedStates, removedStates);
}

bool InterleavedSearcher::empty() { return searchers[0]->empty(); }

void InterleavedSearcher::printName(llvm::raw_ostream &os) {
  os << "<InterleavedSearcher> containing " << searchers.size()
     << " searchers:\n";
  for (const auto &searcher : searchers)
    searcher->printName(os);
  os << "</InterleavedSearcher>\n";
}
