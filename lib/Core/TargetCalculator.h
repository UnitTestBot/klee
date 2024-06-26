//===-- TargetCalculator.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETCALCULATOR_H
#define KLEE_TARGETCALCULATOR_H

#include "ObjectManager.h"

#include "klee/ADT/RNG.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Target.h"
#include "klee/Module/TargetHash.h"
#include "klee/Support/OptionCategories.h"
#include "klee/System/Time.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <map>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace klee {
class CodeGraphInfo;
class ExecutionState;

enum class TrackCoverageBy { None, Blocks, Branches, All };

class TargetCalculator : public Subscriber {
  using StatesSet = states_ty;

  enum HistoryKind { Blocks, Transitions };

  typedef std::unordered_map<KFunction *, KBlockMap<std::set<unsigned>>>
      CoveredBranches;

  typedef std::unordered_set<KFunction *> CoveredFunctionsBranches;

  void update(const ExecutionState &state);

  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates);

public:
  TargetCalculator(CodeGraphInfo &codeGraphInfo)
      : codeGraphInfo(codeGraphInfo) {}

  void update(ref<ObjectManager::Event> e) override;

  TargetHashSet calculate(ExecutionState &state);

  bool isCovered(KFunction *kf) const;
  bool uncoveredBlockPredicate(KBlock *kblock);

private:
  CodeGraphInfo &codeGraphInfo;
  CoveredBranches coveredBranches;
  CoveredFunctionsBranches coveredFunctionsInBranches;
  CoveredFunctionsBranches fullyCoveredFunctions;
  StatesSet localStates;

  const KBlockMap<std::set<unsigned>> &getCoverageTargets(KFunction *kf);
};
} // namespace klee

#endif /* KLEE_TARGETCALCULATOR_H */
