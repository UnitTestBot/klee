// RUN: %clang %s -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --memory-backend=fixed %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --memory-backend=dynamic %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --memory-backend=persistent %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --memory-backend=mixed %t2.bc

/* this test is basically just for coverage and doesn't really do any
   correctness check (aside from testing that the various combinations
   don't crash) */

#include "klee/klee.h"
#include <stdlib.h>

int validate(char *buf, int N) {

  int i;

  for (i = 0; i < N; i++) {
    if (buf[i] == 0) {
      return 0;
    }
  }

  return 1;
}

#ifndef SYMBOLIC_SIZE
#define SYMBOLIC_SIZE 15
#endif
int main(int argc, char **argv) {
  int N = SYMBOLIC_SIZE;
  char *buf = malloc(N);
  int i;

  klee_make_symbolic(buf, N, "buf");
  if (validate(buf, N))
    return buf[0];
  return 0;
}

int other_main(int argc, char **argv) {
  int N = SYMBOLIC_SIZE;
  char *buf = malloc(N);
  int i;

  klee_make_symbolic(buf, N, "buf");
  if (validate(buf, N + 1))
    return buf[0];
  return 0;
}
