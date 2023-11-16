// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize=false --output-dir=%t.klee-out %t1.bc
// RUN: grep "total queries = 0" %t.klee-out/info

#include <assert.h>

int main() {
  unsigned char x, y;

  klee_make_symbolic(&x, sizeof x, "x");

  y = x;

  // should be exactly 0 query, finally we have enough optimizations
  if (x == 10) {
    assert(y == 10);
  }

  klee_silent_exit(0);
  return 0;
}
