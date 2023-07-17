// -*- C++ -*-
#ifndef KLEE_INITIALIZER_H
#define KLEE_INITIALIZER_H

#include "ProofObligation.h"
#include "klee/Module/CodeGraphDistance.h"
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

  void addConflictInit(const Conflict &, KBlock *) override;
  void update(const pobs_ty &added, const pobs_ty &removed) override;

  explicit ConflictCoreInitializer(CodeGraphDistance *cgd) : cgd(cgd){};
  ~ConflictCoreInitializer() override {}

private:
  CodeGraphDistance *cgd;

  // There are proof obligation in these targets
  std::map<ref<Target>, unsigned> knownTargets;

  // Targets collected for each initial instruction
  std::map<KInstruction *, std::set<ref<Target>>> targetMap;

  // awaiting until the are proof obligations in one of their targets
  std::list<KInstruction *> awaiting;

  // There are currently proof obligations in their targets so they are
  // queued for dispatch
  std::list<KInstruction *> queued;

  // For every (KI, Target) pair in this map, there is a state that starts
  // at KI and has Target as one of its targets.
  std::map<KInstruction *, std::set<ref<Target>>> initialized;

  void addInit(KInstruction *from, ref<Target> to);
  void addPob(ProofObligation *pob);
  void removePob(ProofObligation *pob);
};

}; // namespace klee

#endif
