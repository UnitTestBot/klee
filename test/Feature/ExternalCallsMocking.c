// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=all --mock-external-calls %t.bc 2>&1 | FileCheck %s
// CHECK-NOT: failed external call
// CHECK: KLEE: done: generated tests = 1

struct Pair {
  int x, y;
};

struct Pair divide(int x, int y);

int get_sign(int x);

void increase(int* x);

int main() {
  int x, y;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");
  struct Pair ans = divide(x, y);
  increase(&ans.x);
  return get_sign(ans.y);
}