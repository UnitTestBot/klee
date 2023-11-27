// REQUIRES: geq-llvm-12.0

// RUN: %clang %s -emit-llvm %O0opt -c -fno-discard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --execution-mode=bidirectional --initialize-in-join-blocks --function-call-reproduce=reach_error --skip-not-lazy-initialized --skip-not-symbolic-objects --debug-log=rootpob,backward,conflict,closepob,reached,init %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}

int loop(int x) {
  int res = 0;
  for (int i = 0; i < x; ++i) {
    if (res == 1) {
      reach_error();
      return -1;
    }
  }
  return 1;
}

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  return loop(a);
}

// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT: Target: [%entry, reach_error]
