void foo() {
  int x = -1;
  if (x > 0) {
    x -= 1;
  } else {
    x += 1;
  }

  return;
}

// REQUIRES: z3
// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --max-stepped-instructions=10 --max-cycles=0 --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
// CHECK: KLEE: WARNING: 50.00% NullPointerException False Positive at trace 1
