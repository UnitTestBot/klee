// -*- C++ -*-
#ifndef KLEE_SEARCHERUTIL_H
#define KLEE_SEARCHERUTIL_H

#include "ExecutionState.h"
#include "klee/Expr/Path.h"
#include "klee/Module/KInstruction.h"

namespace klee {

struct BidirectionalAction {
  friend class ref<BidirectionalAction>;

protected:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

public:
  enum class Kind { Forward };

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

} // namespace klee

#endif
