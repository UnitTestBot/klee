//===-- Target.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Target.h"
#include "ExecutionState.h"

#include "klee/Module/CodeGraphDistance.h"
#include "klee/Module/KInstruction.h"

#include <set>
#include <vector>

using namespace llvm;
using namespace klee;

namespace klee {
llvm::cl::opt<TargetCalculateBy> TargetCalculatorMode(
    "target-calculator-kind", cl::desc("Specifiy the target calculator mode."),
    cl::values(
        clEnumValN(TargetCalculateBy::Default, "default",
                   "Looks for the closest uncovered block."),
        clEnumValN(
            TargetCalculateBy::Blocks, "blocks",
            "Looks for the closest uncovered block by state blocks history."),
        clEnumValN(TargetCalculateBy::Transitions, "transitions",
                   "Looks for the closest uncovered block by state transitions "
                   "history.")),
    cl::init(TargetCalculateBy::Default), cl::cat(ExecCat));


llvm::cl::opt<bool> LocationAccuracy(
    "location-accuracy", cl::init(false),
    cl::desc(
        "Check location with line and column accuracy (default=false)"));
} // namespace klee

std::string Target::toString() const {
  std::ostringstream repr;
  repr << "Target " << getId() << ": ";
  if (shouldFailOnThisTarget()) {
    repr << "error in ";
  }
  repr << block->getAssemblyLocation();
  if (atReturn()) {
    repr << " (at the end)";
  }
  return repr.str();
}

Target::EquivTargetHashSet Target::cachedTargets;
Target::TargetHashSet Target::targets;

ref<Target> Target::getFromCacheOrReturn(Target *target) {
  std::pair<TargetHashSet::const_iterator, bool> success =
      cachedTargets.insert(target);
  if (success.second) {
    // Cache miss
    targets.insert(target);
    return target;
  }
  // Cache hit
  delete target;
  target = *(success.first);
  return target;
}

ref<Target> Target::create(ReachWithError _error, unsigned _id, optional<ErrorLocation> _loc, KBlock *_block) {
  Target *target = new Target(_error, _id, _loc, _block);
  return getFromCacheOrReturn(target);
}

ref<Target> Target::create(KBlock *_block) {
  return create(ReachWithError::None, 0, nonstd::nullopt, _block);
}

bool Target::isTheSameAsIn(KInstruction *instr) const {
  if (!loc.has_value()) {
    return false;
  }
  const auto& errLoc = *loc;
  return instr->info->line >= errLoc.startLine && instr->info->line <= errLoc.endLine &&
         (!LocationAccuracy || !errLoc.startColumn.has_value() || (instr->info->column >= *errLoc.startColumn && instr->info->column <= *errLoc.endColumn));
}

int Target::compare(const Target &other) const {
  if (block != other.block) {
    return block < other.block ? -1 : 1;
  }
  if (error != other.error) {
    return error < other.error ? -1 : 1;
  }
  if (id != other.id) {
    return id < other.id ? -1 : 1;
  }

  return 0;
}

bool Target::equals(const Target &other) const {
  return compare(other) == 0;
}

bool Target::operator<(const Target &other) const {
  return compare(other) == -1;
}

bool Target::operator==(const Target &other) const {
  return equals(other);
}

Target::~Target() {
  if (targets.find(this) != targets.end()) {
    cachedTargets.erase(this);
    targets.erase(this);
  }
}

void TargetCalculator::update(const ExecutionState &state) {
  switch (TargetCalculatorMode) {
  case TargetCalculateBy::Default:
    blocksHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.getInitPCBlock());
    break;

  case TargetCalculateBy::Blocks:
    blocksHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.level.begin(), state.level.end());
    break;

  case TargetCalculateBy::Transitions:
    blocksHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.level.begin(), state.level.end());
    transitionsHistory[state.getInitPCBlock()][state.getPrevPCBlock()].insert(
        state.transitionLevel.begin(), state.transitionLevel.end());
    break;
  }
}

bool TargetCalculator::differenceIsEmpty(
    const ExecutionState &state,
    const std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history,
    KBlock *target) {
  std::vector<BasicBlock *> diff;
  std::set<BasicBlock *> left(state.level.begin(), state.level.end());
  std::set<BasicBlock *> right(history.at(target->basicBlock).begin(),
                               history.at(target->basicBlock).end());
  std::set_difference(left.begin(), left.end(), right.begin(), right.end(),
                      std::inserter(diff, diff.begin()));
  return diff.empty();
}

bool TargetCalculator::differenceIsEmpty(
    const ExecutionState &state,
    const std::unordered_map<llvm::BasicBlock *, VisitedTransitions> &history,
    KBlock *target) {
  std::vector<Transition> diff;
  std::set<Transition> left(state.transitionLevel.begin(),
                            state.transitionLevel.end());
  std::set<Transition> right(history.at(target->basicBlock).begin(),
                             history.at(target->basicBlock).end());
  std::set_difference(left.begin(), left.end(), right.begin(), right.end(),
                      std::inserter(diff, diff.begin()));
  return diff.empty();
}

ref<Target> TargetCalculator::calculate(ExecutionState &state) {
  BasicBlock *initialBlock = state.getInitPCBlock();
  std::unordered_map<llvm::BasicBlock *, VisitedBlocks> &history =
      blocksHistory[initialBlock];
  std::unordered_map<llvm::BasicBlock *, VisitedTransitions>
      &transitionHistory = transitionsHistory[initialBlock];
  BasicBlock *bb = state.getPCBlock();
  KFunction *kf = module.functionMap.at(bb->getParent());
  KBlock *kb = kf->blockMap[bb];
  ref<Target> nearestBlock;
  unsigned int minDistance = UINT_MAX;
  unsigned int sfNum = 0;
  bool newCov = false;
  for (auto sfi = state.stack.rbegin(), sfe = state.stack.rend(); sfi != sfe;
       sfi++, sfNum++) {
    kf = sfi->kf;

    for (const auto &kbd : codeGraphDistance.getSortedDistance(kb)) {
      KBlock *target = kbd.first;
      unsigned distance = kbd.second;
      if ((sfNum > 0 || distance > 0)) {
        if (distance >= minDistance)
          break;
        if (history[target->basicBlock].size() != 0) {
          bool diffIsEmpty = true;
          if (!newCov) {
            switch (TargetCalculatorMode) {
            case TargetCalculateBy::Blocks:
              diffIsEmpty = differenceIsEmpty(state, history, target);
              break;
            case TargetCalculateBy::Transitions:
              diffIsEmpty = differenceIsEmpty(state, transitionHistory, target);
              break;
            case TargetCalculateBy::Default:
              break;
            }
          }

          if (diffIsEmpty) {
            continue;
          }
        } else {
          newCov = true;
        }
        nearestBlock = Target::create(target);
        minDistance = distance;
      }
    }

    if (nearestBlock) {
      return nearestBlock;
    }

    if (sfi->caller) {
      kb = sfi->caller->parent;
    }
  }

  return nearestBlock;
}
