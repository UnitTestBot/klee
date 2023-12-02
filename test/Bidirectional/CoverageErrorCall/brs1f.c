// REQUIRES: geq-llvm-12.0

// RUN: %clang %s -emit-llvm %O0opt -fno-discard-value-names -g -c -o %t.bc
// RUN: %clang %testcomp_defs -emit-llvm -g -c -o %t.testcomp_defs.bc
// RUN: %llvmlink %t.testcomp_defs.bc %t.bc -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --strip-unwanted-calls --forward-ticks=0 --delete-dead-loops=false --function-call-reproduce=reach_error --emit-all-errors --execution-mode=bidirectional --write-kqueries --mock-all-externals --external-calls=all --use-forked-solver=false --max-solvers-approx-tree-inc=16 --max-memory=6008 --optimize --skip-not-lazy-initialized --output-dir=%t.klee-out --write-ktests=true  --use-sym-size-alloc=true --cex-cache-validity-cores --symbolic-allocation-threshold=8192 --uninit-memory-test-multiplier=10 --allocate-determ --allocate-determ-size=3072 --allocate-determ-start-address=0x00030000000 --mem-trigger-cof --use-alpha-equivalence=true --track-coverage=all --use-iterative-deepening-search=max-cycles --max-solver-time=10s --max-cycles-before-stuck=15 --only-output-states-covering-new --dump-states-on-halt=all --search=dfs --search=random-state --debug-log=rootpob,backward,conflict,closepob,reached,init %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

/*
 * Benchmarks contributed by Divyesh Unadkat[1,2], Supratik Chakraborty[1], Ashutosh Gupta[1]
 * [1] Indian Institute of Technology Bombay, Mumbai
 * [2] TCS Innovation labs, Pune
 *
 */

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
void reach_error() { __assert_fail("0", "brs1f.c", 10, "reach_error"); }
extern void abort(void);
void assume_abort_if_not(int cond) {
  if (!cond) {
    abort();
  }
}
void __VERIFIER_assert(int cond) {
  if (!(cond)) {
  ERROR : {
    reach_error();
    abort();
  }
  }
}
extern int __VERIFIER_nondet_int(void);
void *malloc(unsigned int size);

int N;

int main() {
  N = __VERIFIER_nondet_int();
  if (N <= 0)
    return 1;
  assume_abort_if_not(N <= 2147483647 / sizeof(int));

  int i;
  long long sum[1];
  int *a = malloc(sizeof(int) * N);

  for (i = 0; i < N; i++) {
    if (i % 1 == 0) {
      a[i] = 1;
    } else {
      a[i] = 0;
    }
  }

  for (i = 0; i < N; i++) {
    if (i == 0) {
      sum[0] = N / 2 + 10;
    } else {
      sum[0] = sum[0] + a[i];
    }
  }
  __VERIFIER_assert(sum[0] <= N);
  return 1;
}

// CHECK: TRUE POSITIVE
