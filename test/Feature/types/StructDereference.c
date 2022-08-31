// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --type-system=CXX --use-tbaa --lazy-instantiation=false --use-gep-expr %t.bc 2>&1 | FileCheck %s

#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

typedef struct Node {
  int x;
  struct Node *next;
} Node;

int main() {
  Node node;

  int *pointer;
  klee_make_symbolic(&pointer, sizeof(pointer), "pointer");
  *pointer = 100;

  // CHECK-NOT: ASSERTION FAIL
  assert((void *)pointer != (void *)&pointer);

  // CHECK: x
  if ((void *)pointer == (void *)&node.x) {
    // CHECK-NOT: ASSERTION-FAIL
    assert(node.x == 100);
    printf("x\n");
    return 0;
  }
}
