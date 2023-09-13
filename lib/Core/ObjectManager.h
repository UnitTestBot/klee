#ifndef KLEE_OBJECTMANAGER_H
#define KLEE_OBJECTMANAGER_H

#include "ExecutionState.h"
#include "PForest.h"
#include "ProofObligation.h"
#include "SearcherUtil.h"
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
    enum class Kind { States, Propagations, ProofObligations, Conflicts };

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
    bool isolated;

    States(ExecutionState *modified, const std::vector<ExecutionState *> &added,
           const std::vector<ExecutionState *> &removed, bool isolated)
        : modified(modified), added(added), removed(removed),
          isolated(isolated) {}

    Kind getKind() const { return Kind::States; }
    static bool classof(const Event *A) { return A->getKind() == Kind::States; }
    static bool classof(const States *) { return true; }
  };

  struct Propagations : public Event {
    friend class ref<Propagations>;
    const propagations_ty &added;
    const propagations_ty &removed;

    Propagations(const propagations_ty &added, const propagations_ty &removed)
        : added(added), removed(removed) {}

    Kind getKind() const { return Kind::Propagations; }
    static bool classof(const Event *A) {
      return A->getKind() == Kind::Propagations;
    }
    static bool classof(const Propagations *) { return true; }
  };

  struct Conflicts : public Event {
    friend class ref<Conflicts>;
    const std::vector<ref<TargetedConflict>> &conflicts;

    Conflicts(const std::vector<ref<TargetedConflict>> &conflicts)
        : conflicts(conflicts) {}

    Kind getKind() const { return Kind::Conflicts; }
    static bool classof(const Event *A) {
      return A->getKind() == Kind::Conflicts;
    }
    static bool classof(const Conflicts *) { return true; }
  };

  struct ProofObligations : public Event {
    friend class ref<ProofObligations>;
    ExecutionState *context;
    const pobs_ty &added;
    const pobs_ty &removed;

    ProofObligations(ExecutionState *context, const pobs_ty &added,
                     const pobs_ty &removed)
        : context(context), added(added), removed(removed) {}

    Kind getKind() const { return Kind::ProofObligations; }
    static bool classof(const Event *A) {
      return A->getKind() == Kind::ProofObligations;
    }
    static bool classof(const ProofObligations *) { return true; }
  };

  ObjectManager();
  ~ObjectManager();

  void addSubscriber(Subscriber *);
  void addProcessForest(PForest *);

  void setEmptyState(ExecutionState *state);
  void addInitialState(ExecutionState *state);
  void clear();

  void setCurrentState(ExecutionState *_current);
  void setContextState(ExecutionState *_context);
  ExecutionState *branchState(ExecutionState *state, BranchType reason);
  void removeState(ExecutionState *state);
  ExecutionState *initializeState(KInstruction *location,
                                  std::set<ref<Target>> targets);

  const states_ty &getStates();
  const states_ty &getIsolatedStates();
  const pobs_ty &getLeafPobs();
  const pobs_ty &getRootPobs();

  void addTargetedConflict(ref<TargetedConflict> conflict);

  void addPob(ProofObligation *pob);
  void removePob(ProofObligation *pob);

  void removePropagation(Propagation prop);

  void updateSubscribers();
  void initialUpdate();

private:
  std::vector<Subscriber *> subscribers;
  PForest *processForest;

  InitializerPredicate *predicate;

public:
  Subscriber *tgms;
  ExecutionState *emptyState;

  std::set<KFunction *> entrypoints;

  states_ty states;
  states_ty isolatedStates;
  pobs_ty leafPobs;
  pobs_ty rootPobs;
  std::map<ref<Target>, states_ty> reachedStates;
  std::map<ref<Target>, pobs_ty> pobs;
  std::map<std::pair<Path, ref<Target>>, ProofObligation *> pathedPobs;
  std::map<ref<Target>, propagations_ty> propagations;
  std::map<ProofObligation *, unsigned> propagationCount;

  // These are used to buffer execution results and pass the updates to
  // subscribers

  bool statesUpdated = false;
  enum class StateKind {
    Regular,
    Isolated,
    None
  } stateUpdateKind = StateKind::None;

  ExecutionState *current = nullptr;
  std::vector<ExecutionState *> addedStates;
  std::vector<ExecutionState *> removedStates;

  ExecutionState *context = nullptr;
  pobs_ty addedPobs;
  pobs_ty removedPobs;

  propagations_ty addedPropagations;
  propagations_ty removedPropagations;

  std::vector<ref<TargetedConflict>> addedTargetedConflicts;

  bool checkStack(ExecutionState *state, ProofObligation *pob);
  void checkReachedStates();
  void checkReachedPobs();

  bool pobExists(ProofObligation *pob) {
    return pathedPobs.count({pob->constraints.path(), pob->location});
  }

  void setPredicate(InitializerPredicate *predicate_) {
    predicate = predicate_;
  }
};

class Subscriber {
public:
  virtual void update(ref<ObjectManager::Event> e) = 0;
};

} // namespace klee

#endif /*KLEE_OBJECTMANAGER_H*/
