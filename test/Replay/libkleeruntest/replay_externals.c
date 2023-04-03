// REQUIRES: z3
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --solver-backend=z3 --skip-not-lazy-initialized --external-calls=all --mock-strategy=deterministic --external-objects --replay-mocks %t.bc
//
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test ! -f %t.klee-out/test000002.ktest
//
// Now try to replay with libkleeRuntest
// RUN: %clang %t.klee-out/replay.ll -Wl,-L %libkleeruntestdir -lkleeRuntest -o %t_runner
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner
// XFAIL: *

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
