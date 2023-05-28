#include "BidirectionalSearcher.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "ObjectManager.h"
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

namespace klee {

ref<BidirectionalAction> ForwardOnlySearcher::selectAction() {
  return new ForwardAction(&searcher->selectState());
}

bool ForwardOnlySearcher::empty() { return searcher->empty(); }

void ForwardOnlySearcher::update(ref<ObjectManager::Event> e) {
  if (auto statesEvent = dyn_cast<ObjectManager::States>(e)) {
    searcher->update(statesEvent->modified, statesEvent->added,
                     statesEvent->removed);
  }
}

ForwardOnlySearcher::ForwardOnlySearcher(Searcher *_searcher) {
  searcher = _searcher;
}

ForwardOnlySearcher::~ForwardOnlySearcher() {}

} // namespace klee
