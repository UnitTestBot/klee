#include "BackwardSearcher.h"
#include "ExecutionState.h"
#include "SearcherUtil.h"
#include "llvm/Support/ErrorHandling.h"
#include <algorithm>
#include <climits>
#include <cstddef>
#include <utility>

namespace klee {
bool RecencyRankedSearcher::empty() { return propagations.empty(); }

Propagation RecencyRankedSearcher::selectAction() {
  unsigned leastUsed = UINT_MAX;
  Propagation chosen = {0, 0};
  for (auto i : propagations) {
    if (i.pob->propagationCount[i.state] < leastUsed) {
      leastUsed = i.pob->propagationCount[i.state];
      chosen = i;
      if (leastUsed == 0) {
        break;
      }
    }
  }

  assert(chosen.pob && chosen.state);

  return chosen;
}

void RecencyRankedSearcher::update(const propagations_ty &addedPropagations,
                                   const propagations_ty &removedPropagations) {
  for (auto propagation : removedPropagations) {
    propagations.remove(propagation);
    pausedPropagations.remove(propagation);
  }
  for (auto propagation : addedPropagations) {
    if (propagation.pob->propagationCount[propagation.state] <=
        maxPropagations) {
      propagations.push_back(propagation);
    } else {
      pausedPropagations.push_back(propagation);
    }
  }
}

void RecencyRankedSearcher::update(const pobs_ty &addedPobs,
                                   const pobs_ty &removedPobs) {}

Propagation RandomPathBackwardSearcher::selectAction() {
  // Choose tree
  ProofObligation *current = nullptr;
  unsigned NPropagatable = 0;
  for (auto pob : rootPobs) {
    if (pob->subtreePropagationCount > 0) {
      NPropagatable++;
    }
  }
  auto choice = rng.getInt32() % NPropagatable;
  for (auto pob : rootPobs) {
    if (choice == 0 && pob->subtreePropagationCount > 0) {
      current = pob;
      break;
    } else if (pob->subtreePropagationCount > 0) {
      choice--;
    }
  }

  ProofObligation *chosen = nullptr;
  // Random path
  while (true) {
    unsigned NPropagatable = 0;
    if (!propagations[current].empty()) {
      NPropagatable++;
    }
    for (auto child : current->children) {
      if (child->subtreePropagationCount > 0) {
        NPropagatable++;
      }
    }
    auto choice = rng.getInt32() % NPropagatable;
    if (choice == 0 && !propagations[current].empty()) {
      chosen = current;
      break;
    }
    for (auto child : current->children) {
      if (choice == 0 && child->subtreePropagationCount > 0) {
        current = child;
        break;
      } else if (child->subtreePropagationCount > 0) {
        choice--;
      }
    }
  }
  assert(propagations[chosen].size() > 0);
  choice = rng.getInt32() % propagations[chosen].size();
  for (auto state : propagations[chosen]) {
    if (choice == 0) {
      return {state, chosen};
    } else {
      choice--;
    }
  }
  llvm_unreachable("Must have chosen the state in the loop");
}

void RandomPathBackwardSearcher::update(
    const propagations_ty &addedPropagations,
    const propagations_ty &removedPropagations) {

  for (auto prop : removedPropagations) {
    propagations[prop.pob].erase(prop.state);
    propagationsCount--;
    if (propagations[prop.pob].empty()) {
      auto cur = prop.pob;
      while (cur) {
        cur->subtreePropagationCount--;
        cur = cur->parent;
      }
    }
  }

  for (auto prop : addedPropagations) {
    if (propagations[prop.pob].empty()) {
      auto cur = prop.pob;
      while (cur) {
        cur->subtreePropagationCount++;
        cur = cur->parent;
      }
    }
    propagations[prop.pob].insert(prop.state);
    propagationsCount++;
  }
}

void RandomPathBackwardSearcher::update(const pobs_ty &addedPobs,
                                        const pobs_ty &removedPobs) {
  for (auto pob : addedPobs) {
    if (!pob->parent) {
      rootPobs.insert(pob);
    }
  }

  for (auto pob : removedPobs) {
    if (!pob->parent) {
      rootPobs.erase(pob);
    }
  }
}

bool RandomPathBackwardSearcher::empty() { return propagationsCount == 0; }

InterleavedBackwardSearcher::InterleavedBackwardSearcher(
    const std::vector<BackwardSearcher *> &_searchers) {
  searchers.reserve(_searchers.size());
  for (auto searcher : _searchers)
    searchers.emplace_back(searcher);
}

Propagation InterleavedBackwardSearcher::selectAction() {
  BackwardSearcher *s = searchers[--index].get();
  if (index == 0)
    index = searchers.size();
  return s->selectAction();
}

void InterleavedBackwardSearcher::update(
    const propagations_ty &addedPropagations,
    const propagations_ty &removedPropagations) {
  for (auto &searcher : searchers) {
    searcher->update(addedPropagations, removedPropagations);
  }
  propagationCount += addedPropagations.size();
  propagationCount -= removedPropagations.size();
}

void InterleavedBackwardSearcher::update(const pobs_ty &addedPobs,
                                         const pobs_ty &removedPobs) {
  for (auto &searcher : searchers) {
    searcher->update(addedPobs, removedPobs);
  }
}

bool InterleavedBackwardSearcher::empty() { return propagationCount == 0; }

}; // namespace klee
