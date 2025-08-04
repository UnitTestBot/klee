// XFAIL: true
// do not support good symbolic memory currently
// RUN: %clang %s -emit-llvm %O0opt -c -fno-discard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --max-propagations=3 --max-stack-frames=4 --execution-mode=bidirectional --tmp-skip-fns-in-init=false --initialize-in-join-blocks --function-call-reproduce=reach_error --skip-not-lazy-initialized --forward-ticks=0 --backward-ticks=5 --skip-not-symbolic-objects --use-visitor-hash=false --write-xml-tests --debug-log=rootpob,backward,conflict,closepob,reached,init --debug-constraints=backward %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}

int *my_alloca() {
  int *a = malloc(sizeof(int));
  return a;
}

int main() {
  int *x = my_alloca();
  int *y = my_alloca();
  if (y != x) {
    reach_error();
  }
  free(x);
  free(y);
}

// CHECK: [TRUE POSITIVE]
