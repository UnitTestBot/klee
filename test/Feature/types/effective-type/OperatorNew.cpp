// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --type-system=CXX --use-tbaa --lazy-instantiation=false --use-gep-expr %t.bc

#include "klee/klee.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

struct S {
  int a;
  float b = 1.0;
  double c = 2.0;
  int16_t d;
  S(int x) : a(x) {}
};

bool isSameAddress(void *a, void *b) {
  return a == b;
}

int main() {
  S *s = new S(10);

  int *ptr_int;
  klee_make_symbolic(&ptr_int, sizeof(ptr_int), "ptr_int");
  *ptr_int = 100;

  // CHECK: x
  if (isSameAddress(ptr_int, s)) {
    printf("x\n");
    return 0;
  }

  float *ptr_float;
  klee_make_symbolic(&ptr_float, sizeof(ptr_float), "ptr_float");
  *ptr_float = 10.f;

  // CHECK: y
  if (isSameAddress(ptr_float, s)) {
    printf("y\n");
    return 0;
  }

  double *ptr_double;
  klee_make_symbolic(&ptr_double, sizeof(ptr_double), "ptr_double");
  *ptr_double = 120;
  // CHECK-NOT: ASSERTION-FAIL
  assert(!isSameAddress(ptr_double, s));
}