//===--SeedInfo.h ----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SEEDINFO_H
#define KLEE_SEEDINFO_H

#include "klee/Expr/Assignment.h"

#include <deque>
#include <set>

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
  unsigned maxInstructions = 0;
  bool isCompleted = 0;
  std::set<struct KTestObject *> used;
  std::string path = "";
  mutable std::deque<ref<box<bool>>> coveredNew;
  mutable ref<box<bool>> coveredNewError = nullptr;
  unsigned inputPosition = 0;

public:
  ~ExecutingSeed() {}

  ExecutingSeed() {}

  explicit ExecutingSeed(KTest *input, unsigned maxInstructions,
                         bool isCompleted,
                         std::deque<ref<box<bool>>> coveredNew,
                         ref<box<bool>> coveredNewError)
      : input(input, kTestDeleter), maxInstructions(maxInstructions),
        isCompleted(isCompleted), coveredNew(coveredNew),
        coveredNewError(coveredNewError) {}

  explicit ExecutingSeed(Assignment assignment, unsigned maxInstructions,
                         bool isCompleted,
                         std::deque<ref<box<bool>>> coveredNew,
                         ref<box<bool>> coveredNewError)
      : assignment(assignment), maxInstructions(maxInstructions),
        isCompleted(isCompleted), coveredNew(coveredNew),
        coveredNewError(coveredNewError) {}

  ExecutingSeed(std::string _path);

  KTestObject *getNextInput(const MemoryObject *mo, bool byName);

  static void kTestDeleter(KTest *kTest);

  /// Patch the seed so that condition is satisfied while retaining as
  /// many of the seed values as possible.
  void patchSeed(const ExecutionState &state, ref<Expr> condition,
                 TimingSolver *solver);
};
} // namespace klee

#endif /* KLEE_SEEDINFO_H */
