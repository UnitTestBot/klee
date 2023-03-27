// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc

// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --skip-not-lazy-initialized --external-calls=all --mock-strategy=naive --external-objects %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-1
// CHECK-1: KLEE: done: completed paths = 1
// CHECK-1: KLEE: done: partially completed paths = 0
// CHECK-1: KLEE: done: generated tests = 1


#include <assert.h>

struct Person;

extern struct Person Nikita;
extern struct Person *Andrew;

void* name(struct Person *p) {
  return (char*)p;
}

extern int age(struct Person *p);

int main() {
  if (name(&Nikita) == 0) {
    assert(0 && "Name returned null when pointer is not null");
  }
  if (name(Andrew) == 0 && &Nikita == Andrew) {
    assert(0 && "Name returned null when pointer is not null");
  }
  return age(&Nikita);
}
