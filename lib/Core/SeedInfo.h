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

#include <set>

extern "C" {
struct KTest;
struct KTestObject;
}

namespace klee {
class ExecutionState;
class TimingSolver;
class MemoryObject;

struct KTestDeleter {
  static void kTestDeleter(KTest *kTest);
  static void testKTestDeleter(KTest *kTest);
};

class StoredSeed {
public:
  std::shared_ptr<KTest> output;
  unsigned steppedInstructions;
  bool isCompleted;

public:
  ~StoredSeed() {}

  explicit StoredSeed(KTest *output, unsigned steppedInstructions,
                      bool isCompleted)
      : output(output, KTestDeleter::testKTestDeleter),
        steppedInstructions(steppedInstructions), isCompleted(isCompleted) {}
};

class ExecutingSeed {
public:
  Assignment assignment;
  std::shared_ptr<KTest> input;
  unsigned maxInstructions = 0;
  bool isCompleted = 0;
  unsigned inputPosition = 0;
  std::set<struct KTestObject *> used;
  std::string path = "";

public:
  ~ExecutingSeed() {}

  explicit ExecutingSeed(KTest *input, unsigned maxInstructions,
                         bool isCompleted)
      : input(input, KTestDeleter::testKTestDeleter),
        maxInstructions(maxInstructions), isCompleted(isCompleted),
        inputPosition(0) {}
  ExecutingSeed(StoredSeed seed);
  ExecutingSeed(std::string _path);

  KTestObject *getNextInput(const MemoryObject *mo, bool byName);

  /// Patch the seed so that condition is satisfied while retaining as
  /// many of the seed values as possible.
  void patchSeed(const ExecutionState &state, ref<Expr> condition,
                 TimingSolver *solver);
};
} // namespace klee

#endif /* KLEE_SEEDINFO_H */
