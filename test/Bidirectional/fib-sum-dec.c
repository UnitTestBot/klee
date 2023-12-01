// REQUIRES: geq-llvm-12.0

// RUN: %clang %s -emit-llvm %O0opt -c -fno-discard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --solver-backend=z3 --write-kqueries --output-dir=%t.klee-out --execution-mode=bidirectional --max-propagations=3 --max-stack-frames=4 --skip-not-lazy-initialized --skip-not-symbolic-objects --initialize-in-join-blocks=true --use-query-log=all:kquery --debug-log=reached,init %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log
// RUN: sed -n '/\[pob\]/,$p' %t.log | sed -n '1,/KLEE\: done\: newly/ p' > %t.log.tail
// RUN: diff %t.log.tail %s.good

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

int dec(int n) {
  return --n;
}

int sum(int a, int b) {
  return a + b;
}

int fib(int n) {
  if (n == 0 || n == 1)
    return 1;
  return sum(fib(dec(n)), fib(dec(dec(n))));
}

int main() {
  int n = 0;
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_assume(n > 0);
  int f1 = fib(n);
  int f2 = fib(n + 1);
  int f3 = fib(n + 2);
  klee_assert(f1 + f2 == f3);
  return 0;
}

// CHECK: KLEE: done: newly summarized locations = 1
