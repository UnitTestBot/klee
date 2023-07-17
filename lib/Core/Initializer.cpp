#include "Initializer.h"
#include "ProofObligation.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
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
  auto &blocks = conflict.path.getBlocks();
  std::vector<KFunction *> functions;

  for (auto block : blocks) {
    if (std::find(functions.begin(), functions.end(), block->parent) ==
        functions.end()) {
      functions.push_back(block->parent);
    }
  }

  // Dismantle all functions present in the path
  for (auto function : functions) {
    auto dismantled =
        cgd->dismantle(function->entryKBlock, function->returnKBlocks);
    for (auto i : dismantled) {
      KInstruction *from =
          (isa<KCallBlock>(i.first) &&
                   isa<llvm::CallInst>(
                       cast<KCallBlock>(i.first)->kcallInstruction->inst)
               ? i.first->instructions[1]
               : i.first->instructions[0]);
      addInit(from, Target::create(i.second));
    }
  }

  // Go through all the present functions and bridge calls if they lead to
  // a present function (itself included)
  for (auto function : functions) {
    for (auto &block : function->blocks) {
      if (auto call = dyn_cast<KCallBlock>(block.get())) {
        auto called = call->getKFunction();
        if (std::find(functions.begin(), functions.end(), called) !=
            functions.end()) {
          addInit(call->getFirstInstruction(), Target::create(called->entryKBlock));
        }
      }
    }
  }

  // Dismantle to target
  auto dismantled = cgd->dismantle(target->parent->entryKBlock, {target});
  for (auto i : dismantled) {
    KInstruction *from =
        (isa<KCallBlock>(i.first) &&
                 isa<llvm::CallInst>(
                     cast<KCallBlock>(i.first)->kcallInstruction->inst)
             ? i.first->instructions[1]
             : i.first->instructions[0]);
    addInit(from, Target::create(i.second));
  }
}

void ConflictCoreInitializer::addInit(KInstruction *from, ref<Target> to) {
  if (initialized[from].count(to)) {
    return;
  }
  initialized[from].insert(to);

  if (debugPrints.isSet(DebugPrint::Init)) {
    llvm::errs() << "[initializer] From " << from->toString() << " to "
                 << to->toString() << "\n";
  }

  targetMap[from].insert(to);
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
