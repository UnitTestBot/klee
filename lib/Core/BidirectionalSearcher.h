#ifndef KLEE_BIDIRECTIONALSEARCHER_H
#define KLEE_BIDIRECTIONALSEARCHER_H

#include "BackwardSearcher.h"
#include "Initializer.h"
#include "ProofObligation.h"
#include "Searcher.h"
#include "SearcherUtil.h"

#include "ObjectManager.h"

#include "klee/ADT/Ticker.h"
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

class BidirectionalSearcher : public IBidirectionalSearcher {
  enum class StepKind { Forward, Branch, Backward, Initialize };

public:
  ref<BidirectionalAction> selectAction() override;
  void update(ref<ObjectManager::Event> e) override;
  bool empty() override;

  // Assumes ownership
  explicit BidirectionalSearcher(Searcher *_forward, Searcher *_branch,
                                 BackwardSearcher *_backward,
                                 Initializer *_initializer);

  ~BidirectionalSearcher() override;

private:
  Ticker ticker;

  Searcher *forward;
  Searcher *branch;
  BackwardSearcher *backward;
  Initializer *initializer;

private:
  StepKind selectStep();
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
