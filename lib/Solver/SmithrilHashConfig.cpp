//===-- SmithrilHashConfig.cpp ---------------------------------------*- C++
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SmithrilHashConfig.h"
#include <klee/Expr/Expr.h>

namespace SmithrilHashConfig {
llvm::cl::opt<bool> UseConstructHashSmithril(
    "use-construct-hash-smithril",
    llvm::cl::desc(
        "Use hash-consing during Smithril query construction (default=true)"),
    llvm::cl::init(true), llvm::cl::cat(klee::ExprCat));
} // namespace SmithrilHashConfig
