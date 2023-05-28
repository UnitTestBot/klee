#include "ObjectManager.h"
#include "PForest.h"
#include "SearcherUtil.h"
#include "klee/Module/KModule.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace klee;

ObjectManager::ObjectManager() {}

void ObjectManager::addSubscriber(Subscriber *s) { subscribers.push_back(s); }

void ObjectManager::addProcessForest(PForest *pf) { processForest = pf; }

void ObjectManager::makeInitialStates(ExecutionState *initial) {
  initialState = initial->copy();

  addedStates.push_back(initial);
  processForest->addRoot(initial);

  emptyState = initial->copy();
  emptyState->stack.clear();
}

void ObjectManager::setCurrentState(ExecutionState *_current) {
  assert(current == nullptr);
  current = _current;
  statesUpdated = true;
}

ExecutionState *ObjectManager::branchState(ExecutionState *state,
                                           BranchType reason) {
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
  }

  state->pc = state->prevPC;
  removedStates.push_back(state);
}

void ObjectManager::updateSubscribers() {
  if (statesUpdated) {
    ref<Event> e = new States(current, addedStates, removedStates);
    for (auto s : subscribers) {
      s->update(e);
    }
    for (auto state : addedStates) {
      states.insert(state);
    }
    for (auto state : removedStates) {
      processForest->remove(state->ptreeNode);
      states.erase(state);
      delete state;
    }

    current = nullptr;
    addedStates.clear();
    removedStates.clear();
    statesUpdated = false;
  }
}

void ObjectManager::initialUpdate() {
  addedStates.insert(addedStates.begin(), states.begin(), states.end());
  statesUpdated = true;
  updateSubscribers();
  addedStates.clear();
  statesUpdated = false;
}

const states_ty &ObjectManager::getStates() { return states; }
