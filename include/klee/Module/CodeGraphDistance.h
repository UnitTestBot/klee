//===-- CodeGraphDistance.h -------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CODEGRAPHGDISTANCE_H
#define KLEE_CODEGRAPHGDISTANCE_H

#include "klee/Module/KModule.h"

#include <unordered_map>

namespace klee {

class CodeGraphDistance {

  using BlockDistance = std::unordered_map<KBlock *, unsigned>;
  using SortedBlockDistance = std::vector<std::pair<KBlock *, unsigned>>;

  using FunctionDistance = std::unordered_map<KFunction *, unsigned>;
  using SortedFunctionDistance = std::vector<std::pair<KFunction *, unsigned>>;

private:
  // For basic blocks
  std::unordered_map<KBlock *, BlockDistance> blockDistance;
  std::unordered_map<KBlock *, BlockDistance> blockBackwardDistance;
  std::unordered_map<KBlock *, SortedBlockDistance> blockSortedDistance;
  std::unordered_map<KBlock *, SortedBlockDistance> blockSortedBackwardDistance;

  // For functions
  std::unordered_map<KFunction *, FunctionDistance> functionDistance;
  std::unordered_map<KFunction *, FunctionDistance> functionBackwardDistance;
  std::unordered_map<KFunction *, SortedFunctionDistance> functionSortedDistance;
  std::unordered_map<KFunction *, SortedFunctionDistance> functionSortedBackwardDistance;

private:
  void calculateDistance(KBlock *bb);
  void calculateBackwardDistance(KBlock *bb);

  void calculateDistance(KFunction *kf);
  void calculateBackwardDistance(KFunction *kf);

  void getNearestPredicateSatisfying(KBlock *from, KBlockPredicate predicate,
                                     bool forward,
                                     std::set<KBlock *, KBlockLess> &result);

public:

  const BlockDistance &getDistance(KBlock *kb);
  const BlockDistance &getBackwardDistance(KBlock *kb);
  const SortedBlockDistance &getSortedDistance(KBlock *kb);
  const SortedBlockDistance &getSortedBackwardDistance(KBlock *kb);

  const FunctionDistance &getDistance(KFunction *kf);
  const FunctionDistance &getBackwardDistance(KFunction *kf);
  const SortedFunctionDistance &getSortedDistance(KFunction *kf);
  const SortedFunctionDistance &getSortedBackwardDistance(KFunction *kf);

  std::set<KBlock *, KBlockLess>
  getNearestPredicateSatisfying(KBlock *from, KBlockPredicate predicate,
                                bool forward);

  std::vector<std::pair<KBlock *, KBlock *>>
  dismantleFunction(KFunction *kf, KBlockPredicate predicate);
};


} // namespace klee

#endif /* KLEE_CODEGRAPHGDISTANCE_H */
