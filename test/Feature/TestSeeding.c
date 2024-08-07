// REQUIRES: z3
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t1.klee-out
// RUN: rm -rf %t2.klee-out
// RUN: rm -rf %t3.klee-out
// RUN: rm -rf %t4.klee-out
// RUN: %klee --output-dir=%t1.klee-out  %t1.bc
// RUN: %klee --output-dir=%t2.klee-out --max-instructions=500 %t1.bc
// RUN: %klee --seed-dir=%t1.klee-out --output-dir=%t3.klee-out  %t1.bc
// RUN: %klee --seed-dir=%t2.klee-out --output-dir=%t4.klee-out  %t1.bc
// RUN: %klee-stats --print-columns 'SolverQueries' --table-format=csv %t3.klee-out > %t3.stats
// RUN: %klee-stats --print-columns 'SolverQueries' --table-format=csv %t4.klee-out > %t4.stats
// RUN: FileCheck -check-prefix=CHECK-NO-MAX-INSTRUCTIONS -input-file=%t3.stats %s
// RUN: FileCheck -check-prefix=CHECK-MAX-INSTRUCTIONS -input-file=%t4.stats %s

#include "klee/klee.h"

int main(int argc, char **argv) {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  for (int i = 0; i < 100; i++) {
    if (a + i == 2 * i) {
      break;
    }
  }
}
// CHECK-NO-MAX-INSTRUCTIONS: SolverQueries
// CHECK-NO-MAX-INSTRUCTIONS: 101
// CHECK-MAX-INSTRUCTIONS: SolverQueries
// CHECK-MAX-INSTRUCTIONS: 172