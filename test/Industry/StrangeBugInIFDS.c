#include <stddef.h>
#include <stdlib.h>

typedef struct { int magic; } Struct;

void Exec(void **para) {
  if (para == NULL)
    return;             // event 3
  *para = malloc(sizeof(Struct));
}

void* Alloc() {
  void *buf = NULL;     // event 1
  Exec(&buf);           // event 2
  return buf;           // event 4
}

void Init() {
  Struct *p = Alloc();
  p->magic = 42;        // event 5|
}
// REQUIRES: z3
// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --analysis-reproduce=%s.sarif %t1.bc 2>&1 | FileCheck %s

// CHECK: KLEE: WARNING: 100.00% (MayBeNullPointerException|NullPointerException) False Positive at trace somebigid123
