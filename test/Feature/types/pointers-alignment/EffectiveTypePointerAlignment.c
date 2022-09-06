// RUN: %clang %s -emit-llvm -g -c -fsanitize=alignment,null -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --type-system=CXX --use-tbaa --lazy-instantiation=false --align-symbolic-pointers=true --use-gep-expr %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <stdlib.h>

int main() {
  int outer_int_value;
  double outer_float_value;

  void *area = malloc(sizeof(int *) + sizeof(double *));

  int **int_place = area;
  double **double_place = area + sizeof(int *);

  *int_place = 0;
  *double_place = 0;

  klee_make_symbolic(area, sizeof(int *) + sizeof(double *), "area");

  // CHECK-NOT: EffectiveTypePointerAlignment.c:[[@LINE+1]]: either misaligned address for 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
  **int_place = 10;
  // CHECK-NOT: EffectiveTypePointerAlignment.c:[[@LINE+1]]: either misaligned address for 0x{{.*}} or invalid usage of address 0x{{.*}} with insufficient space
  **double_place = 20;
}
