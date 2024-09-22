//===-- SeedInfo.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SeedInfo.h"

#include "ExecutionState.h"
#include "Memory.h"
#include "TimingSolver.h"

#include "klee/ADT/KTest.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Support/ErrorHandling.h"

#include <set>

using namespace klee;

void ExecutingSeed::kTestDeleter(KTest *ktest) { kTest_free(ktest); }
