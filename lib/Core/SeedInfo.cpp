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

KTestObject *ExecutingSeed::getNextInput(const MemoryObject *mo, bool byName) {
  if (!input)
    return nullptr;

  if (byName) {
    unsigned i;

    for (i = 0; i < input->numObjects; ++i) {
      KTestObject *obj = &input->objects[i];
      if (std::string(obj->name) == mo->name)
        if (used.insert(obj).second)
          return obj;
    }

    // If first unused input matches in size then accept that as
    // well.
    for (i = 0; i < input->numObjects; ++i)
      if (!used.count(&input->objects[i]))
        break;
    if (i < input->numObjects) {
      KTestObject *obj = &input->objects[i];
      if (ref<ConstantExpr> sizeExpr =
              dyn_cast<ConstantExpr>(mo->getSizeExpr())) {
        if (obj->numBytes == sizeExpr->getZExtValue()) {
          used.insert(obj);
          klee_warning_once(mo,
                            "using seed input %s[%d] for: %s (no name match)",
                            obj->name, obj->numBytes, mo->name.c_str());
          return obj;
        }
      }
    }

    klee_warning_once(mo, "no seed input for: %s", mo->name.c_str());
    return 0;
  } else {
    if (inputPosition >= input->numObjects) {
      return 0;
    } else {
      return &input->objects[inputPosition++];
    }
  }
}
