// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64
// RUN: test -f %t.klee-out/test000001.ktestjson
// RUN: test -f %t.klee-out/test000002.ktestjson
// RUN: test -f %t.klee-out/test000003.ktestjson
// RUN: test -f %t.klee-out/test000004.ktestjson
// RUN: test -f %t.klee-out/test000005.ktestjson
// RUN: test -f %t.klee-out/test000006.ktestjson
// RUN: test -f %t.klee-out/test000007.ktestjson
// RUN: test -f %t.klee-out/test000008.ktestjson
// RUN: test -f %t.klee-out/test000009.ktestjson
// RUN: test -f %t.klee-out/test000010.ktestjson
// RUN: test -f %t.klee-out/test000011.ktestjson
// RUN: test -f %t.klee-out/test000012.ktestjson
// RUN: test -f %t.klee-out/test000013.ktestjson
// RUN: test -f %t.klee-out/test000014.ktestjson
// RUN: test -f %t.klee-out/test000015.ktestjson
// RUN: test -f %t.klee-out/test000016.ktestjson
// RUN: test -f %t.klee-out/test000017.ktestjson
// RUN: test -f %t.klee-out/test000018.ktestjson
// RUN: test -f %t.klee-out/test000019.ktestjson
// RUN: test -f %t.klee-out/test000020.ktestjson
// RUN: test -f %t.klee-out/test000021.ktestjson
// RUN: not test -f %t.klee-out/test000022.ktestjson

#include <stdio.h>

int main() {
  char a[8];
  fgets(a, 6, stdin);
  if (a[0] == 'u' && a[1] == 't' && a[2] == 'b' && a[3] == 'o' && a[4] == 't') {
    return 1;
  }
  return 0;
}
