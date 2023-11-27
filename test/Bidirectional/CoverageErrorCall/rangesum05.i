// REQUIRES: geq-llvm-12.0

// RUN: %clang %s -emit-llvm %O0opt -fno-discard-value-names -g -c -o %t.bc
// RUN: %clang %testcomp_defs -emit-llvm -g -c -o %t.testcomp_defs.bc
// RUN: %llvmlink %t.testcomp_defs.bc %t.bc -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --strip-unwanted-calls --forward-ticks=0 --delete-dead-loops=false --function-call-reproduce=reach_error --emit-all-errors --execution-mode=bidirectional --write-kqueries --mock-all-externals --external-calls=all --use-forked-solver=false --max-solvers-approx-tree-inc=16 --max-memory=6008 --optimize --skip-not-lazy-initialized --output-dir=%t.klee-out --write-ktests=true  --use-sym-size-alloc=true --cex-cache-validity-cores --symbolic-allocation-threshold=8192 --uninit-memory-test-multiplier=10 --allocate-determ --allocate-determ-size=3072 --allocate-determ-start-address=0x00030000000 --mem-trigger-cof --use-alpha-equivalence=true --track-coverage=all --use-iterative-deepening-search=max-cycles --max-solver-time=10s --max-cycles-before-stuck=15 --only-output-states-covering-new --dump-states-on-halt=all --search=dfs --search=random-state --debug-log=rootpob,backward,conflict,closepob,reached,init %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log


extern void abort(void);

extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern void __assert_perror_fail (int __errnum, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));

void reach_error() { ((void) sizeof ((0) ? 1 : 0), __extension__ ({ if (0) ; else __assert_fail ("0", "rangesum05.c", 18, __extension__ __PRETTY_FUNCTION__); })); }
extern int __VERIFIER_nondet_int();

void init_nondet(int x[5]) {
  int i;
  for (i = 0; i < 5; i++) {
    x[i] = __VERIFIER_nondet_int();
  }
}

int rangesum (int x[5])
{
  int i;
  long long ret;
  ret = 0;
  int cnt = 0;
  for (i = 0; i < 5; i++) {
    if( i > 5/2){
       ret = ret + x[i];
       cnt = cnt + 1;
    }
  }
  if ( cnt !=0)
    return ret / cnt;
  else
    return 0;
}

int main ()
{
  int x[5];
  init_nondet(x);
  int temp;
  int ret;
  int ret2;
  int ret5;

  ret = rangesum(x);

  temp=x[0];x[0] = x[1]; x[1] = temp;
  ret2 = rangesum(x);
  temp=x[0];
  for(int i =0 ; i<5 -1; i++){
     x[i] = x[i+1];
  }
  x[5 -1] = temp;
  ret5 = rangesum(x);

  if(ret != ret2 || ret !=ret5){
    {reach_error();}
  }
  return 1;
}

// CHECK: TRUE POSITIVE
