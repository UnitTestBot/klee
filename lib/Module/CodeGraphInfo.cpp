//===-- CodeGraphInfo.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/CodeGraphInfo.h"
#include "klee/Module/KModule.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/CFG.h"
DISABLE_WARNING_POP

#include <deque>
#include <queue>
#include <unordered_map>

using namespace klee;
using namespace llvm;

void CodeGraphInfo::calculateDistance(KBlock *bb) {
  auto &dist = blockDistance[bb];
  auto &sort = blockSortedDistance[bb];
  std::deque<KBlock *> nodes;
  nodes.push_back(bb);
  dist[bb] = 0;
  sort.push_back({bb, 0});
  bool hasCycle = false;
  for (; !nodes.empty(); nodes.pop_front()) {
    auto currBB = nodes.front();
    for (auto succ : currBB->successors()) {
      if (succ == bb) {
        hasCycle = true;
        continue;
      }
      if (dist.count(succ))
        continue;
      auto d = dist[currBB] + 1;
      dist.emplace(succ, d);
      sort.push_back({succ, d});
      nodes.push_back(succ);
    }
  }
  if (hasCycle)
    blockCycles.insert(bb);
}

void CodeGraphInfo::calculateBackwardDistance(KBlock *bb) {
  auto &bdist = blockBackwardDistance[bb];
  auto &bsort = blockSortedBackwardDistance[bb];
  std::deque<KBlock *> nodes;
  nodes.push_back(bb);
  bdist[bb] = 0;
  bsort.push_back({bb, 0});
  while (!nodes.empty()) {
    auto currBB = nodes.front();
    for (auto const &pred : currBB->predecessors()) {
      if (bdist.count(pred) == 0) {
        bdist[pred] = bdist[currBB] + 1;
        bsort.push_back({pred, bdist[currBB] + 1});
        nodes.push_back(pred);
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateDistance(KFunction *kf) {
  auto &dist = functionDistance[kf];
  auto &sort = functionSortedDistance[kf];
  std::deque<KFunction *> nodes;
  nodes.push_back(kf);
  dist[kf] = 0;
  sort.push_back({kf, 0});
  while (!nodes.empty()) {
    auto currKF = nodes.front();
    for (auto callBlock : currKF->kCallBlocks) {
      for (auto calledFunction : callBlock->calledFunctions) {
        if (!calledFunction || calledFunction->function->isDeclaration())
          continue;
        if (dist.count(calledFunction) == 0) {
          auto d = dist[currKF] + 1;
          dist[calledFunction] = d;
          sort.emplace_back(calledFunction, d);
          nodes.push_back(calledFunction);
        }
      }
    }
    nodes.pop_front();
  }
}

void CodeGraphInfo::calculateBackwardDistance(KFunction *kf) {
  auto &callMap = kf->parent->callMap;
  auto &bdist = functionBackwardDistance[kf];
  auto &bsort = functionSortedBackwardDistance[kf];
  std::deque<KFunction *> nodes = {kf};
  bdist[kf] = 0;
  bsort.emplace_back(kf, 0);
  for (; !nodes.empty(); nodes.pop_front()) {
    auto currKF = nodes.front();
    for (auto cf : callMap[currKF]) {
      if (cf->function->isDeclaration())
        continue;
      auto it = bdist.find(cf);
      if (it == bdist.end()) {
        auto d = bdist[currKF] + 1;
        bdist.emplace_hint(it, cf, d);
        bsort.emplace_back(cf, d);
        nodes.push_back(cf);
      }
    }
  }
}

void CodeGraphInfo::calculateFunctionBranches(KFunction *kf) {
  std::map<KBlock *, std::set<unsigned>> &fbranches = functionBranches[kf];
  for (auto &kb : kf->blocks) {
    fbranches[kb.get()];
    for (unsigned branch = 0;
         branch < kb->basicBlock->getTerminator()->getNumSuccessors();
         ++branch) {
      fbranches[kb.get()].insert(branch);
    }
  }
}
void CodeGraphInfo::calculateFunctionConditionalBranches(KFunction *kf) {
  std::map<KBlock *, std::set<unsigned>> &fbranches =
      functionConditionalBranches[kf];
  for (auto &kb : kf->blocks) {
    if (kb->basicBlock->getTerminator()->getNumSuccessors() > 1) {
      fbranches[kb.get()];
      for (unsigned branch = 0;
           branch < kb->basicBlock->getTerminator()->getNumSuccessors();
           ++branch) {
        fbranches[kb.get()].insert(branch);
      }
    }
  }
}
void CodeGraphInfo::calculateFunctionBlocks(KFunction *kf) {
  std::map<KBlock *, std::set<unsigned>> &fbranches = functionBlocks[kf];
  for (auto &kb : kf->blocks) {
    fbranches[kb.get()];
  }
}

const BlockDistanceMap &CodeGraphInfo::getDistance(KBlock *b) {
  if (blockDistance.count(b) == 0)
    calculateDistance(b);
  return blockDistance.at(b);
}

bool CodeGraphInfo::hasCycle(KBlock *kb) {
  if (!blockDistance.count(kb))
    calculateDistance(kb);
  return blockCycles.count(kb);
}

const BlockDistanceMap &CodeGraphInfo::getBackwardDistance(KBlock *kb) {
  if (blockBackwardDistance.count(kb) == 0)
    calculateBackwardDistance(kb);
  return blockBackwardDistance.at(kb);
}

const FunctionDistanceMap &CodeGraphInfo::getDistance(KFunction *kf) {
  if (functionDistance.count(kf) == 0)
    calculateDistance(kf);
  return functionDistance.at(kf);
}

const FunctionDistanceMap &CodeGraphInfo::getBackwardDistance(KFunction *kf) {
  if (functionBackwardDistance.count(kf) == 0)
    calculateBackwardDistance(kf);
  return functionBackwardDistance.at(kf);
}

void CodeGraphInfo::getNearestPredicateSatisfying(
    KBlock *from, KBlockPredicate predicate, bool forward,
    std::set<KBlock *, KBlockCompare> &result) {
  std::unordered_set<KBlock *> visited;
  std::unordered_set<KBlock *> queued;

  auto blockMap = from->parent->blockMap;
  std::deque<KBlock *> nodes;
  bool firstIteration = true;
  nodes.push_back(from);

  while (!nodes.empty()) {
    KBlock *currBB = nodes.front();
    if (!firstIteration) {
      visited.insert(currBB);
    }

    if (predicate(currBB) && !firstIteration) {
      result.insert(currBB);
    } else {
      if (forward) {
        for (auto const &succ : successors(currBB->basicBlock)) {
          if (!visited.count(blockMap[succ]) && !queued.count(blockMap[succ])) {
            nodes.push_back(blockMap[succ]);
            queued.insert(blockMap[succ]);
          }
        }
      } else {
        for (auto const &pred : predecessors(currBB->basicBlock)) {
          if (!visited.count(blockMap[pred]) && !queued.count(blockMap[pred])) {
            nodes.push_back(blockMap[pred]);
            queued.insert(blockMap[pred]);
          }
        }
      }
    }
    nodes.pop_front();
    firstIteration = false;
  }
}

const std::map<KBlock *, std::set<unsigned>> &
CodeGraphInfo::getFunctionBranches(KFunction *kf) {
  if (functionBranches.count(kf) == 0)
    calculateFunctionBranches(kf);
  return functionBranches.at(kf);
}

const std::map<KBlock *, std::set<unsigned>> &
CodeGraphInfo::getFunctionConditionalBranches(KFunction *kf) {
  if (functionConditionalBranches.count(kf) == 0)
    calculateFunctionConditionalBranches(kf);
  return functionConditionalBranches.at(kf);
}

std::set<KBlock *, KBlockCompare> CodeGraphInfo::getNearestPredicateSatisfying(
    KBlock *from, KBlockPredicate predicate, bool forward) {
  std::set<KBlock *, KBlockCompare> result;
  getNearestPredicateSatisfying(from, predicate, forward, result);
  return result;
}

const std::map<KBlock *, std::set<unsigned>> &
CodeGraphInfo::getFunctionBlocks(KFunction *kf) {
  if (functionBlocks.count(kf) == 0)
    calculateFunctionBlocks(kf);
  return functionBlocks.at(kf);
}

std::vector<std::pair<KBlock *, KBlock *>>
CodeGraphInfo::dismantleFunction(KFunction *kf, KBlockPredicate predicate) {
  std::vector<std::pair<KBlock *, KBlock *>> dismantled;
  std::queue<KBlock *> queue;
  std::set<KBlock *> used;

  // triple check!
  if (kf->finalKBlocks.count(kf->entryKBlock)) {
    return {{kf->entryKBlock, kf->entryKBlock}};
  }

  queue.push(kf->entryKBlock);
  while (!queue.empty()) {
    auto kblock = queue.front();
    queue.pop();
    used.insert(kblock);
    std::set<KBlock *> visited;
    std::set<KBlock *, KBlockCompare> nearest;
    getNearestPredicateSatisfying(kblock, predicate, true, nearest);
    for (auto to : nearest) {
      dismantled.push_back({kblock, to});
      if (!used.count(to)) {
        queue.push(to);
      }
    }
  }
  return dismantled;
}
