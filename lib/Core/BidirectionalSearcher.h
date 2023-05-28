#ifndef KLEE_BIDIRECTIONALSEARCHER_H
#define KLEE_BIDIRECTIONALSEARCHER_H

#include "Searcher.h"
#include "SearcherUtil.h"

#include "ObjectManager.h"

#include "klee/Module/KModule.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace klee {

class IBidirectionalSearcher : public Subscriber {
public:
  virtual ref<BidirectionalAction> selectAction() = 0;
  virtual bool empty() = 0;
  virtual ~IBidirectionalSearcher() {}
};

class ForwardOnlySearcher : public IBidirectionalSearcher {
public:
  ref<BidirectionalAction> selectAction() override;
  void update(ref<ObjectManager::Event>) override;
  bool empty() override;
  explicit ForwardOnlySearcher(Searcher *searcher);
  ~ForwardOnlySearcher() override;

private:
  Searcher *searcher;
};

} // namespace klee

#endif
