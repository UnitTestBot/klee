#ifndef KLEE_OBJECTMANAGER_H
#define KLEE_OBJECTMANAGER_H

#include "ExecutionState.h"
#include "PForest.h"
#include "klee/ADT/Ref.h"
#include "klee/Core/BranchTypes.h"
#include "klee/Module/KModule.h"

#include <set>
#include <vector>

namespace klee {

class Subscriber;

class ObjectManager {
public:
  struct Event {
    friend class ref<Event>;

  protected:
    class ReferenceCounter _refCount;

  public:
    enum class Kind { States };

    Event() = default;
    virtual ~Event() = default;

    virtual Kind getKind() const = 0;
    static bool classof(const Event *) { return true; }
  };

  struct States : public Event {
    friend class ref<States>;
    ExecutionState *modified;
    const std::vector<ExecutionState *> &added;
    const std::vector<ExecutionState *> &removed;

    States(ExecutionState *modified, const std::vector<ExecutionState *> &added,
           const std::vector<ExecutionState *> &removed)
        : modified(modified), added(added), removed(removed) {}

    Kind getKind() const { return Kind::States; }
    static bool classof(const Event *A) { return A->getKind() == Kind::States; }
    static bool classof(const States *) { return true; }
  };

  ObjectManager();
  ~ObjectManager();

  void addSubscriber(Subscriber *);
  void addProcessForest(PForest *);

  void addInitialState(ExecutionState *state);
  void addFirstState(ExecutionState *state);
  
  void setCurrentState(ExecutionState *_current);

  ExecutionState *branchState(ExecutionState *state, BranchType reason);
  void removeState(ExecutionState *state);

  void unseed(ExecutionState *state) {
    if(state->isSeeded) {
      state->isSeeded = false;
      seedChanges.insert(state);
      statesUpdated = true;
    }
  }
  void seed(ExecutionState *state) {
    if(!state->isSeeded) {
      state->isSeeded = true;
      seedChanges.insert(state);
      statesUpdated = true;
    }
  }
  states_ty &getSeedChanges(){
    return seedChanges; 
  }

  ExecutionState *initializeState(KInstruction *location,
                                  std::set<ref<Target>> targets);

  const states_ty &getStates();

  void updateSubscribers();
  void initialUpdate();

private:
  std::vector<Subscriber *> subscribers;
  PForest *processForest;

public:
  states_ty states;

  // These are used to buffer execution results and pass the updates to
  // subscribers

  bool statesUpdated = false;

  ExecutionState *current = nullptr;
  std::vector<ExecutionState *> addedStates;
  std::vector<ExecutionState *> removedStates;

  states_ty seedChanges;
};

class Subscriber {
public:
  virtual void update(ref<ObjectManager::Event> e) = 0;
};

} // namespace klee

#endif /*KLEE_OBJECTMANAGER_H*/
