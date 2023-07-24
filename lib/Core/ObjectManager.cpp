#include "ObjectManager.h"

#include "PForest.h"
#include "CoreStats.h"
#include "SearcherUtil.h"

#include "klee/Module/KModule.h"
#include "klee/Support/Debug.h"
#include "klee/Support/DebugFlags.h"

#include "llvm/Support/CommandLine.h"
#include <algorithm>

using namespace llvm;
using namespace klee;

ObjectManager::ObjectManager(KBlockPredicate predicate)
    : predicate(predicate), emptyState(nullptr) {}

ObjectManager::~ObjectManager() {}

void ObjectManager::addSubscriber(Subscriber *s) { subscribers.push_back(s); }

void ObjectManager::addProcessForest(PForest *pf) { processForest = pf; }

void ObjectManager::setEmptyState(ExecutionState *state) {
  emptyState = state;
}

void ObjectManager::addInitialState(ExecutionState *state) {
  reachedStates[Target::create(state->pc->parent)].insert(state->copy());
  states.insert(state);
  processForest->addRoot(state);
}

void ObjectManager::clear() {
  delete emptyState;
  for (auto target : reachedStates) {
    for (auto state : target.second) {
      delete state;
    }
  }
}

void ObjectManager::setCurrentState(ExecutionState *_current) {
  assert(current == nullptr);
  current = _current;
  statesUpdated = true;
  stateUpdateKind =
      (current->isolated ? StateKind::Isolated : StateKind::Regular);
}

ExecutionState *ObjectManager::branchState(ExecutionState *state,
                                           BranchType reason) {
  if (statesUpdated) {
    auto kind = (state->isolated ? StateKind::Isolated : StateKind::Regular);
    assert(kind == stateUpdateKind);
  } else {
    assert(0); // Is this possible?
  }
  ExecutionState *newState = state->branch();
  addedStates.push_back(newState);
  processForest->attach(state->ptreeNode, newState, state, reason);
  stats::incBranchStat(reason, 1);
  return newState;
}

void ObjectManager::removeState(ExecutionState *state) {
  std::vector<ExecutionState *>::iterator itr =
      std::find(removedStates.begin(), removedStates.end(), state);
  assert(itr == removedStates.end());

  if (!statesUpdated) {
    statesUpdated = true;
    stateUpdateKind =
        (state->isolated ? StateKind::Isolated : StateKind::Regular);
  } else {
    auto kind = (state->isolated ? StateKind::Isolated : StateKind::Regular);
    assert(kind == stateUpdateKind);
  }

  state->pc = state->prevPC;
  removedStates.push_back(state);
}

ExecutionState *ObjectManager::initializeState(KInstruction *location,
                                               std::set<ref<Target>> targets) {
  ExecutionState *state = nullptr;
  state = emptyState->withKInstruction(location);
  processForest->addRoot(state);
  state->setTargeted(true);
  for (auto target : targets) {
    state->targetForest.add(target);
  }

  state->setHistory(state->targetForest.getHistory());
  state->setTargets(state->targetForest.getTargets());
  state->stepTargetsAndHistory();

  statesUpdated = true;
  stateUpdateKind = StateKind::Isolated;
  addedStates.push_back(state);
  return state;
}

void ObjectManager::updateSubscribers() {
  if (statesUpdated) {
    assert(stateUpdateKind != StateKind::None);
    bool isolated = stateUpdateKind == StateKind::Isolated;

    if (isolated) {
      checkReachedStates();
    }

    if (!isolated) {
      checkReachedPobs();
    }

    ref<Event> e = new States(current, addedStates, removedStates, isolated);
    for (auto s : subscribers) {
      s->update(e);
    }

    if (current) {
      current->stepTargetsAndHistory();
    }

    for (auto state : addedStates) {
      state->stepTargetsAndHistory();
      isolated ? isolatedStates.insert(state) : states.insert(state);
    }

    for (auto state : removedStates) {
      processForest->remove(state->ptreeNode);
      isolated ? isolatedStates.erase(state) : states.erase(state);
      delete state;
    }

    current = nullptr;
    addedStates.clear();
    removedStates.clear();
    statesUpdated = false;
    stateUpdateKind = StateKind::None;
  }

  {
    ref<Event> e = new Propagations(addedPropagations, removedPropagations);
    for (auto s : subscribers) {
      s->update(e);
    }
    for (auto prop : addedPropagations) {
      propagations[prop.pob->location].insert(prop);
    }
    for (auto prop : removedPropagations) {
      propagations[prop.pob->location].erase(prop);
    }
    addedPropagations.clear();
    removedPropagations.clear();
  }

  {
    ref<Event> e = new ProofObligations(addedPobs, removedPobs);
    for (auto s : subscribers) {
      s->update(e);
    }
    for (auto pob : addedPobs) {
      pobs[pob->location].insert(pob);
    }
    for (auto pob : removedPobs) {
      pobs[pob->location].erase(pob);
      delete pob;
    }
    addedPobs.clear();
    removedPobs.clear();
  }

  {
    ref<Event> e = new Conflicts(addedTargetedConflicts);
    for (auto s : subscribers) {
      s->update(e);
    }
    addedTargetedConflicts.clear();
  }
}

void ObjectManager::initialUpdate() {
  addedStates.insert(addedStates.begin(), states.begin(), states.end());
  statesUpdated = true;
  stateUpdateKind = StateKind::Regular;
  updateSubscribers();
}

const states_ty &ObjectManager::getStates() { return states; }

const states_ty &ObjectManager::getIsolatedStates() { return isolatedStates; }

void ObjectManager::checkReachedStates() {
  assert(statesUpdated && stateUpdateKind == StateKind::Isolated);
  std::set<ExecutionState *> states(addedStates.begin(), addedStates.end());
  if (current) {
    states.insert(current);
  }

  std::vector<ExecutionState *> toRemove;
  for (auto state : states) {
    if (!isOKIsolatedState(state)) {
      toRemove.push_back(state);
      continue;
    }

    auto reached = state->getTarget();
    if (!reached || state->constraints.path().KBlockSize() == 0) {
      continue;
    }

    if (state->targets().count(reached)) {
      if (debugPrints.isSet(DebugPrint::Reached)) {
        llvm::errs() << "[reached] Isolated state: "
                     << state->pathAndPCToString() << "\n";
      }
      auto copy = state->copy();
      reachedStates[reached].insert(copy);
      for (auto pob : pobs[reached]) {
        if (checkStack(copy, pob)) {
          addedPropagations.insert({copy, pob});
        }
      }
      if (predicate(reached->getBlock())) {
        toRemove.push_back(state);
      }
    } else {
      if (predicate(reached->getBlock())) {
        toRemove.push_back(state);
      }
    }
  }

  for (auto state : toRemove) {
    if (std::find(removedStates.begin(), removedStates.end(), state) ==
        removedStates.end()) {
      removeState(state);
    }
  }
}

void ObjectManager::checkReachedPobs() {
  assert(statesUpdated && stateUpdateKind == StateKind::Regular);

  std::set<ExecutionState *> states(addedStates.begin(), addedStates.end());
  if (current) {
    states.insert(current);
  }

  std::set<ProofObligation *> toRemove;
  for (auto state : states) {
    auto reached = state->getTarget();
    if (reached && pobs.count(reached)) {
      for (auto pob : pobs.at(reached)) {
        if (!pob->parent) {
          if (debugPrints.isSet(DebugPrint::ClosePob)) {
            llvm::errs() << "[close pob] Pob closed due to forward reach at: "
                         << pob->location->toString() << "\n";
          }
          toRemove.insert(pob);
        }
      }
    }
  }

  for (auto pob : toRemove) {
    removePob(pob);
  }
}

bool ObjectManager::isOKIsolatedState(ExecutionState *state) {
  assert(state->isolated);

  return true;

  if (state->stack.size() < 2) {
    return true;
  }
  if (state->stack.size() == 2) {
    auto initBlock = state->initPC->parent;
    if (isa<KCallBlock>(initBlock) &&
        state->initPC == initBlock->getFirstInstruction()) {
      return true;
    }
  }
  return false;
}

void ObjectManager::addTargetedConflict(ref<TargetedConflict> conflict) {
  addedTargetedConflicts.push_back(conflict);
}

void ObjectManager::addPob(ProofObligation *pob) {
  assert(!pobExists(pob));

  if (!pob->parent && debugPrints.isSet(DebugPrint::RootPob)) {
    llvm::errs() << "[pob] New root proof obligation at: "
                 << pob->location->toString() << "\n";
  }

  // if (pob->parent) {
  //   llvm::errs() << "[pob] NEW POB WITH:\n";
  //   pob->constraints.cs().dump();
  // }

  addedPobs.insert(pob);
  pathedPobs.insert({{pob->constraints.path(), pob->location}, pob});
  for (auto state : reachedStates[pob->location]) {
    if (checkStack(state, pob)) {
      addedPropagations.insert({state, pob});
    }
  }
}

void ObjectManager::removePob(ProofObligation *pob) {
  auto subtree = pob->getSubtree();
  for (auto pob : subtree) {
    removedPobs.insert(pob);
    pathedPobs.erase({pob->constraints.path(), pob->location});
    for (auto prop : propagations[pob->location]) {
      if (prop.pob == pob) {
        removedPropagations.insert(prop);
      }
    }
  }
}

void ObjectManager::removePropagation(Propagation prop) {
  removedPropagations.insert(prop);
}

bool ObjectManager::checkStack(ExecutionState *state, ProofObligation *pob) {
  if (state->stack.size() == 0) {
    return true;
  }

  size_t range = std::min(state->stack.callStack().size() - 1, pob->stack.size());
  auto stateIt = state->stack.callStack().rbegin();
  auto pobIt = pob->stack.rbegin();

  for (size_t i = 0; i < range; ++i) {
    if (stateIt->kf != pobIt->second ||
        (pobIt->first && &*(stateIt->caller) != pobIt->first)) {
      return false;
    }
    stateIt++;
    pobIt++;
  }
  return true;
}
