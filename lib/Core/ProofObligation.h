#ifndef KLEE_PROOFOBLIGATION_H
#define KLEE_PROOFOBLIGATION_H

#include "ExecutionState.h"

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Path.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"

#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace klee {

class MemoryObject;

class ProofObligation {

private:
  static unsigned nextID;

public:
  const std::uint32_t id;
  ProofObligation *parent;
  ProofObligation *root;
  std::set<ProofObligation *> children;
  std::vector<stackframe_ty> stack;
  std::map<ExecutionState *, unsigned, ExecutionStateIDCompare>
      propagationCount;

  ref<Target> location;
  PathConstraints constraints;

  std::vector<std::pair<ref<const MemoryObject>, const Array *>>
      sourcedSymbolics;

  ProofObligation(KBlock *_location)
      : id(nextID++), parent(nullptr), root(this),
        location(Target::create(_location)) {
    if (!location->atReturn()) {
      constraints.advancePath(_location->getFirstInstruction());
    }
  }

  ProofObligation(PathConstraints &pconstraint, ProofObligation &_parent,
                  KBlock *_location = nullptr)
      : id(nextID++), parent(&_parent), root(parent->root),
        stack(pconstraint.path().getStack(true)),
        propagationCount(parent->propagationCount),
        location(_location
                     ? Target::create(_location)
                     : Target::create(pconstraint.path().getBlocks().front())),
        constraints(pconstraint) {
    parent->children.insert(this);
  }

  ~ProofObligation() {
    for (auto pob : children) {
      pob->parent = nullptr;
    }
    if (parent) {
      parent->children.erase(this);
    }
  }

  std::set<ProofObligation *> getSubtree() {
    std::set<ProofObligation *> subtree;
    std::queue<ProofObligation *> queue;
    queue.push(this);
    while (!queue.empty()) {
      auto current = queue.front();
      queue.pop();
      subtree.insert(current);
      for (auto pob : current->children) {
        queue.push(pob);
      }
    }
    return subtree;
  }

  bool atReturn() const { return isa<KReturnBlock>(location->getBlock()); }
  std::uint32_t getID() const { return id; };
  std::string print() const;
};

struct ProofObligationIDCompare {
  bool operator()(const ProofObligation *a, const ProofObligation *b) const {
    return a->getID() < b->getID();
  }
};

using pobs_ty = std::set<ProofObligation *, ProofObligationIDCompare>;

ProofObligation *propagateToReturn(ProofObligation *pob, KInstruction *callSite,
                                   KBlock *returnBlock);

} // namespace klee

#endif
