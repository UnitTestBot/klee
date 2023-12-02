// -*- C++ -*-
#ifndef KLEE_INITIALIZER_H
#define KLEE_INITIALIZER_H

#include "ProofObligation.h"
#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include <list>
#include <queue>
#include <set>

namespace klee {
struct Conflict;

class Initializer {
public:
  virtual ~Initializer() {}
  virtual std::pair<KInstruction *, std::set<ref<Target>>> selectAction() = 0;
  virtual bool empty() = 0;
  virtual void update(const pobs_ty &added, const pobs_ty &removed) = 0;
  virtual void addConflictInit(const Conflict &, KBlock *) = 0;
};

class ConflictCoreInitializer : public Initializer {
public:
  std::pair<KInstruction *, std::set<ref<Target>>> selectAction() override;
  bool empty() override;

  bool initsLeftForTarget(ref<Target> t) {
    return instructionMap.count(t) && !instructionMap.at(t).empty();
  }

  void addConflictInit(const Conflict &, KBlock *) override;

  void initializeFunctions(std::set<KFunction *, KFunctionCompare> functions);
  void addErrorInit(ref<Target> errorTarget);

  void update(const pobs_ty &added, const pobs_ty &removed) override;

  explicit ConflictCoreInitializer(CodeGraphInfo *cgd,
                                   InitializerPredicate &predicate,
                                   bool errorGuided)
      : cgd(cgd), predicate(predicate), errorGuided(errorGuided){};

  ~ConflictCoreInitializer() override {}

private:
  CodeGraphInfo *cgd;
  InitializerPredicate &predicate;
  bool errorGuided;

  // There are proof obligation in these targets
  std::map<ref<Target>, unsigned> knownTargets;

  // Targets collected for each initial instruction
  std::map<KInstruction *, std::set<ref<Target>>, KInstructionCompare>
      targetMap;

  // Reverse
  std::map<ref<Target>, std::set<KInstruction *, KInstructionCompare>>
      instructionMap;

  // awaiting until the are proof obligations in one of their targets
  std::list<KInstruction *> awaiting;

  // There are currently proof obligations in their targets so they are
  // queued for dispatch
  std::list<KInstruction *> queued;

  // For every (KI, Target) pair in this map, there is a state that starts
  // at KI and has Target as one of its targets.
  std::map<KInstruction *, std::set<ref<Target>>, KInstructionCompare>
      initialized;

  // Already dismantled functions don't need to be dismantled again
  std::unordered_set<KFunction *> dismantledFunctions;

  std::set<KFunction *, KFunctionCompare> allowed;

  void addInit(KInstruction *from, ref<Target> to);
  void addPob(ProofObligation *pob);
  void removePob(ProofObligation *pob);
};

}; // namespace klee

#endif
