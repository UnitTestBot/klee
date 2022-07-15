// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000059.ktestjson
// RUN: not test -f %t.klee-out/test000060.ktestjson


#include <stdio.h>

int main() {
  char a[8];
  gets(a);
  if (a[0] == 'u' && a[1] == 't' && a[2] == 'b' && a[3] == 'o' && a[4] == 't') {
    return 1;
  }
  return 0;
}
