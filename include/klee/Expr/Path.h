#ifndef KLEE_PATH_H
#define KLEE_PATH_H

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <stack>
#include <string>
#include <vector>

namespace klee {
struct CallStackFrame;
struct KBlock;
struct KCallBlock;
struct KFunction;
struct KInstruction;
class KModule;

class Path {
public:
  enum class TransitionKind { In, Out, None };

  struct entry {
    KBlock *block;
    TransitionKind kind;

    bool operator==(const entry &other) const {
      return block == other.block && kind == other.kind;
    }

    bool operator<(const entry &other) const {
      return block < other.block || (block == other.block && kind < other.kind);
    }

    std::vector<entry> getPredecessors();
    std::vector<entry> getSuccessors();
  };

  using path_ty = std::vector<entry>;

  struct PathIndex {
    unsigned long block;
    unsigned long instruction;
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

public:
  void stepInstruction(KInstruction *prevPC, KInstruction *pc);
  void retractInstruction();

  friend bool operator==(const Path &lhs, const Path &rhs) {
    return lhs.path == rhs.path && lhs.first == rhs.first &&
           lhs.last == rhs.last && lhs.next == rhs.next;
  }
  friend bool operator!=(const Path &lhs, const Path &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<(const Path &lhs, const Path &rhs) {
    return lhs.path < rhs.path ||
           (lhs.path == rhs.path &&
            (lhs.first < rhs.first ||
             (lhs.first == rhs.first &&
              (lhs.last < rhs.last ||
               (lhs.last == rhs.last && lhs.next < rhs.next)))));
  }

  bool empty() const { return path.empty(); }
  bool emptyWithNext() const { return path.empty() && next; }

  std::pair<bool, KCallBlock *> fromOutTransition() const;

  const path_ty &getBlocks() const;
  unsigned getFirstIndex() const;
  KInstruction *getFirstInstruction() const;
  unsigned getLastIndex() const;
  KInstruction *getLastInstruction() const;
  KInstruction *getNext() const { return next; }

  bool blockCompleted(unsigned index) const;
  KFunction *getCalledFunction(unsigned index) const;
  KInstruction *getCallsiteFromReturn(unsigned index) const;

  PathIndex getCurrentIndex() const;

  std::vector<CallStackFrame> getStack(bool reversed) const;

  void print(llvm::raw_ostream &ss) const;
  void dump() const;
  std::string toString() const;

  static Path concat(const Path &l, const Path &r);

  // For proof obligations
  Path() = default;

  // For execution states
  explicit Path(KInstruction *next) : next(next) {}

  Path(unsigned first, std::vector<entry> path, unsigned last,
       KInstruction *next)
      : first(first), last(last), path(path), next(next) {}

private:
  // The path is stored as:
  // [KBlock, ... , KBlock], PC <- Next inst to execute
  // ^first executed inst ^last executed inst

  // Index of the first executed instruction
  // in the first basic block
  unsigned first = 0;
  // Index of the last (current) instruction
  // in the lastly executed basic block
  unsigned last = 0;

  // Basic blocks in the middle are fully executed
  path_ty path;

  // Next instruction to execute, if makes sense
  KInstruction *next = nullptr;
};

Path::TransitionKind getTransitionKindFromInst(KInstruction *ki);
KInstruction *getLastInstructionFromPathEntry(Path::entry entry);

}; // namespace klee

#endif
