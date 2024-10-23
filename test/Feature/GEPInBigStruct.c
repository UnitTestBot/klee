// RUN: %clang %s -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --max-constant-size-alloc=1000000000 --output-dir=%t.klee-out %t2.bc
// RUN: rm -rf %t.klee-out


#include "klee/klee.h"

struct C {
  int a[5];
  int b;
  long long c;
};
typedef struct C C;

struct B {
  int a[3];
  C b[3];
};
typedef struct B B;

struct A {
  B a;
  int b[2];
};
typedef struct A A;

int main() {
  A a[100000];
  int w_index, r_index;
  klee_make_symbolic(&w_index, sizeof(w_index), "w_index");
  klee_make_symbolic(&r_index, sizeof(r_index), "r_index");

  a[4].a.b[1].a[w_index] = 3;

  if (w_index == r_index) {
    klee_assert(a[4].a.b[1].a[r_index] == 3);
  }

  return 0;
}
