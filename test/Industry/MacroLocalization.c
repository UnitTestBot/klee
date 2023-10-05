#include <stddef.h>

#define RETURN_NULL_OTHERWISE(sth) if ((sth) != 42) {                \
        return NULL;                              \
    }

int *foo(int *status) {
  int st = *status;
  RETURN_NULL_OTHERWISE(st);
  return status;
}

int main(int x) {
  int *result = foo(&x);
  return *result;
}

// REQUIRES: z3
// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --debug-localization --location-accuracy --analysis-reproduce=%s.sarif %t1.bc 2>&1 | FileCheck %s

// CHECK: Industry/MacroLocalization.c:15-15 # 1
// CHECK: Industry/MacroLocalization.c:9:3-9:28 1 # 2
// CHECK: Industry/MacroLocalization.c:9:3-9:3 33 # 3
// CHECK: Industry/MacroLocalization.c:14:17-14:17 56 # 3
// CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace bc7beead5f814c9deac02e8e9e6eef08
