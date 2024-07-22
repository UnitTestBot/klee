// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-not-lazy-initialized --use-sym-size-li --min-number-elements-li=1 --only-output-states-covering-new  --skip-not-symbolic-objects --symbolic-allocation-threshold=0 %t.bc >%t.log 2>&1
// RUN: %ktest-tool %t.klee-out/test*.ktest >>%t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"

struct AB {
  int a;
  int b;
  int *c;
  int **d;
};

int *sum(int *a, int *b, int c, struct AB ab, struct AB ab2) {
  // CHECK-DAG: 2024-07-12-symbolic-size-minimization.c:[[@LINE+1]]: memory error: null pointer exception
  *a += *b + c + ab.a + *ab2.c + **ab.d;
  // CHECK-DAG: 2024-07-12-symbolic-size-minimization.c:[[@LINE+1]]: memory error: out of bound pointer
  if (a[7] == 12) {
    *a += 12;
  }

  // Check that exists at least one path with size exactly 32
  // CHECK-DAG: name: 'unnamed'
  // CHECK-DAG: size: 32
  return a;
}

int main() {
  int *a;
  klee_make_symbolic(&a, sizeof(a), "a");
  ////////////////////////////////////////////
  int *b;
  klee_make_symbolic(&b, sizeof(b), "b");
  ////////////////////////////////////////////
  int c;
  klee_make_symbolic(&c, sizeof(c), "c");
  ////////////////////////////////////////////
  struct AB ab;
  klee_make_symbolic(&ab, sizeof(ab), "ab");
  ////////////////////////////////////////////
  struct AB ab2;
  klee_make_symbolic(&ab2, sizeof(ab2), "ab2");
  ////////////////////////////////////////////
  int *utbot_result;
  klee_make_symbolic(&utbot_result, sizeof(utbot_result), "utbot_result");
  utbot_result = sum(a, b, c, ab, ab2);
  return 0;
}
