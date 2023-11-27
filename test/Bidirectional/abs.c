// REQUIRES: geq-llvm-12.0

// RUN: %clang %s -emit-llvm %O0opt -fno-discard-value-names -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize=false --execution-mode=bidirectional --max-propagations=3 --max-stack-frames=4 --skip-not-lazy-initialized --skip-not-symbolic-objects --initialize-in-join-blocks=true --search=dfs --use-guided-search=none --debug-log=rootpob,backward,conflict,closepob,reached,init %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log
// RUN: diff %t.klee-out/summary.ksummary %s.ksummary.good

#include "klee/klee.h"
#include <limits.h>

int abs(int x) {
  if (x >= 0) {
    return x;
  }
  return -x;
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), " x ");
  int y = abs(x);
  if (y != INT_MIN && y < 0) {
    assert(0 && " Reached ");
  }
}

// CHECK: KLEE: done: newly summarized locations = 2
