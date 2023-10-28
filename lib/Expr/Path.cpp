#include "klee/Expr/Path.h"

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

using namespace klee;
using namespace llvm;

void Path::stepInstruction(KInstruction *prevPC, KInstruction *pc) {
  // We actually executed the previous pc
  assert(next == prevPC);

  if (path.empty()) {
    path.push_back({prevPC->parent, getTransitionKindFromInst(prevPC)});
    first = prevPC->getIndex();
    last = prevPC->getIndex();
    next = pc;
  } else {
    auto lastEntry = path.back();
    if (prevPC->parent != lastEntry.block) {
      path.push_back({prevPC->parent, getTransitionKindFromInst(prevPC)});
    }
    last = prevPC->getIndex();
    next = pc;
  }
}

void Path::retractInstruction() {
  assert(!path.empty());
  auto lastExecuted = getLastInstruction();

  if (path.size() == 1 && first == last) {
    // Only one instruction executed
    path.pop_back();
    first = 0;
    last = 0;
    next = lastExecuted;
  } else {
    // Get before last instruction (and retract bb if needed)
    auto transitionKind = getTransitionKindFromInst(lastExecuted);
    if (transitionKind == TransitionKind::In ||
        transitionKind == TransitionKind::Out ||
        lastExecuted->parent->getFirstInstruction() == lastExecuted) {
      path.pop_back();
      last = getLastInstructionFromPathEntry(path.back())->getIndex();
    } else {
      assert(last > 0);
      last--;
      assert(last < lastExecuted->parent->getNumInstructions());
      assert(last == lastExecuted->parent->instructions[last]->getIndex());
    }
    next = lastExecuted;
  }
}

const Path::path_ty &Path::getBlocks() const { return path; }

unsigned Path::getFirstIndex() const { return first; }

KInstruction *Path::getFirstInstruction() const {
  assert(path.size() > 0);
  auto block = path.front().block;
  return block->instructions[first];
}

unsigned Path::getLastIndex() const { return last; }

KInstruction *Path::getLastInstruction() const {
  assert(path.size() > 0);
  auto block = path.back().block;
  return block->instructions[last];
}

bool Path::blockCompleted(unsigned index) const {
  assert(index >= 0 && index < path.size());
  if (index + 1 < path.size()) {
    return true;
  }
  assert(index + 1 == path.size());
  auto block = path.at(index);
  if (block.kind == TransitionKind::In) {
    return last == path.at(index).block->getFirstInstruction()->getIndex();
  } else {
    return last == path.at(index).block->getLastInstruction()->getIndex();
  }
}

KFunction *Path::getCalledFunction(unsigned index) const {
  assert(index >= 0 && index < path.size());
  assert(isa<KCallBlock>(path.at(index).block));
  if (index + 1 < path.size()) {
    return path.at(index + 1).block->parent;
  } else {
    assert(next);
    return next->parent->parent;
  }
}

KInstruction *Path::getCallsiteFromReturn(unsigned index) const {
  assert(index >= 0 && index < path.size());
  assert(index + 1 < path.size());
  return dyn_cast<KCallBlock>(path.at(index + 1).block)->kcallInstruction;
}

Path::PathIndex Path::getCurrentIndex() const {
  return {path.size() - 1, last};
}

std::vector<CallStackFrame> Path::getStack(bool reversed) const {
  std::vector<CallStackFrame> stack;
  for (unsigned i = 0; i < path.size(); i++) {
    auto index = reversed ? path.size() - 1 - i : i;
    auto current = path[index];

    if (i == 0) {
      stack.push_back(CallStackFrame(nullptr, current.block->parent));
      continue;
    }

    // TRIPLE CHECK i != 0 constraint
    if (reversed && i != 0) {
      if (current.kind == TransitionKind::In) {
        if (!stack.empty()) {
          stack.pop_back();
        }
      } else if (isa<KReturnBlock>(current.block) && blockCompleted(index)) {
        stack.push_back(CallStackFrame(getCallsiteFromReturn(index),
                                       current.block->parent));
      }
    } else {
      if (current.kind == TransitionKind::In) {
        stack.push_back(CallStackFrame(
            dyn_cast<KCallBlock>(current.block)->kcallInstruction,
            getCalledFunction(index)));
      } else if (isa<KReturnBlock>(current.block) && blockCompleted(index)) {
        if (!stack.empty()) {
          stack.pop_back();
        }
      }
    }
  }
  return stack;
}

Path Path::concat(const Path &l, const Path &r) {
  if (l.empty()) {
    return r;
  }
  if (r.empty()) {
    return l;
  }

  if (l.emptyWithNext()) {
    if (r.emptyWithNext()) {
      assert(l.next == r.next);
      return l;
    } else {
      assert(l.next == r.getFirstInstruction());
      return r;
    }
  }

  if (r.emptyWithNext()) {
    assert(l.next == r.next);
    return l;
  }

  // States that have nowhere to go after return
  if (!l.path.empty() && !l.next) {
    assert(l.blockCompleted(l.path.size() - 1));
    assert(isa<KReturnBlock>(l.path.back().block));
    assert(r.path.front().kind == TransitionKind::Out);
    auto kf = l.path.back().block->parent;
    assert(dyn_cast<KCallBlock>(r.path.front().block)
               ->calledFunctions.count(kf->function));
  } else {
    assert(l.next == r.getFirstInstruction());
  }
  Path path;
  auto leftWhole = l.blockCompleted(l.path.size() - 1);
  path.path.reserve(leftWhole ? l.path.size() + r.path.size()
                              : l.path.size() + r.path.size() - 1);
  for (unsigned i = 0; i < l.path.size(); i++) {
    path.path.push_back(l.path.at(i));
  }
  for (unsigned i = 0; i < r.path.size(); i++) {
    if (i == 0 && !leftWhole) {
      continue;
    }
    path.path.push_back(r.path.at(i));
  }
  path.first = l.first;
  path.last = r.last;
  path.next = r.next;
  return path;
}

void Path::print(llvm::raw_ostream &ss) const {
  std::vector<KFunction *> stack;
  std::vector<KFunction *> understack;
  for (unsigned i = 0; i < path.size(); i++) {
    auto current = path.at(i);
    if (current.kind == TransitionKind::In) {
      stack.push_back(getCalledFunction(i));
    } else if (isa<KReturnBlock>(current.block) && blockCompleted(i)) {
      if (!stack.empty()) {
        stack.pop_back();
      } else {
        // Check this, might get nullptr stackframe?
        if (i != path.size() - 1) {
          understack.push_back(getCallsiteFromReturn(i)->parent->parent);
        }
      }
    }
  }

  // assert(understack.empty() || stack.empty());

  ss << "path: (";

  if (path.empty()) {
    ss << "Empty";
  } else {
    ss << first << " ";
    int stackBalance = 0;
    for (auto it = understack.rbegin(); it != understack.rend(); it++) {
      ss << "(" << (*it)->getName().str() << ": ";
      stackBalance++;
    }

    for (unsigned i = 0; i < path.size(); i++) {
      auto current = path.at(i);
      if (i == 0 || path.at(i - 1).kind == TransitionKind::In) {
        ss << "(" << current.block->parent->getName().str() << ": ";
        stackBalance++;
      }
      if (current.kind == TransitionKind::Out) {
        ss << "-> ";
      }
      ss << current.block->getLabel();
      if (current.kind == TransitionKind::In) {
        ss << " ->";
      }
      if (isa<KReturnBlock>(current.block) || i == path.size() - 1) {
        ss << ")";
        stackBalance--;
        if (i != path.size() - 1) {
          ss << " ";
        }
      } else {
        ss << " ";
      }
    }
    assert(stackBalance >= 0);
    while (stackBalance > 0) {
      ss << ")";
      stackBalance--;
    }

    ss << " " << last;
  }
  ss << ") @ " << (next ? next->toString() : "None");
}

std::pair<bool, KCallBlock *> Path::fromOutTransition() const {
  if (path.empty()) {
    return {false, nullptr};
  }
  if (path.front().kind == TransitionKind::Out) {
    return {true, dyn_cast<KCallBlock>(path.front().block)};
  } else {
    return {false, nullptr};
  }
}

void Path::dump() const { print(llvm::errs()); }

std::string Path::toString() const {
  std::string result;
  llvm::raw_string_ostream ss(result);
  print(ss);
  return ss.str();
}

Path::TransitionKind klee::getTransitionKindFromInst(KInstruction *ki) {
  if (RegularFunctionPredicate(ki->parent)) {
    return ki->getIndex() == 0 ? Path::TransitionKind::In
                               : Path::TransitionKind::Out;
  }
  return Path::TransitionKind::None;
}

KInstruction *klee::getLastInstructionFromPathEntry(Path::entry entry) {
  switch (entry.kind) {
  case Path::TransitionKind::In:
    return entry.block->getFirstInstruction();
  case Path::TransitionKind::Out:
  case Path::TransitionKind::None:
    return entry.block->getLastInstruction();
  }
}
