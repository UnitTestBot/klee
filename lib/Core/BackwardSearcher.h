// -*- C++ -*-
#ifndef KLEE_BACKWARDSEARCHER_H
#define KLEE_BACKWARDSEARCHER_H

#include "ExecutionState.h"
#include "ProofObligation.h"
#include "SearcherUtil.h"

#include <list>

namespace klee {

class BackwardSearcher {
public:
  virtual ~BackwardSearcher() = default;
  virtual Propagation selectAction() = 0;
  virtual void update(const propagations_ty &addedPropagations,
                      const propagations_ty &removedPropagations) = 0;
  virtual void update(const pobs_ty &addedPobs, const pobs_ty &removedPobs) = 0;
  virtual bool empty() = 0;
};

class RecencyRankedSearcher : public BackwardSearcher {
private:
  unsigned maxPropagations;

public:
  RecencyRankedSearcher(unsigned _maxPropagation)
      : maxPropagations(_maxPropagation) {}
  Propagation selectAction() override;
  void update(const propagations_ty &addedPropagations,
              const propagations_ty &removedPropagations) override;
  void update(const pobs_ty &addedPobs, const pobs_ty &removedPobs) override;
  bool empty() override;

private:
  std::list<Propagation> propagations;
  std::list<Propagation> pausedPropagations;
};

class RandomPathBackwardSearcher : public BackwardSearcher {
public:
  RandomPathBackwardSearcher(RNG &rng) : rng(rng) {}
  Propagation selectAction() override;
  void update(const propagations_ty &addedPropagations,
              const propagations_ty &removedPropagations) override;
  void update(const pobs_ty &addedPobs, const pobs_ty &removedPobs) override;
  bool empty() override;

private:
  unsigned propagationsCount = 0;
  pobs_ty rootPobs;
  std::map<ProofObligation *, std::set<ExecutionState *>> propagations;
  RNG &rng;
};

class InterleavedBackwardSearcher : public BackwardSearcher {
  unsigned propagationCount = 0;
  std::vector<std::unique_ptr<BackwardSearcher>> searchers;
  unsigned index{1};

public:
  explicit InterleavedBackwardSearcher(
      const std::vector<BackwardSearcher *> &searchers);
  ~InterleavedBackwardSearcher() override = default;
  Propagation selectAction() override;
  void update(const propagations_ty &addedPropagations,
              const propagations_ty &removedPropagations) override;
  void update(const pobs_ty &addedPobs, const pobs_ty &removedPobs) override;
  bool empty() override;
};

}; // namespace klee

#endif
