// REQUIRES: posix-runtime
// REQUIRES: z3

// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out  --mock-policy=all --posix-runtime --external-calls=all %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK
// CHECK: ASSERTION FAIL
// CHECK: KLEE: done: completed paths = 2
// CHECK: KLEE: done: generated tests = 3

#include <assert.h>

extern int foo(int x, int y);

int main() {
  int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  if (a == b && foo(a + b, b) != foo(2 * b, a)) {
    assert(0);
  }
  return 0;
}
