#include "PathForest.h"
#include "klee/Support/DebugFlags.h"
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

bool PathNode::tryMerge(PathNode *node, bool forward) {
  if (node->next.empty()) {
    return true;
  }
  auto children =
      forward ? node->block.getSuccessors() : node->block.getPredecessors();

  for (auto child : children) {
    if (!PathNode::finalTransition(node, child)) {
      return false;
    }
  }

  node->next.clear();
  return true;
}

bool PathNode::finalTransition(PathNode *node, Path::entry block) {
  return node->next.count(block) && node->next.at(block)->next.empty();
}

bool PathNode::isBlocked(PathNode *node, bool forward) {
  assert(!node->parent);

  auto next =
      forward ? node->block.getSuccessors() : node->block.getPredecessors();

  if (next.size() == node->next.size()) {
    bool allFinal = true;
    for (auto i : node->next) {
      if (!PathNode::finalTransition(node, i.first)) {
        allFinal = false;
        break;
      }
    }
    return allFinal;
  } else {
    return false;
  }
}

void PathForest::addPath(const Path::path_kind_ty &path) {
  if (!UsePathForest) {
    return;
  }

  if (debugPrints.isSet(DebugPrint::PathForest)) {
    llvm::errs() << "[pathforest] Adding path: " << Path(0, path, 0).toString()
                 << "\n";
    llvm::errs() << "Forest before:\n";
    print(llvm::errs());
  }

  assert(path.size() >= 2);

  { // Prepare forward
    if (!forward.count(path.front())) {
      forward[path.front()] = new PathNode(path.front(), nullptr);
    }
    auto current = forward.at(path.front());
    for (unsigned i = 1; i < path.size(); i++) {
      if (PathNode::finalTransition(current, path[i])) {
        current = PathNode::get(current, path[i]);
        break;
      }
      current = PathNode::get(current, path[i]);
      if (i == path.size() - 1) {
        current->next.clear();
      }
    }
    while (current && current->parent && PathNode::tryMerge(current, true)) {
      current = current->parent;
    }

    auto root = forward[path.front()];
    if (PathNode::isBlocked(root, true)) {
      propagateBlocked(root, true);
    }
  }

  { // Prepare backward
    if (!backward.count(path.back())) {
      backward[path.back()] = new PathNode(path.back(), nullptr);
    }
    auto current = backward.at(path.back());
    for (int i = path.size() - 2; i >= 0; i--) {
      if (PathNode::finalTransition(current, path[i])) {
        current = PathNode::get(current, path[i]);
        break;
      }
      current = PathNode::get(current, path[i]);
      if (i == 0) {
        current->next.clear();
      }
    }

    while (current && current->parent && PathNode::tryMerge(current, false)) {
      current = current->parent;
    }

    auto root = backward[path.back()];
    if (PathNode::isBlocked(root, false)) {
      propagateBlocked(root, false);
    }
  }

  if (debugPrints.isSet(DebugPrint::PathForest)) {
    llvm::errs() << "Forest after:\n";
    print(llvm::errs());
  }
}

void PathForest::propagateBlocked(PathNode *node, bool forward) {
  assert(PathNode::isBlocked(node, forward));

  if (forward && debugPrints.isSet(DebugPrint::BlacklistBlockForward)) {
    llvm::errs() << "[block blocked] " << node->block.block->toString()
                 << " is forward blocked\n";
  }

  if (!forward && debugPrints.isSet(DebugPrint::BlacklistBlockBackward)) {
    llvm::errs() << "[block blocked] " << node->block.block->toString()
                 << " is backward blocked\n";
  }

  auto preds =
      forward ? node->block.getPredecessors() : node->block.getSuccessors();
  for (auto pred : preds) {
    PathNode *predNode = nullptr;
    if (forward) {
      if (!this->forward.count(pred)) {
        this->forward[pred] = new PathNode(pred, nullptr);
      }
      predNode = this->forward.at(pred);
    } else {
      if (!this->backward.count(pred)) {
        this->backward[pred] = new PathNode(pred, nullptr);
      }
      predNode = this->backward.at(pred);
    }
    PathNode::get(predNode, node->block);
    if (PathNode::isBlocked(predNode, forward)) {
      propagateBlocked(predNode, forward);
    }
  }

  if (preds.empty()) {
    if (forward && debugPrints.isSet(DebugPrint::BlacklistBlockForward)) {
      llvm::errs()
          << "[block blocked] ENTRY BLOCK (NO PREDECESSORS) IS BLOCKED!\n";
    }

    if (!forward && debugPrints.isSet(DebugPrint::BlacklistBlockBackward)) {
      llvm::errs()
          << "[block blocked] FINAL BLOCK (NO SUCCESSORS) IS BLOCKED!\n";
    }
  }

  // if (!preds.empty()) {
  //   if (forward) {
  //     this->forward.erase(node->block);
  //   } else {
  //     this->backward.erase(node->block);
  //   }
  // }
}

void PathForest::print(llvm::raw_ostream &os) const {
  if (forward.empty()) {
    os << "[Empty]\n";
    return;
  }
  for (auto i : forward) {
    i.second->printInternal(os, 0);
  }
}

std::string PathForest::toString() const {
  std::string ret;
  llvm::raw_string_ostream s(ret);
  print(s);
  return s.str();
}

void PathTree::addTree(const PathNode *node) {
  assert(!root || node->block == root->block);
  if (forward && debugPrints.isSet(DebugPrint::PathTree)) {
    llvm::errs() << "[pathforest] Adding tree to forward. Before:\n";
    print(llvm::errs());
    llvm::errs() << "State targets:\n" << (targets.empty() ? "[None]\n" : "");
    for (auto i : targets) {
      llvm::errs() << i.toString() << "\n";
    }
    llvm::errs() << "Adding tree:\n";
    node->printInternal(llvm::errs(), 0);
  }
  if (!root) {
    root = new PathNode(node->block, nullptr);
  }
  bool childrenMerged = true;
  for (auto next : node->next) {
    childrenMerged = childrenMerged &&
                     attachAndMerge(root.get(), next.second.get());
  }
  // if (childrenMerged) {
  //   PathNode::tryMerge(root.get(), forward);
  // }
  if (forward && debugPrints.isSet(DebugPrint::PathTree)) {
    llvm::errs() << "After adding:\n";
    print(llvm::errs());
  }
}

void PathTree::transfer(Path::entry to, const PathForest &forest, std::set<Target> targets) {
  cgd = forest.cgd;
  this->targets = targets;
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
  if (forward) {
    auto children = parent->block.getSuccessors();
    for (auto child : children) {
      auto distance = cgd->getDistance(child.block);
      bool reachable = false;
      if (targets.empty()) {
        reachable = true;
      }
      for (auto target : targets) {
        if (child.block == target.getBlock() ||
            distance.count(target.getBlock())) {
          reachable = true;
        }
      }
      if (!reachable) {
        // llvm::errs() << "UNREACHABLE T\n";
        PathNode::get(parent, child);
      }
    }
  }

  if (PathNode::finalTransition(parent, child->block)) {
    return true;
  }
  auto childNode = PathNode::get(parent, child->block);
  bool childrenMerged = true;
  for (auto next : child->next) {
    childrenMerged =
        childrenMerged && attachAndMerge(childNode, next.second.get());
  }
  return childrenMerged ? PathNode::tryMerge(childNode, forward) : false;
}

void PathTree::print(llvm::raw_ostream &os) const {
  if (!root) {
    os << "[Empty]\n";
    return;
  }
  root->printInternal(os, 0);
}

std::string PathTree::toString() const {
  std::string ret;
  llvm::raw_string_ostream s(ret);
  print(s);
  return s.str();
}

void PathNode::printInternal(llvm::raw_ostream &os, unsigned indent) const {
  os << std::string(indent, ' ') << block.block->toString() << " "
     << block.kindToString() << "\n";
  if (!next.empty()) {
    for (auto e : next) {
      e.second->printInternal(os, indent + 2);
    }
  }
}

std::string PathNode::toString() const {
  std::string ret;
  llvm::raw_string_ostream s(ret);
  printInternal(s, 0);
  return s.str();
}

} // namespace klee
