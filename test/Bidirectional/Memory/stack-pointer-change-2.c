// REQUIRES: not-metasmt
// RUN: %clang %s -emit-llvm %O0opt -c -fno-discard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --max-propagations=5 --max-stack-frames=15 --execution-mode=bidirectional --tmp-skip-fns-in-init=false --initialize-in-join-blocks --function-call-reproduce=reach_error --skip-not-lazy-initialized --forward-ticks=0 --backward-ticks=5 --skip-not-symbolic-objects --write-xml-tests --debug-log=rootpob,backward,conflict,closepob,reached,init --debug-constraints=backward %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}

void f() {
  int n = 0;
}

int main() {
  int m;
  int *n = &m;
  klee_make_symbolic(&m, sizeof(m), "m");
  klee_assume(m != 0);
  int k;
  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k == 0);
  f();
  if (k == 0) {
    *n = 0;
  }
  if (m != 0) {
    reach_error();
  }
}

// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT
