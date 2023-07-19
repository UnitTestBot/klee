#include "klee/Expr/Path.h"

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/Casting.h"

using namespace klee;
using namespace llvm;

void Path::advance(KInstruction *ki) {
  if (KBlocks.empty()) {
    firstInstruction = ki->index;
    lastInstruction = ki->index;
    KBlocks.push_back(ki->parent);
    return;
  }
  auto lastBlock = KBlocks.back();
  if (ki->parent != lastBlock) {
    KBlocks.push_back(ki->parent);
  }
  lastInstruction = ki->index;
  return;
}

unsigned Path::KBlockSize() const { return KBlocks.size(); }

const Path::path_ty &Path::getBlocks() const { return KBlocks; }

const Path::path_kind_ty Path::getKindedBlocks() const {
  path_kind_ty kinded;
  kinded.reserve(KBlocks.size());

  if (KBlocks.size() == 1 && RegularFunctionPredicate(KBlocks.front())) {
    kinded.push_back(
        {KBlocks.front(), firstInstruction == 0 ? Kind::In : Kind::Out});
    return kinded;
  }

  for (unsigned i = 0; i < KBlocks.size(); i++) {
    if (RegularFunctionPredicate(KBlocks[i])) {
      if (i != KBlocks.size() - 1) {
        if (KBlocks[i + 1]->parent->entryKBlock == KBlocks[i + 1]) {
          kinded.push_back({KBlocks[i], Kind::In});
        } else {
          kinded.push_back({KBlocks[i], Kind::Out});
        }
        continue;
      }
      if (i != 0) {
        if (isa<KReturnBlock>(KBlocks[i - 1])) {
          kinded.push_back({KBlocks[i], Kind::Out});
        } else {
          kinded.push_back({KBlocks[i], Kind::In});
        }
        continue;
      }
    } else {
      kinded.push_back({KBlocks[i], Kind::None});
    }
  }
  return kinded;
}

unsigned Path::getFirstIndex() const { return firstInstruction; }

unsigned Path::getLastIndex() const { return lastInstruction; }

Path::PathIndex Path::getCurrentIndex() const {
  return {KBlocks.size() - 1, lastInstruction};
}

std::vector<stackframe_ty> Path::getStack(bool reversed) const {
  std::vector<stackframe_ty> stack;
  for (unsigned i = 0; i < KBlocks.size(); i++) {
    auto current = reversed ? KBlocks[KBlocks.size() - 1 - i] : KBlocks[i];
    // Previous for reversed is the next
    KBlock *prev = nullptr;
    if (i != 0) {
      prev = reversed ? KBlocks[KBlocks.size() - i] : KBlocks[i - 1];
    }
    if (i == 0) {
      stack.push_back({nullptr, current->parent});
      continue;
    }
    if (reversed) {
      auto kind = getTransitionKind(current, prev);
      if (kind == Kind::In) {
        if (!stack.empty()) {
          stack.pop_back();
        }
      } else if (kind == Kind::Out) {
        assert(isa<KCallBlock>(prev));
        stack.push_back({prev->getFirstInstruction(), current->parent});
      }
    } else {
      auto kind = getTransitionKind(prev, current);
      if (kind == Kind::In) {
        stack.push_back({prev->getFirstInstruction(), current->parent});
      } else if (kind == Kind::Out) {
        if (!stack.empty()) {
          stack.pop_back();
        }
      }
    }
  }
  return stack;
}

std::vector<std::pair<KFunction *, Path::BlockRange>>
Path::asFunctionRanges() const {
  assert(!KBlocks.empty());
  std::vector<std::pair<KFunction *, BlockRange>> ranges;
  BlockRange range{0, 0};
  KFunction *function = KBlocks[0]->parent;
  for (unsigned i = 1; i < KBlocks.size(); i++) {
    if (getTransitionKind(KBlocks[i - 1], KBlocks[i]) == Kind::None) {
      if (i == KBlocks.size() - 1) {
        range.last = i;
        ranges.push_back({function, range});
        return ranges;
      } else {
        continue;
      }
    }
    range.last = i - 1;
    ranges.push_back({function, range});
    range.first = i;
    function = KBlocks[i]->parent;
  }
  llvm_unreachable("asFunctionRanges reached the end of the for!");
}

Path Path::concat(const Path &l, const Path &r) {
  Path path = l;
  for (auto block : r.KBlocks) {
    path.KBlocks.push_back(block);
  }
  path.lastInstruction = r.lastInstruction;
  return path;
}

std::string Path::toString() const {
  std::string blocks = "";
  unsigned depth = 0;
  for (unsigned i = 0; i < KBlocks.size(); i++) {
    auto current = KBlocks[i];
    KBlock *prev = nullptr;
    if (i != 0) {
      prev = KBlocks[i - 1];
    }
    auto kind =
        i == 0 ? Kind::In : getTransitionKind(prev, current);
    if (kind == Kind::In) {
      blocks += " (" + current->parent->getName().str() + ":";
      depth++;
    } else if (kind == Kind::Out) {
      blocks += ")";
      if (depth > 0) {
        depth--;
      }
      if (depth == 0) {
        blocks = "(" + current->parent->getName().str() + ":" + blocks;
        ++depth;
      }
    }
    blocks += " " + current->getLabel();
    if (i == KBlocks.size() - 1) {
      blocks += ")";
      if (depth > 0) {
        depth--;
      }
    }
  }
  blocks += std::string(depth, ')');
  return "(path: " + llvm::utostr(firstInstruction) + blocks + " " +
         utostr(lastInstruction) + ")";
}

Path Path::parse(const std::string &str, const KModule &km) {
  unsigned index = 0;
  assert(str.substr(index, 7) == "(path: ");
  index += 7;

  std::string firstInstructionStr;
  while (index < str.size() && str[index] != ' ') {
    firstInstructionStr += str[index];
    index++;
  }
  auto firstInstruction = std::stoul(firstInstructionStr);

  std::stack<KFunction *> stack;
  path_ty KBlocks;
  bool firstParsed = false;
  while (!stack.empty() || !firstParsed) {
    while (index < str.size() && str[index] == ' ') {
      index++;
    }
    assert(index < str.size());
    if (str[index] == '(') {
      index++;
      std::string functionName;
      while (str[index] != ':') {
        functionName += str[index];
        ++index;
      }
      assert(km.functionNameMap.count(functionName));
      stack.push(km.functionNameMap.at(functionName));
      firstParsed = true;
      ++index;
    } else if (str[index] == ')') {
      index++;
      stack.pop();
    } else if (str[index] == '%') {
      std::string label = "%";
      ++index;
      while (str[index] != ' ' && str[index] != ')') {
        label += str[index];
        ++index;
      }
      KBlocks.push_back(stack.top()->getLabelMap().at(label));
    }
  }
  assert(index < str.size());
  assert(str[index] == ' ');
  index++;

  std::string lastInstructionStr;
  while (index < str.size() && str[index] != ' ') {
    lastInstructionStr += str[index];
    index++;
  }
  auto lastInstruction = std::stoul(lastInstructionStr);
  assert(index < str.size() && str[index] == ')');
  return Path(firstInstruction, KBlocks, lastInstruction);
}

Path::Kind Path::getTransitionKind(KBlock *a, KBlock *b) {
  if (auto cb = dyn_cast<KCallBlock>(a)) {
    if (b->parent->function == cb->calledFunction &&
        b == b->parent->entryKBlock) {
      return Kind::In;
    }
  }
  if (auto rb = dyn_cast<KReturnBlock>(a)) {
    return Kind::Out;
  }
  assert(a->parent == b->parent);
  return Kind::None;
}

std::vector<Path::entry> Path::entry::getPredecessors() const {
  auto km = block->parent->parent;
  std::vector<Path::entry> ret;

  if (kind == Kind::Out) {
    auto kCallBlock = dyn_cast<KCallBlock>(block);
    auto kf = kCallBlock->getKFunction();
    for (auto retBlock : kf->returnKBlocks) {
      ret.push_back({retBlock, Kind::None});
    }
  } else if (block->parent->entryKBlock == block) {
    for (auto callBlock : km->callSiteMap.at(block->parent->function)) {
      ret.push_back({callBlock, Kind::In});
    }
  } else {
    auto bb = block->basicBlock;
    for (auto it = llvm::pred_iterator(bb), et = llvm::pred_end(bb); it != et;
         it++) {
      auto kb = km->getKBlock(*it);
      auto kind = RegularFunctionPredicate(kb) ? Kind::Out : Kind::None;
      ret.push_back({kb, kind});
    }
  }
  return ret;
}

std::vector<Path::entry> Path::entry::getSuccessors() const {
  auto km = block->parent->parent;
  std::vector<Path::entry> ret;

  if (kind == Kind::In) {
    auto kCallBlock = dyn_cast<KCallBlock>(block);
    auto kb = kCallBlock->getKFunction()->entryKBlock;
    auto kind = RegularFunctionPredicate(kb) ? Kind::In : Kind::None;
    ret.push_back({kb, kind});
  } else if (isa<KReturnBlock>(block)) {
    for (auto callBlock : km->callSiteMap.at(block->parent->function)) {
      ret.push_back({callBlock, Kind::Out});
    }
  } else {
    for (auto successor : block->successors()) {
      auto kind = RegularFunctionPredicate(successor) ? Kind::In : Kind::None;
      ret.push_back({successor, kind});
    }
  }
  return ret;
}
