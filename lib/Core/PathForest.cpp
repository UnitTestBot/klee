#include "PathForest.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace klee {

cl::OptionCategory PathForestCat("","");

cl::opt<bool> UsePathForest("path-forest", cl::init(true),
                            cl::desc("Enable path forest (default=true)"),
                            cl::cat(PathForestCat));

PathNode *PathNode::get(PathNode *node, Path::entry next) {
  if (!node->next.count(next)) {
    node->next[next] = new PathNode(next, node);
  }
  return node->next.at(next).get();
}

void PathNode::remove(PathNode *node) { node->next.clear(); }

bool PathNode::tryMerge(PathNode *node, bool forward) {
  if (node->next.empty()) {
    return true;
  }
  auto children =
      forward ? node->block.getSuccessors() : node->block.getPredecessors();

  for (auto child : children) {
    if (!node->next.count(child) ||
        !tryMerge(node->next.at(child).get(), forward)) {
      return false;
    }
  }

  remove(node);
  return true;
}

void PathForest::addPath(const Path::path_kind_ty &path) {
  if (!UsePathForest) {
    return;
  }

  assert(!path.empty());

  { // Prepare forward
    if (!forward.count(path.front())) {
      forward[path.front()] = new PathNode(path.front(), nullptr);
    }
    auto current = forward.at(path.front());
    for (unsigned i = 1; i < path.size(); i++) {
      current = PathNode::get(current, path[i]);
    }
    auto parent = current->parent;
    while (parent && PathNode::tryMerge(parent, true)) {
      parent = parent->parent;
    }
  }

  { // Prepare backward
    if (!backward.count(path.back())) {
      backward[path.back()] = new PathNode(path.back(), nullptr);
    }
    auto current = backward.at(path.back());
    for (int i = path.size() - 2; i >= 0; i--) {
      current = PathNode::get(current, path[i]);
    }
    auto parent = current->parent;
    while (parent && PathNode::tryMerge(parent, false)) {
      parent = parent->parent;
    }
  }
}

void PathTree::addTree(const PathNode *node) {
  assert(!root || node->block == root->block);
  if (!root) {
    root = new PathNode(node->block, nullptr);
  }
  bool childrenMerged = true;
  for (auto next : node->next) {
    childrenMerged = childrenMerged &&
                     attachAndMerge(root.get(), next.second.get());
  }
  if (childrenMerged) {
    PathNode::tryMerge(root.get(), forward);
  }
}

void PathTree::transfer(Path::entry to, const PathForest &forest) {
  if (root) {
    if (root->next.count(to)) {
      // Assert we are not stepping into a leaf
      assert(root->next.at(to)->next.size() > 0);
      root = root->next.at(to);
    } else {
      root = nullptr;
    }
  }

  if (forward) {
    if (forest.forward.count(to)) {
      addTree(forest.forward.at(to));
    }
  } else {
    if (forest.backward.count(to)) {
      addTree(forest.backward.at(to));
    }
  }
}

bool PathTree::TransitionForbidden(Path::entry block) {
  if (root) {
    if (root->next.count(block) && root->next.at(block)->next.size() == 0) {
      return true;
    }
  }
  return false;
}

bool PathTree::TransitionForbidden(const Path &path) {
  if (!root) {
    return false;
  }
  auto blocks = path.getKindedBlocks();
  auto current = root;
  if (!forward) {
    for (auto it = blocks.rbegin(); it != blocks.rend(); it++) {
      if (current->next.count(*it)) {
        if (current->next.at(*it)->next.empty()) {
          return false;
        } else {
          current = current->next.at(*it);
        }
      } else {
        break;
      }
    }
    return true;
  } else {
    for (auto it = blocks.begin(); it != blocks.end(); it++) {
      if (current->next.count(*it)) {
        if (current->next.at(*it)->next.empty()) {
          return false;
        } else {
          current = current->next.at(*it);
        }
      } else {
        break;
      }
    }
    return true;
  }
}

bool PathTree::attachAndMerge(PathNode *parent, PathNode *child) {
  auto childNode = PathNode::get(parent, child->block);
  bool childrenMerged = true;
  for (auto next : child->next) {
    childrenMerged =
        childrenMerged && attachAndMerge(childNode, next.second.get());
  }
  return childrenMerged ? PathNode::tryMerge(childNode, forward) : false;
}

} // namespace klee
