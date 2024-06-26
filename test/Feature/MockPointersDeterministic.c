// REQUIRES: z3
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc

// RUN: rm -rf %t.klee-out-1
// RUN: %klee --solver-backend=z3 --output-dir=%t.klee-out-1 --min-number-elements-li=0 --use-sym-size-li --skip-not-lazy-initialized --mock-policy=all --mock-strategy=deterministic %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-1
// CHECK-1: memory error: null pointer exception
// CHECK-1: memory error: out of bound pointer
// CHECK-1: KLEE: done: completed paths = 1
// CHECK-1: KLEE: done: partially completed paths = 2
// CHECK-1: KLEE: done: generated tests = 3

#include <assert.h>

extern int *age();

int main() {
  if (age() != age()) {
    assert(0 && "age is deterministic");
  }
  if (*age() != *age()) {
    assert(0 && "age is deterministic");
  }
  return *age();
}
