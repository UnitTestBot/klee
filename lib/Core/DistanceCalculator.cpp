//===-- DistanceCalculator.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "DistanceCalculator.h"
#include "ExecutionState.h"
#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/Target.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
DISABLE_WARNING_POP

#include <limits>

using namespace llvm;
using namespace klee;

bool DistanceResult::operator<(const DistanceResult &b) const {
  if (isInsideFunction != b.isInsideFunction)
    return isInsideFunction;
  if (result == WeightResult::Continue && b.result == WeightResult::Continue)
    return weight < b.weight;
  return result < b.result;
}

std::string DistanceResult::toString() const {
  std::ostringstream out;
  out << "(" << (int)!isInsideFunction << ", " << (int)result << ", " << weight
      << ")";
  return out.str();
}

unsigned DistanceCalculator::SpeculativeState::computeHash() {
  unsigned res =
      (reinterpret_cast<uintptr_t>(kb) * SymbolicSource::MAGIC_HASH_CONSTANT) +
      kind;
  res = res * SymbolicSource::MAGIC_HASH_CONSTANT + reversed;
  hashValue = res;
  return hashValue;
}

DistanceResult DistanceCalculator::getDistance(const ExecutionState &state,
                                               KBlock *target) {
  assert(state.pc);
  // In "br" inst of call block
  if (isa<KCallBlock>(state.pc->parent) && state.pc->getIndex() == 1) {
    auto nextBB =
        state.pc->parent->basicBlock->getTerminator()->getSuccessor(0);
    auto nextKB = state.pc->parent->parent->blockMap.at(nextBB);
    return getDistance(nextKB, state.stack.callStack(), target, false);
  }
  return getDistance(state.pc->parent, state.stack.callStack(), target, false);
}

DistanceResult DistanceCalculator::getDistance(const ProofObligation &pob,
                                               KBlock *target) {
  return getDistance(pob.location->getBlock(), pob.stack, target, true);
}

DistanceResult DistanceCalculator::getDistance(KBlock *kb, TargetKind kind,
                                               KBlock *target, bool reversed) {
  SpeculativeState specState(kb, kind, reversed);
  if (distanceResultCache.count(target) == 0 ||
      distanceResultCache.at(target).count(specState) == 0) {
    auto result = computeDistance(kb, kind, target, reversed);
    distanceResultCache[target][specState] = result;
  }
  return distanceResultCache.at(target).at(specState);
}

DistanceResult DistanceCalculator::computeDistance(KBlock *kb, TargetKind kind,
                                                   KBlock *target,
                                                   bool reversed) const {
  const auto &distanceToTargetFunction =
      reversed ? codeGraphInfo.getDistance(target->parent)
               : codeGraphInfo.getBackwardDistance(target->parent);
  weight_type weight = 0;
  WeightResult res = Miss;
  bool isInsideFunction = true;
  switch (kind) {
  case LocalTarget:
    res = tryGetTargetWeight(kb, weight, target, reversed);
    break;

  case PreTarget:
    res = tryGetPreTargetWeight(kb, weight, distanceToTargetFunction, target,
                                reversed);
    isInsideFunction = false;
    break;

  case PostTarget:
    res = tryGetPostTargetWeight(kb, weight, target, reversed);
    isInsideFunction = false;
    break;

  case NoneTarget:
    break;
  }
  return DistanceResult(res, weight, isInsideFunction);
}

DistanceResult
DistanceCalculator::getDistance(KBlock *pcBlock,
                                const ExecutionStack::call_stack_ty &frames,
                                KBlock *target, bool reversed) {
  KBlock *kb = pcBlock;
  const auto &distanceToTargetFunction =
      reversed ? codeGraphInfo.getDistance(target->parent)
               : codeGraphInfo.getBackwardDistance(target->parent);
  unsigned int minCallWeight = UINT_MAX, minSfNum = UINT_MAX, sfNum = 0;
  auto sfi = frames.rbegin(), sfe = frames.rend();
  bool strictlyAfterKB =
      sfi != sfe && sfi->kf->parent->inMainModule(*sfi->kf->function);
  for (; sfi != sfe; sfi++) {
    unsigned callWeight;
    if (distanceInCallGraph(sfi->kf, kb, callWeight, distanceToTargetFunction,
                            target, strictlyAfterKB && sfNum != 0, reversed)) {
      callWeight *= 2;
      callWeight += sfNum;

      if (callWeight < UINT_MAX) {
        minCallWeight = callWeight;
        minSfNum = sfNum;
      }
    }

    if (sfi->caller) {
      kb = sfi->caller->parent;
    }
    sfNum++;

    if (minCallWeight < UINT_MAX)
      break;
  }

  if (minCallWeight == UINT_MAX && reversed) {
    if (distanceToTargetFunction.count(pcBlock->parent)) {
      minCallWeight = 2 * distanceToTargetFunction.at(pcBlock->parent) + sfNum;
      minSfNum = sfNum;
      if (minSfNum == 0) {
        minSfNum = 1;
      }
    }
  }

  TargetKind kind = NoneTarget;
  if (minCallWeight == 0) {
    kind = LocalTarget;
  } else if (minSfNum == 0) {
    kind = PreTarget;
  } else if (minSfNum != UINT_MAX) {
    kind = PostTarget;
  }

  return getDistance(pcBlock, kind, target, reversed);
}

bool DistanceCalculator::distanceInCallGraph(
    KFunction *kf, KBlock *origKB, unsigned int &distance,
    const std::unordered_map<KFunction *, unsigned int>
        &distanceToTargetFunction,
    KBlock *target, bool strictlyAfterKB, bool reversed) const {
  distance = UINT_MAX;
  const std::unordered_map<KBlock *, unsigned> &dist =
      reversed ? codeGraphInfo.getBackwardDistance(origKB)
               : codeGraphInfo.getDistance(origKB);
  KBlock *targetBB = target;
  KFunction *targetF = targetBB->parent;

  if (kf == targetF && dist.count(targetBB) != 0) {
    distance = 0;
    return true;
  }

  if (!strictlyAfterKB)
    return distanceInCallGraph(kf, origKB, distance, distanceToTargetFunction,
                               target, reversed);
  auto min_distance = UINT_MAX;
  distance = UINT_MAX;
  if (reversed) {
    for (auto bb : (predecessors(origKB->basicBlock))) {
      auto kb = kf->blockMap[bb];
      distanceInCallGraph(kf, kb, distance, distanceToTargetFunction, target,
                          reversed);
      if (distance < min_distance)
        min_distance = distance;
    }
  } else {
    for (auto bb : (successors(origKB->basicBlock))) {
      auto kb = kf->blockMap[bb];
      distanceInCallGraph(kf, kb, distance, distanceToTargetFunction, target,
                          reversed);
      if (distance < min_distance)
        min_distance = distance;
    }
  }
  distance = min_distance;
  return distance != UINT_MAX;
}

bool DistanceCalculator::distanceInCallGraph(
    KFunction *kf, KBlock *kb, unsigned int &distance,
    const std::unordered_map<KFunction *, unsigned int>
        &distanceToTargetFunction,
    KBlock *target, bool reversed) const {
  distance = UINT_MAX;
  const std::unordered_map<KBlock *, unsigned> &dist =
      reversed ? codeGraphInfo.getBackwardDistance(kb)
               : codeGraphInfo.getDistance(kb);

  for (auto &kCallBlock : kf->kCallBlocks) {
    if (dist.count(kCallBlock) == 0)
      continue;
    for (auto &calledFunction : kCallBlock->calledFunctions) {
      KFunction *calledKFunction = kf->parent->functionMap[calledFunction];
      if (distanceToTargetFunction.count(calledKFunction) != 0 &&
          distance > distanceToTargetFunction.at(calledKFunction) + 1) {
        distance = distanceToTargetFunction.at(calledKFunction) + 1;
      }
    }
  }
  return distance != UINT_MAX;
}

WeightResult
DistanceCalculator::tryGetLocalWeight(KBlock *kb, weight_type &weight,
                                      const std::vector<KBlock *> &localTargets,
                                      KBlock *target, bool reversed) const {
  KFunction *currentKF = kb->parent;
  KBlock *currentKB = kb;
  const std::unordered_map<KBlock *, unsigned> &dist =
      reversed ? codeGraphInfo.getBackwardDistance(currentKB)
               : codeGraphInfo.getDistance(currentKB);
  weight = UINT_MAX;
  for (auto &end : localTargets) {
    if (dist.count(end) > 0) {
      unsigned int w = dist.at(end);
      weight = std::min(w, weight);
    }
  }

  if (weight == UINT_MAX)
    return Miss;
  if (weight == 0) {
    return Done;
  }

  return Continue;
}

WeightResult DistanceCalculator::tryGetPreTargetWeight(
    KBlock *kb, weight_type &weight,
    const std::unordered_map<KFunction *, unsigned int>
        &distanceToTargetFunction,
    KBlock *target, bool reversed) const {
  KFunction *currentKF = kb->parent;
  std::vector<KBlock *> localTargets;
  for (auto &kCallBlock : currentKF->kCallBlocks) {
    for (auto &calledFunction : kCallBlock->calledFunctions) {
      KFunction *calledKFunction =
          currentKF->parent->functionMap[calledFunction];
      if (distanceToTargetFunction.count(calledKFunction) > 0) {
        localTargets.push_back(kCallBlock);
      }
    }
  }

  if (localTargets.empty())
    return Miss;

  WeightResult res =
      tryGetLocalWeight(kb, weight, localTargets, target, reversed);
  return res == Done ? Continue : res;
}

WeightResult DistanceCalculator::tryGetPostTargetWeight(KBlock *kb,
                                                        weight_type &weight,
                                                        KBlock *target,
                                                        bool reversed) const {
  KFunction *currentKF = kb->parent;

  if (!reversed && currentKF->returnKBlocks.empty())
    return Miss;

  WeightResult res =
      reversed ? tryGetLocalWeight(kb, weight, {currentKF->entryKBlock}, target,
                                   true)
               : tryGetLocalWeight(kb, weight, currentKF->returnKBlocks, target,
                                   false);
  return res == Done ? Continue : res;
}

WeightResult DistanceCalculator::tryGetTargetWeight(KBlock *kb,
                                                    weight_type &weight,
                                                    KBlock *target,
                                                    bool reversed) const {
  std::vector<KBlock *> localTargets = {target};
  WeightResult res =
      tryGetLocalWeight(kb, weight, localTargets, target, reversed);
  return res;
}
