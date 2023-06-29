#ifndef KLEE_PATHFOREST_H
#define KLEE_PATHFOREST_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Path.h"
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

  static PathNode *get(PathNode *, Path::entry);
  static void remove(PathNode *);
  static bool tryMerge(PathNode *, bool forward);
};


class PathForest {
public:
  void addPath(const Path::path_kind_ty &path);

public:
  std::map<Path::entry, PathNode *> forward;
  std::map<Path::entry, PathNode *> backward;
};

class PathTree {
public:
  PathTree(bool forward) : forward(forward) {}

  void addTree(const PathNode *node);
  bool TransitionForbidden(Path::entry block);
  bool TransitionForbidden(const Path &path);
  void transfer(Path::entry to, const PathForest &forest);

private:
  bool attachAndMerge(PathNode *parent, PathNode *child);

private:
  ref<PathNode> root;
  const bool forward;
};

}
#endif
