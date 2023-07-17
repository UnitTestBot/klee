#include "BackwardSearcher.h"
#include "ExecutionState.h"
#include "SearcherUtil.h"
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

}; // namespace klee
