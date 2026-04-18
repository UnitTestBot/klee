//===-- SmithrilHashConfig.h -----------------------------------------*- C++
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SMITHRILHASHCONFIG_H
#define KLEE_SMITHRILHASHCONFIG_H

#include "llvm/Support/CommandLine.h"

namespace SmithrilHashConfig {
extern llvm::cl::opt<bool> UseConstructHashSmithril;
} // namespace SmithrilHashConfig
#endif // KLEE_SMITHRILHASHCONFIG_H
