// -*- C++ -*-
#ifndef KLEE_SEARCHERUTIL_H
#define KLEE_SEARCHERUTIL_H

#include "ExecutionState.h"
#include "ProofObligation.h"
#include "klee/Expr/Path.h"
#include "klee/Module/KInstruction.h"

namespace klee {

struct Propagation {
  ExecutionState *state;
  ProofObligation *pob;

  Propagation(ExecutionState *_state, ProofObligation *_pob)
      : state(_state), pob(_pob) {}

  bool operator==(const Propagation &rhs) const {
    return state == rhs.state && pob == rhs.pob;
  }

  bool operator<(const Propagation &rhs) const {
    return state->id < rhs.state->id ||
           (state->id == rhs.state->id && pob->id < rhs.pob->id);
  }
};

struct PropagationIDCompare {
  bool operator()(const Propagation &a, const Propagation &b) const {
    return a.state->getID() < b.state->getID() ||
           (a.state->getID() == b.state->getID() &&
            a.pob->getID() < b.pob->getID());
  }
};

using propagations_ty = std::set<Propagation, PropagationIDCompare>;

struct BidirectionalAction {
  friend class ref<BidirectionalAction>;

protected:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

public:
  enum class Kind { Initialize, Forward, Backward };

  BidirectionalAction() = default;
  virtual ~BidirectionalAction() = default;

  virtual Kind getKind() const = 0;

  static bool classof(const BidirectionalAction *) { return true; }
};

struct ForwardAction : public BidirectionalAction {
  friend class ref<ForwardAction>;

  ExecutionState *state;

  ForwardAction(ExecutionState *_state) : state(_state) {}

  Kind getKind() const { return Kind::Forward; }
  static bool classof(const BidirectionalAction *A) {
    return A->getKind() == Kind::Forward;
  }
  static bool classof(const ForwardAction *) { return true; }
};

struct BackwardAction : public BidirectionalAction {
  friend class ref<BackwardAction>;

  Propagation prop;

  BackwardAction(Propagation prop) : prop(prop) {}

  Kind getKind() const { return Kind::Backward; }
  static bool classof(const BidirectionalAction *A) {
    return A->getKind() == Kind::Backward;
  }
  static bool classof(const BackwardAction *) { return true; }
};

struct InitializeAction : public BidirectionalAction {
  friend class ref<InitializeAction>;

  KInstruction *location;
  std::set<ref<Target>> targets;

  InitializeAction(KInstruction *_location, std::set<ref<Target>> _targets)
      : location(_location), targets(_targets) {}

  Kind getKind() const { return Kind::Initialize; }
  static bool classof(const BidirectionalAction *A) {
    return A->getKind() == Kind::Initialize;
  }
  static bool classof(const InitializeAction *) { return true; }
};

} // namespace klee

#endif
