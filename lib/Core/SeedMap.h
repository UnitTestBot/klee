#ifndef KLEE_SEEDMAP_H
#define KLEE_SEEDMAP_H

#include "ExecutionState.h"
#include "ObjectManager.h"
#include "SeedInfo.h"

#include <map>

namespace klee {
class SeedMap : public Subscriber {
private:
  std::map<ExecutionState *, std::vector<ExecutingSeed>> seedMap;

public:
  SeedMap();

  void update(ref<ObjectManager::Event> e) override;

  std::map<ExecutionState *, std::vector<ExecutingSeed>>::iterator
  upper_bound(ExecutionState *state);
  std::map<ExecutionState *, std::vector<ExecutingSeed>>::iterator
  find(ExecutionState *state);
  std::map<ExecutionState *, std::vector<ExecutingSeed>>::iterator end();
  std::map<ExecutionState *, std::vector<ExecutingSeed>>::iterator begin();
  void
  erase(std::map<ExecutionState *, std::vector<ExecutingSeed>>::iterator it);
  void erase(ExecutionState *state);
  void push_back(ExecutionState *state,
                 std::vector<ExecutingSeed>::iterator siit);
  std::size_t count(ExecutionState *state);
  std::vector<ExecutingSeed> &at(ExecutionState *state);
  unsigned size() { return seedMap.size(); }
  bool empty();

  virtual ~SeedMap();
};
} // namespace klee

#endif /*KLEE_SEEDMAP_H*/
