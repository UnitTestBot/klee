#ifndef KLEE_PATH_H
#define KLEE_PATH_H

#include "klee/ADT/ImmutableList.h"

#include <string>
#include <vector>

namespace klee {
struct KBlock;
struct KFunction;
struct KInstruction;
class KModule;

// Callsite, called function
using stackframe_ty = std::pair<KInstruction *, KFunction *>;

class Path {
public:
  using path_ty = ImmutableList<KBlock *>;
  enum class TransitionKind { StepInto, StepOut, None };

  struct PathIndex {
    unsigned long block;
    unsigned long instruction;
    bool operator==(const PathIndex &rhs) const {
      return block == rhs.block && instruction == rhs.instruction;
    }
    PathIndex() = default;
  };

  struct PathIndexCompare {
    bool operator()(const PathIndex &a, const PathIndex &b) const {
      return a.block < b.block ||
             (a.block == b.block && a.instruction < b.instruction);
    }
  };

  struct BlockRange {
    unsigned long first;
    unsigned long last;
  };

  void advance(KInstruction *ki);

  unsigned KBlockSize() const;
  const path_ty &getBlocks() const;
  unsigned getFirstIndex() const;
  unsigned getLastIndex() const;

  PathIndex getCurrentIndex() const;

  std::vector<stackframe_ty> getStack(bool reversed) const;

  std::string toString() const;

  static Path concat(const Path &l, const Path &r);

  static Path parse(const std::string &str, const KModule &km);

  Path() = default;

  Path(unsigned firstInstruction, const path_ty &kblocks,
       unsigned lastInstruction)
      : KBlocks(kblocks), firstInstruction(firstInstruction),
        lastInstruction(lastInstruction) {}

private:
  path_ty KBlocks;
  // Index of the first instruction in the first basic block
  unsigned firstInstruction = 0;
  // Index of the last (current) instruction in the current basic block
  unsigned lastInstruction = 0;

  static TransitionKind getTransitionKind(KBlock *a, KBlock *b);
};

}; // namespace klee

template <> struct std::hash<klee::Path::PathIndex> {
  std::size_t operator()(const klee::Path::PathIndex &s) const noexcept {
    std::size_t r = 0;
    std::size_t h1 = std::hash<unsigned long>{}(s.block);
    std::size_t h2 = std::hash<unsigned long>{}(s.instruction);
    r ^= h1 + 0x9e3779b9 + (r << 6) + (r >> 2);
    r ^= h2 + 0x9e3779b9 + (r << 6) + (r >> 2);
    return r;
  }
};

#endif
