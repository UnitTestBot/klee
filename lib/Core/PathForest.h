#ifndef KLEE_PATHFOREST_H
#define KLEE_PATHFOREST_H

#include "Target.h"
#include "klee/ADT/Ref.h"
#include "klee/Expr/Path.h"
#include "klee/Module/CodeGraphDistance.h"
#include "klee/Module/KModule.h"

#include <map>
#include <unordered_set>

namespace klee {

struct PathNode {
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  Path::entry block;
  PathNode *parent;
  std::map<Path::entry, ref<PathNode>> next;

  PathNode(Path::entry block, PathNode *parent)
      : block(block), parent(parent) {}

  ~PathNode() {
    for (auto child : next) {
      child.second->parent = nullptr;
    }
  }

  // Creates the corresponding node if it does not exist
  static PathNode *get(PathNode *, Path::entry);

  static bool tryMerge(PathNode *, bool forward);

  static bool finalTransition(PathNode *node, Path::entry block);
  static bool isBlocked(PathNode *node, bool forward);

  void printInternal(llvm::raw_ostream &os, unsigned indent) const;
  std::string toString() const;
};


class PathForest {
public:
  void addPath(const Path::path_kind_ty &path);

  void print(llvm::raw_ostream &os) const;
  std::string toString() const;
public:
  std::map<Path::entry, PathNode *> forward;
  std::map<Path::entry, PathNode *> backward;

  CodeGraphDistance *cgd;

  std::set<Path::entry> forwardBlocked;
  std::set<Path::entry> backwardBlocked;

private:
  void propagateBlocked(PathNode *, bool forward);
};

class PathTree {
public:
  CodeGraphDistance *cgd;
  std::set<Target> targets;

public:
  PathTree(bool forward) : forward(forward) {}

  void addTree(const PathNode *node);
  bool TransitionForbidden(Path::entry block);
  bool TransitionForbidden(const Path &path);
  void transfer(Path::entry to, const PathForest &forest, std::set<Target> targets = {});

  void print(llvm::raw_ostream &os) const;
  std::string toString() const;

private:
  bool attachAndMerge(PathNode *parent, PathNode *child);

private:
  ref<PathNode> root;
  const bool forward;
};

}
#endif
