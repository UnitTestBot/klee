//=== --SeedInfo.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SEEDINFO_H
#define KLEE_SEEDINFO_H

#include "ExecutionState.h"
#include "klee/ADT/ImmutableList.h"
#include "klee/ADT/PersistentSet.h"
#include "klee/Expr/Assignment.h"
#include "klee/Module/Target.h"

#include <deque>
#include <set>
#include <vector>

extern "C" {
struct KTest;
struct KTestObject;
}

namespace klee {
class ExecutionState;
class TimingSolver;
class MemoryObject;

class ExecutingSeed {
public:
  Assignment assignment;
  std::shared_ptr<KTest> input;
  unsigned maxInstructions;
  std::set<struct KTestObject *> used;
  mutable std::deque<ref<box<bool>>> coveredNew;
  mutable ref<box<bool>> coveredNewError = nullptr;
  unsigned inputPosition = 0;
  unsigned parentId;
  PersistentSet<ref<Target>> targets;
  bool isTargeted;
  std::vector<unsigned char> path;
  std::vector<unsigned char> symPath;

public:
  ~ExecutingSeed() {}

  ExecutingSeed() {}

  explicit ExecutingSeed(KTest *input, unsigned maxInstructions = 0,
                         std::deque<ref<box<bool>>> coveredNew = {},
                         ref<box<bool>> coveredNewError = 0,
                         const PersistentSet<ref<Target>> targets = {},
                         bool isTargeted = 0)
      : input(input, kTestDeleter), maxInstructions(maxInstructions),
        coveredNew(coveredNew), coveredNewError(coveredNewError),
        targets(targets), isTargeted(isTargeted) {}

  explicit ExecutingSeed(Assignment assignment, unsigned maxInstructions = 0,
                         std::deque<ref<box<bool>>> coveredNew = {},
                         ref<box<bool>> coveredNewError = 0,
                         const PersistentSet<ref<Target>> targets = {},
                         bool isTargeted = 0)
      : assignment(assignment), maxInstructions(maxInstructions),
        coveredNew(coveredNew), coveredNewError(coveredNewError),
        targets(targets), isTargeted(isTargeted) {}

  KTestObject *getNextInput(const MemoryObject *mo, bool byName);

  static void kTestDeleter(KTest *ktest);
};
} // namespace klee

#endif /* KLEE_SEEDINFO_H */
