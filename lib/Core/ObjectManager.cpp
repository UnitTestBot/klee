#include "ObjectManager.h"
#include "PForest.h"
#include "SearcherUtil.h"
#include "klee/Module/KModule.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace klee;
namespace klee {
cl::opt<bool> DebugBackward("debug-backward", cl::desc(""), cl::init(false),
                            cl::cat(DebugCat));

cl::opt<bool> DebugReached("debug-reached", cl::desc(""), cl::init(false),
                           cl::cat(DebugCat));

cl::opt<bool> DebugConflicts("debug-conflicts", cl::desc(""), cl::init(false),
                             cl::cat(DebugCat));
} // namespace klee

ObjectManager::ObjectManager() {}

void ObjectManager::addSubscriber(Subscriber *s) { subscribers.push_back(s); }

void ObjectManager::addProcessForest(PForest *pf) { processForest = pf; }

void ObjectManager::makeInitialStates(ExecutionState *initial) {
  initialState = initial->copy();

  addedStates.push_back(initial);
  processForest->addRoot(initial);

  emptyState = initial->copy();
  emptyState->stack.clear();
  emptyState->isolated = true;
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
                                               std::set<Target> targets) {
  ExecutionState *state = nullptr;
  if (location == initialState->initPC) {
    state = initialState->copy();
    state->isolated = true;
  } else {
    state = emptyState->withKInstruction(location);
  }
  processForest->addRoot(state);
  for (auto target : targets) {
    state->targets.insert(target);
  }
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
    }
    for (auto prop : removedPropagations) {
      propagations[prop.pob->location].erase(prop);
    }
    addedPropagations.clear();
    removedPropagations.clear();
  }

  {
    ref<Event> e = new ProofObligations(addedPobs, removedPobs);
    if (DebugBackward) {
      if (addedPobs.size() > 0) {
        llvm::errs() << "Propagated pobs\n";
        for (auto pob : addedPobs) {
          llvm::errs() << "Path: " << pob->constraints.path().toString()
                       << "\n";
          llvm::errs() << "Constraints:\n" << pob->constraints.cs() << "\n";
          llvm::errs() << "\n";
        }
      }
    }
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
    if (DebugConflicts) {
      if (addedTargetedConflicts.size() > 0) {
        llvm::errs() << "Contradictions was found\n";
        for (auto conflict : addedTargetedConflicts) {
          llvm::errs() << "Path: " << conflict->conflict.path.toString()
                       << "\n";
          llvm::errs() << "Contradiction [\n";
          for (const auto &constraint : conflict->conflict.core) {
            constraint->print(llvm::errs());
            llvm::errs() << "\n";
          }
          llvm::errs() << "]\n";
          llvm::errs() << "Target: " << conflict->target->toString() << "\n";
          llvm::errs() << "\n";
        }
      }
    }
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
  addedStates.clear();
  statesUpdated = false;
  stateUpdateKind = StateKind::None;
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
    for (auto target : state->targets) {
      if (state->reachedTarget(target)) {
        auto copy = state->copy();
        if (DebugReached) {
          llvm::errs() << "New isolated state.\n";
          llvm::errs() << "Id: " << state->id << "\n";
          llvm::errs() << "Path: " << state->constraints.path().toString()
                       << "\n";
          llvm::errs() << "Constraints:\n" << state->constraints.cs();
          llvm::errs() << "\n";
        }
        reachedStates[target].insert(copy);
        for (auto pob : pobs[target]) {
          if (checkStack(copy, pob)) {
            addedPropagations.insert({copy, pob});
          }
        }
        toRemove.push_back(state);
        break;
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

bool ObjectManager::isOKIsolatedState(ExecutionState *state) {
  assert(state->isolated);

  if (state->multilevel.size() > state->level.size()) {
    return false;
  }

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

// bool ObjectManager::checkStack(ExecutionState *state, ProofObligation *pob) {
//   if (state->stack.size() == 0) {
//     return true;
//   }

//   size_t range = std::min(state->stack.size() - 1, pob->stack.size());
//   auto stateIt = state->stack.rbegin();
//   auto pobIt = pob->stack.rbegin();

//   for (size_t i = 0; i < range; ++i) {
//     KInstruction *stateInstr = stateIt->caller;
//     KInstruction *pobInstr = *pobIt;
//     if (stateInstr != pobInstr) {
//       return false;
//     }
//     stateIt++;
//     pobIt++;
//   }
//   return true;
// }

bool ObjectManager::checkStack(ExecutionState *state, ProofObligation *pob) {
  if (state->stack.size() == 0) {
    return true;
  }

  size_t range = std::min(state->stack.size() - 1, pob->stack.size());
  auto stateIt = state->stack.rbegin();
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
