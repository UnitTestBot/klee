//===-- KInstruction.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringExtras.h"
DISABLE_WARNING_POP

#include <string>

using namespace llvm;
using namespace klee;

/***/

KInstruction::~KInstruction() { delete[] operands; }

std::string KInstruction::getSourceLocation() const {
  if (!info->file.empty())
    return info->file + ":" + std::to_string(info->line) + " " +
           std::to_string(info->column);
  else
    return "[no debug info]";
}

std::string KInstruction::toString() const {
  std::string ret;
  llvm::raw_string_ostream ss(ret);
  ss << "[" << index << ", " << parent->getLabel() << ", "
     << parent->parent->getName() << "]";
  return ss.str();
}

CallStackFrame::CallStackFrame(const CallStackFrame &s)
    : caller(s.caller), kf(s.kf) {}

bool CallStackFrame::equals(const CallStackFrame &other) const {
  return kf == other.kf && caller == other.caller;
}
