#include "Initializer.h"
#include "ProofObligation.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Support/DebugFlags.h"

#include "llvm/IR/Instructions.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <stack>
#include <utility>

namespace klee {

std::pair<KInstruction *, std::set<ref<Target>>>
ConflictCoreInitializer::selectAction() {
  auto KI = queued.front();
  queued.pop_front();
  auto targets = targetMap[KI];
  assert(!targets.empty());
  targetMap.erase(KI);
  for (auto target : targets) {
    instructionMap[target].erase(KI);
  }
  return {KI, targets};
}

bool ConflictCoreInitializer::empty() { return queued.empty(); }

void ConflictCoreInitializer::update(const pobs_ty &added,
                                     const pobs_ty &removed) {
  for (auto i : added) {
    addPob(i);
  }
  for (auto i : removed) {
    removePob(i);
  }
}

void ConflictCoreInitializer::addPob(ProofObligation *pob) {
  auto target = pob->location;
  knownTargets[target]++;
  if (knownTargets[target] > 1) {
    return; // There has been such a target already
  }

  if (pob->location->getBlock()->parent->entryKBlock !=
      pob->location->getBlock()) {
    auto backstep = cgd->getNearestPredicateSatisfying(
        pob->location->getBlock(), PredicateAdapter(predicate), false);

    for (auto from : backstep) {
      auto toBlocks = cgd->getNearestPredicateSatisfying(
          from, PredicateAdapter(predicate), true);
      for (auto to : toBlocks) {
        KInstruction *fromInst =
            (predicate.isInterestingCallBlock(from) ? from->instructions[1]
                                                    : from->instructions[0]);
        addInit(fromInst, ReachBlockTarget::create(to));
      }
      KInstruction *fromInst =
          (predicate.isInterestingCallBlock(from) ? from->instructions[1]
                                                  : from->instructions[0]);
      addInit(fromInst, target);
      // if (!pob->parent && !predicate(pob->location->getBlock())) {
      //   KInstruction *fromInst =
      //       (predicate.isInterestingCallBlock(from) ? from->instructions[1]
      //        : from->instructions[0]);
      //   addInit(fromInst,
      //   ReachBlockTarget::create(pob->location->getBlock()));
      // }
    }
  } else {
    // if (!pob->stack.empty()) {
    //   auto frame = pob->stack.back();
    //   assert(frame.kf == pob->location->getBlock()->parent);
    //   addInit(frame.caller,
    //           ReachBlockTarget::create(pob->location->getBlock()));
    // } else {
    for (auto i : allowed) {
      for (auto kcallblock : i->kCallBlocks) {
        if (kcallblock->calledFunctions.count(
                pob->location->getBlock()->parent)) {
          addInit(kcallblock->getFirstInstruction(),
                  ReachBlockTarget::create(pob->location->getBlock()));
          addInit(kcallblock->getFirstInstruction(), target);
          // }
        }
      }
    }
  }

  std::list<KInstruction *> enqueue;
  for (auto KI : awaiting) {
    if (targetMap[KI].count(target)) {
      enqueue.push_back(KI);
    }
  }

  for (auto KI : enqueue) {
    awaiting.remove(KI);
    queued.push_back(KI);
  }
}

void ConflictCoreInitializer::removePob(ProofObligation *pob) {
  auto target = pob->location;
  assert(knownTargets[target] != 0);
  knownTargets[target]--;

  if (knownTargets[target] > 0) {
    return;
  }

  std::list<KInstruction *> dequeue;
  for (auto KI : queued) {
    bool noKnown = true;
    for (auto target : knownTargets) {
      if (target.second != 0 && targetMap[KI].count(target.first)) {
        noKnown = false;
        break;
      }
    }
    if (noKnown) {
      dequeue.push_back(KI);
    }
  }

  for (auto KI : dequeue) {
    awaiting.push_back(KI);
    queued.remove(KI);
  }
}

void ConflictCoreInitializer::addConflictInit(const Conflict &conflict,
                                              KBlock *target) {
  if (errorGuided) {
    return;
  }

  // auto &blocks = conflict.path.getBlocks();
  // std::set<KFunction *, KFunctionCompare> functions;

  // for (auto block : blocks) {
  //   if (!dismantledFunctions.count(block.block->parent)) {
  //     functions.insert(block.block->parent);
  //     dismantledFunctions.insert(block.block->parent);
  //   }
  // }

  // // Dismantle all functions present in the path
  // for (auto function : functions) {
  //   auto dismantled = cgd->dismantleFunction(function, predicate);
  //   for (auto i : dismantled) {
  //     KInstruction *from =
  //         (RegularFunctionPredicate(i.first) ? i.first->instructions[1]
  //                                            : i.first->instructions[0]);
  //     addInit(from, ReachBlockTarget::create(i.second));
  //   }
  // }

  // // Bridge calls
  // for (auto function : functions) {
  //   for (auto &block : function->blocks) {
  //     if (RegularFunctionPredicate(block.get())) {
  //       auto call = dyn_cast<KCallBlock>(block.get());
  //       auto called = call->getKFunction();
  //       addInit(call->getFirstInstruction(),
  //               ReachBlockTarget::create(called->entryKBlock, false));
  //     }
  //   }
  // }

  // auto targetB = cgd->getNearestPredicateSatisfying(target, predicate,
  // false); if (target != targetB) {
  //   KInstruction *from =
  //       (RegularFunctionPredicate(targetB) ? targetB->instructions[1]
  //                                          : targetB->instructions[0]);
  //   addInit(from, ReachBlockTarget::create(target));
  // }
}

void ConflictCoreInitializer::initializeFunctions(
    std::set<KFunction *, KFunctionCompare> functions) {
  allowed = functions;
  // for (auto function : functions) {
  //   if (dismantledFunctions.count(function)) {
  //     continue;
  //   }
  //   dismantledFunctions.insert(function);

  //   auto dismantled = cgd->dismantleFunction(function, predicate);
  //   for (auto i : dismantled) {
  //     KInstruction *from =
  //         (RegularFunctionPredicate(i.first) ? i.first->instructions[1]
  //                                            : i.first->instructions[0]);
  //     addInit(from, ReachBlockTarget::create(i.second));
  //   }
  //   for (auto &block : function->blocks) {
  //     if (RegularFunctionPredicate(block.get())) {
  //       auto call = dyn_cast<KCallBlock>(block.get());
  //       auto called = call->getKFunction();
  //       addInit(call->getFirstInstruction(),
  //               ReachBlockTarget::create(called->entryKBlock, false));
  //     }
  //   }
  // }
}

void ConflictCoreInitializer::addErrorInit(ref<Target> errorTarget) {
  auto errorT = dyn_cast<ReproduceErrorTarget>(errorTarget);
  auto location = errorTarget->getBlock();
  // Check direction
  std::set<KBlock *, KBlockCompare> nearest;
  if (predicate(errorTarget->getBlock()) && !errorT->isThatError(Reachable)) {
    nearest.insert(errorTarget->getBlock()); // HOT FIX
  } else {
    nearest = cgd->getNearestPredicateSatisfying(
        location, PredicateAdapter(predicate), false);
  }
  for (auto i : nearest) {
    KInstruction *from =
        (predicate.isInterestingCallBlock(i) ? i->instructions[1]
                                             : i->instructions[0]);
    auto toBlocks = cgd->getNearestPredicateSatisfying(
        i, PredicateAdapter(predicate), true);
    for (auto to : toBlocks) {
      addInit(from, ReachBlockTarget::create(to));
    }
    if (errorT->isThatError(Reachable)) {
      addInit(from, ReachBlockTarget::create(location));
    } else {
      addInit(from, errorTarget);
    }
  }
}

void ConflictCoreInitializer::addInit(KInstruction *from, ref<Target> to) {
  if (initialized[from].count(to)) {
    return;
  }
  initialized[from].insert(to);

  if (debugPrints.isSet(DebugPrint::Init)) {
    llvm::errs() << "[initializer] From " << from->toString() << " to "
                 << to->toString() << " scheduled\n";
  }

  targetMap[from].insert(to);
  instructionMap[to].insert(from);
  bool awaits =
      (std::find(awaiting.begin(), awaiting.end(), from) != awaiting.end());
  bool enqueued =
      (std::find(queued.begin(), queued.end(), from) != queued.end());

  if (!awaits && !enqueued) {
    if (knownTargets.count(to)) {
      queued.push_back(from);
    } else {
      awaiting.push_back(from);
    }
  } else if (awaits) {
    if (knownTargets.count(to)) {
      awaiting.remove(from);
      queued.push_back(from);
    }
  }
}

}; // namespace klee
