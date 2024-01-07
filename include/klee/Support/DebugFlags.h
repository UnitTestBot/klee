#ifndef KLEE_DEBUGFLAGS_H
#define KLEE_DEBUGFLAGS_H

#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace klee {

enum class DebugPrint {
  Forward,
  Backward,
  Init,
  Reached,
  Lemma,
  RootPob,
  ClosePob,
  Conflict,
  PathForest
};

extern cl::bits<DebugPrint> debugPrints;
extern cl::bits<DebugPrint> debugConstraints;

}; // namespace klee

#endif /* KLEE_DEBUGFLAGS_H */
