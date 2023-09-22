#include "ProofObligation.h"

namespace klee {
bool ProofObligationIDCompare::operator()(const ProofObligation *a,
                                          const ProofObligation *b) const {
  return a->getID() < b->getID();
}

unsigned ProofObligation::nextID = 0;

pobs_ty ProofObligation::getSubtree() {
  pobs_ty subtree;
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

ProofObligation *ProofObligation::create(ProofObligation *parent,
                                         ExecutionState *state,
                                         PathConstraints &composed,
                                         ref<Expr> nullPointerExpr) {
  auto &statePath = state->constraints.path();
  auto place = statePath.getBlocks().empty()
                   ? statePath.getNext()->parent
                   : statePath.getBlocks().front().block;
  ProofObligation *pob =
      parent->makeChild(ReachBlockTarget::create(place, false));
  pob->constraints = composed;
  pob->propagationCount[state]++;
  pob->stack = parent->stack;
  auto statestack = state->stack.callStack();
  CallStackFrame::subtractFrames(pob->stack, statestack);
  pob->nullPointerExpr = nullPointerExpr;

  return pob;
}

void ProofObligation::propagateToReturn(ProofObligation *pob,
                                        KInstruction *callSite,
                                        KBlock *returnBlock) {
  // Check that location is correct
  pob->stack.push_back({callSite, returnBlock->parent});
  pob->location = ReachBlockTarget::create(returnBlock);
}

} // namespace klee
