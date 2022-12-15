// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=false --use-merge --debug-log-merge --search=nurs:covnew --use-batching-search %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=false --use-merge --debug-log-merge --search=bfs --use-batching-search %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=false --use-merge --debug-log-merge --search=dfs --use-batching-search %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=false --use-merge --debug-log-merge --search=nurs:covnew %t.bc 2>&1 | FileCheck %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=false --use-merge --debug-log-merge --search=random-path %t.bc 2>&1 | FileCheck %s


// CHECK: open merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: close merge:
// CHECK: generated tests = 2{{$}}

#include "klee/klee.h"

int main(int argc, char** args){

  int x;
  int foo = 0;

  klee_make_symbolic(&x, sizeof(x), "x");

  klee_open_merge();

  if (x == 1){
    foo = 5;
  } else if (x == 2){
    klee_close_merge();
    return 6;
  } else {
    foo = 7;
  }

  klee_close_merge();

  return foo;
}