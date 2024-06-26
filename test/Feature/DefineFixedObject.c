// RUN: %clang -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include "klee/klee.h"
#include <stdio.h>

#define ADDRESS ((int *)0x0080)

int main() {
  int *p = klee_define_fixed_object(ADDRESS, 4);
  *p = 10;
  printf("*p: %d\n", *p);

  return 0;
}
