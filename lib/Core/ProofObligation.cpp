#include "ProofObligation.h"

namespace klee {

unsigned ProofObligation::nextID = 0;

std::set<ProofObligation *> ProofObligation::getSubtree() {
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

ProofObligation *ProofObligation::create(ProofObligation *parent,
                                         ExecutionState *state,
                                         PathConstraints &composed) {
  ProofObligation *pob = parent->makeChild(ReachBlockTarget::create(
      state->constraints.path().getBlocks().front().block));
  pob->constraints = composed;
  pob->propagationCount[state]++;
  pob->stack = parent->stack;
  auto statestack = state->stack.callStack();
  while (!pob->stack.empty() && !statestack.empty()) {
    if (statestack.size() == 1) {
      assert(statestack.back().caller == nullptr);
      break;
    }
    auto pobF = pob->stack.back();
    auto stateF = statestack.back();
    assert(pobF == stateF);
    pob->stack.pop_back();
    statestack.pop_back();
  }
  auto history = state->history();
  while (history && history->target) {
    pob->targetForest.stepTo(history->target);
    history = history->next;
  }

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
