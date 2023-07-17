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
  bool empty() override;

private:
  std::list<Propagation> propagations;
  std::list<Propagation> pausedPropagations;
};

}; // namespace klee

#endif
