// REQUIRES: geq-llvm-11.0
// REQUIRES: not-darwin
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=all --mock-strategy=naive %t.bc
// RUN: %clang -c %t.bc -o %t.o
// RUN: %llc %t.klee-out/externals.ll -filetype=obj -o %t_externals.o
// RUN: %objcopy --redefine-syms %t.klee-out/redefinitions.txt %t.o
// RUN: %cc -no-pie %t_externals.o %t.o %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner

extern int variable;

extern int foo(int);

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  return variable + foo(a);
}
