#include "ObjectManager.h"

#include "CoreStats.h"
#include "PForest.h"
#include "SearcherUtil.h"
#include "TargetManager.h"

#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Support/Debug.h"
#include "klee/Support/DebugFlags.h"

#include "llvm/Support/CommandLine.h"
#include <algorithm>

using namespace llvm;
using namespace klee;

ObjectManager::ObjectManager() : tgms(nullptr), emptyState(nullptr) {}

ObjectManager::~ObjectManager() {}

void ObjectManager::addSubscriber(Subscriber *s) { subscribers.push_back(s); }

void ObjectManager::addProcessForest(PForest *pf) { processForest = pf; }

void ObjectManager::setEmptyState(ExecutionState *state) { emptyState = state; }

void ObjectManager::addInitialState(ExecutionState *state) {
  auto isolatedCopy = state->copy();
  isolatedCopy->isolated = true;
  isolatedCopy->finalComposing = true;
  reachedStates[state->getLocationTarget()].insert(isolatedCopy);
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

void ObjectManager::setContextState(ExecutionState *_context) {
  assert(context == nullptr);
  context = _context;
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

  // if (state->isolated) {
  //   llvm::errs() << "Removing isolated: " <<
  //   state->constraints.path().toString() << "\n";
  // }

  if (!statesUpdated) {
    statesUpdated = true;
    stateUpdateKind =
        (state->isolated ? StateKind::Isolated : StateKind::Regular);
  } else {
    auto kind = (state->isolated ? StateKind::Isolated : StateKind::Regular);
    assert(kind == stateUpdateKind);
  }

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

  statesUpdated = true;
  stateUpdateKind = StateKind::Isolated;
  addedStates.push_back(state);
  return state;
}

void ObjectManager::updateSubscribers() {
  if (statesUpdated) {
    assert(stateUpdateKind != StateKind::None);
    bool isolated = stateUpdateKind == StateKind::Isolated;

    ref<Event> ee = new States(current, addedStates, removedStates, isolated);
    if (tgms) {
      tgms->update(ee);
    }

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

    for (auto state : addedStates) {
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
      propagationCount[prop.pob]++;
    }
    for (auto prop : removedPropagations) {
      propagations[prop.pob->location].erase(prop);
      assert(propagationCount[prop.pob] > 0);
      propagationCount[prop.pob]--;
    }
    addedPropagations.clear();
    removedPropagations.clear();
  }

  {
    ref<Event> e = new ProofObligations(context, addedPobs, removedPobs);
    for (auto s : subscribers) {
      s->update(e);
    }
    for (auto pob : addedPobs) {
      pobs[pob->location].insert(pob);
      if (pob->parent) {
        leafPobs.erase(pob->parent);
      }
      if (pob->children.empty()) {
        leafPobs.insert(pob);
      }
    }
    for (auto pob : removedPobs) {
      if (!pob->parent) {
        rootPobs.erase(pob);
      }
      leafPobs.erase(pob);
      pobs[pob->location].erase(pob);
      auto parent = pob->parent;
      if (parent && parent->children.size() == 1 &&
          !removedPobs.count(parent)) {
        leafPobs.insert(parent);
      }
      delete pob;
    }
    addedPobs.clear();
    removedPobs.clear();
    context = nullptr;
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

const pobs_ty &ObjectManager::getLeafPobs() { return leafPobs; }

const pobs_ty &ObjectManager::getRootPobs() { return rootPobs; }

void ObjectManager::checkReachedStates() {
  assert(statesUpdated && stateUpdateKind == StateKind::Isolated);
  std::set<ExecutionState *> states(addedStates.begin(), addedStates.end());
  if (current) {
    states.insert(current);
  }

  for (auto i : removedStates) {
    states.insert(i);
  }

  std::vector<ExecutionState *> toRemove;
  for (auto state : states) {
    std::set<ref<Target>> reached;

    if (state->history() && state->history()->target) {
      auto target = state->history()->target;
      if (TargetManager::isReachedTarget(*state, target)) {
        reached.insert(target);
      }
    }

    assert(reached.size() <= 1);

    if (reached.size() == 1) {
      auto target = *(reached.begin());
      if (debugPrints.isSet(DebugPrint::Reached)) {
        llvm::errs() << "[reached] Isolated state: "
                     << state->constraints.path().toString() << "\n";
      }
      auto copy = state->copy();
      copy->addressSpace.complete = true;
      reachedStates[target].insert(copy);
      for (auto pob : pobs[target]) {
        if (checkStack(copy, pob)) {
          addedPropagations.insert({copy, pob});
        }
      }
      toRemove.push_back(state);
    }

    auto loc = state->getLocationTarget();
    if (loc && (*predicate)(loc->getBlock()) &&
        !state->constraints.path().empty()) {
      if (reached.size() == 0) {
        // assert(0 && "No reached but at special point");
      } else {
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

  pobs_ty toRemove;
  for (auto state : states) {
    auto reached = state->getLocationTarget();
    if (reached && pobs.count(reached)) {
      for (auto pob : pobs.at(reached)) {
        if (!pob->parent) {
          if (debugPrints.isSet(DebugPrint::ClosePob)) {
            llvm::errs() << "[close pob] Pob closed due to forward reach at: "
                         << pob->location->toString() << "\n";
          }
          toRemove.insert(pob);
          // llvm::errs() << "[TRUE POSITIVE] FOUND TRUE POSITIVE VIA FORWARD
          // AT: "
          //              << pob->root->location->toString() << "\n";
          // llvm::errs() << "[TRUE POSITIVE] State path: "
          //              << state->constraints.path().toString() << "\n";
        }
      }
    }
  }

  for (auto pob : toRemove) {
    removePob(pob);
  }
}

void ObjectManager::addTargetedConflict(ref<TargetedConflict> conflict) {
  addedTargetedConflicts.push_back(conflict);
}

void ObjectManager::addPob(ProofObligation *pob) {
  assert(!pobExists(pob));

  if (!pob->parent) {
    if (debugPrints.isSet(DebugPrint::RootPob)) {
      llvm::errs() << "[pob] New root proof obligation at: "
                   << pob->location->toString() << "\n";
    }
    rootPobs.insert(pob);
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

  size_t range =
      std::min(state->stack.callStack().size() - 1, pob->stack.size());
  auto stateIt = state->stack.callStack().rbegin();
  auto pobIt = pob->stack.rbegin();

  for (size_t i = 0; i < range; ++i) {
    if (stateIt->kf != pobIt->kf ||
        (pobIt->caller && pobIt->caller != stateIt->caller)) {
      return false;
    }
    stateIt++;
    pobIt++;
  }
  return true;
}
