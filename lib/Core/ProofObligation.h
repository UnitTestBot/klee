#ifndef KLEE_PROOFOBLIGATION_H
#define KLEE_PROOFOBLIGATION_H

#include "ExecutionState.h"

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Path.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetForest.h"

#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace klee {

class ProofObligation {
public:
  ProofObligation(ref<Target> _location)
      : id(nextID++), parent(nullptr), root(this), location(_location) {}

  ~ProofObligation() {
    for (auto pob : children) {
      pob->parent = nullptr;
    }
    if (parent) {
      parent->children.erase(this);
    }
  }

  std::set<ProofObligation *> getSubtree();

  bool atReturn() const { return isa<KReturnBlock>(location->getBlock()); }
  std::uint32_t getID() const { return id; };

  static ProofObligation *create(ProofObligation *parent, ExecutionState *state,
                                 PathConstraints &composed);

  static void propagateToReturn(ProofObligation *pob, KInstruction *callSite,
                                KBlock *returnBlock);

private:
  ProofObligation *makeChild(ref<Target> target) {
    auto pob = new ProofObligation(target);
    pob->id = nextID++;
    pob->parent = this;
    pob->root = root;
    pob->propagationCount = propagationCount;
    pob->targetForest = targetForest;
    children.insert(pob);
    return pob;
  }

public:
  std::uint32_t id;
  ProofObligation *parent;
  ProofObligation *root;
  std::set<ProofObligation *> children;
  std::vector<CallStackFrame> stack;
  std::map<ExecutionState *, unsigned, ExecutionStateIDCompare>
      propagationCount;

  ref<Target> location;
  TargetForest targetForest;
  PathConstraints constraints;

private:
  static unsigned nextID;
};

struct ProofObligationIDCompare {
  bool operator()(const ProofObligation *a, const ProofObligation *b) const {
    return a->getID() < b->getID();
  }
};

using pobs_ty = std::set<ProofObligation *, ProofObligationIDCompare>;

} // namespace klee

#endif
