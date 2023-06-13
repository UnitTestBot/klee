#include "BidirectionalSearcher.h"
#include "BackwardSearcher.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "Initializer.h"
#include "ObjectManager.h"
#include "ProofObligation.h"
#include "Searcher.h"
#include "SearcherUtil.h"
#include "UserSearcher.h"
#include "klee/Core/Interpreter.h"
#include "klee/Module/KModule.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Instructions.h"

#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include <cstdlib>

namespace {} // namespace

namespace klee {

BidirectionalSearcher::StepKind BidirectionalSearcher::selectStep() {
  size_t initial_choice = ticker.getCurrent();
  size_t choice = initial_choice;

  do {
    switch (choice) {
    case 0: {
      if (!forward->empty()) {
        return StepKind::Forward;
      }
      break;
    }
    case 1: {
      if (!branch->empty()) {
        return StepKind::Branch;
      }
      break;
    }
    case 2: {
      if (!backward->empty()) {
        return StepKind::Backward;
      }
      break;
    }
    case 3: {
      if (!initializer->empty()) {
        return StepKind::Initialize;
      }
      break;
    }
    }
    ticker.moveToNext();
    choice = ticker.getCurrent();
  } while (choice != initial_choice);

  assert(0 && "Empty searcher queried for an action");
}

ref<BidirectionalAction> BidirectionalSearcher::selectAction() {
  ref<BidirectionalAction> action;

  while (action.isNull()) {
    switch (selectStep()) {

    case StepKind::Forward: {
      auto &state = forward->selectState();
      action = new ForwardAction(&state);
      break;
    }

    case StepKind::Branch: {
      auto &state = branch->selectState();
      action = new ForwardAction(&state);
      break;
    }

    case StepKind::Backward: {
      auto propagation = backward->selectAction();
      action = new BackwardAction(propagation);
      break;
    }

    case StepKind::Initialize: {
      auto initAndTargets = initializer->selectAction();
      action =
          new InitializeAction(initAndTargets.first, initAndTargets.second);
      break;
    }
    }
  }
  return action;
}

void BidirectionalSearcher::update(ref<ObjectManager::Event> e) {
  if (auto states = dyn_cast<ObjectManager::States>(e)) {
    if (states->isolated) {
      branch->update(states->modified, states->added, states->removed);
    } else {
      forward->update(states->modified, states->added, states->removed);
    }
  } else if (auto props = dyn_cast<ObjectManager::Propagations>(e)) {
    backward->update(props->added, props->removed);
  } else if (auto pobs = dyn_cast<ObjectManager::ProofObligations>(e)) {
    initializer->update(pobs->added, pobs->removed);
  } else if (auto conflicts = dyn_cast<ObjectManager::Conflicts>(e)) {
    for (auto conflict : conflicts->conflicts) {
      initializer->addConflictInit(conflict->conflict, conflict->target);
    }
  } else {
    assert(0 && "Unknown event");
  }
}

bool BidirectionalSearcher::empty() {
  return forward->empty() && branch->empty() && backward->empty() &&
         initializer->empty();
}

BidirectionalSearcher::BidirectionalSearcher(Searcher *_forward,
                                             Searcher *_branch,
                                             BackwardSearcher *_backward,
                                             Initializer *_initializer)
    : ticker({80, 10, 5, 5}), forward(_forward), branch(_branch),
      backward(_backward), initializer(_initializer) {}

BidirectionalSearcher::~BidirectionalSearcher() {
  delete forward;
  delete branch;
  delete backward;
  delete initializer;
}

ref<BidirectionalAction> ForwardOnlySearcher::selectAction() {
  return new ForwardAction(&searcher->selectState());
}

bool ForwardOnlySearcher::empty() { return searcher->empty(); }

void ForwardOnlySearcher::update(ref<ObjectManager::Event> e) {
  if (auto statesEvent = dyn_cast<ObjectManager::States>(e)) {
    assert(!statesEvent->isolated);
    searcher->update(statesEvent->modified, statesEvent->added,
                     statesEvent->removed);
  }
}

ForwardOnlySearcher::ForwardOnlySearcher(Searcher *_searcher) {
  searcher = _searcher;
}

ForwardOnlySearcher::~ForwardOnlySearcher() { delete searcher; }

} // namespace klee
