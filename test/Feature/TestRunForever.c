// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out
// RUN: %klee --output-dir=%t1.klee-out --store-early-states --write-paths --write-sym-paths --search=bfs --max-memory=500 %t1.bc

#include "klee/klee.h"
int a[100];

void rec(int i) {
  if (i == 28) {
    return;
  }
  if (a[i] < 0) {
    rec(i + 1);
  } else if (a[i] > 100) {
    rec(i + 1);
  }
}
int main(int argc, char **argv) {
  klee_make_symbolic(&a, sizeof(a), "a");
  rec(0);
}