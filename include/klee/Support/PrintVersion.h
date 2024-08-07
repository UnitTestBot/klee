//===-- PrintVersion.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_PRINTVERSION_H
#define KLEE_PRINTVERSION_H

#include "llvm/Support/raw_ostream.h"

namespace klee {
void printVersion(llvm::raw_ostream &OS);
}

#endif /* KLEE_PRINTVERSION_H */
