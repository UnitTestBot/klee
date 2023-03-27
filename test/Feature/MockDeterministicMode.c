// REQUIRES: z3
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc

// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --solver-backend=z3 --external-calls=all --mock-strategy=deterministic --replay-mocks %t.bc 2>&1 | FileCheck %s -check-prefix=CHECK-1
// CHECK-1: KLEE: done: completed paths = 1
// CHECK-1: KLEE: done: partially completed paths = 0
// CHECK-1: KLEE: done: generated tests = 1

extern char foo(int x);
extern int bar();
extern int baz(float x, float y);
extern void execute(void *, char(*)(int));

struct Person;
extern int getName(struct Person *);

int get() {
  int a;
  return getName((void*)&a);
}

int main() {
  int sum = 0;
  int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
  for (int i = 0; i < 6; i++) {
    switch (i % 3) {
    case 0:
      sum += foo(i % 2);
      b = i;
      break;
    case 1:
      sum += foo(i % 2);
      b = a;
      break;
    case 2:
      sum += baz(2.5 * a, 0.33 * b);
      break;
    }
  }
  if (sum % 2 != 0) {
    return bar();
  }
  execute(&a, foo);
  return 0;
}
