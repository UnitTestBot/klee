/* extended Euclid's algorithm */
extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "egcd3-ll.c", 4, "reach_error"); }
extern int __VERIFIER_nondet_int(void);
extern void abort(void);
void assume_abort_if_not(int cond) {
  if(!cond) {abort();}
}
void __VERIFIER_assert(int cond) {
    if (!(cond)) {
    ERROR:
        {reach_error();}
    }
    return;
}

int counter = 0;
int main() {
    int x, y;
    long long a, b, p, q, r, s;
    x = __VERIFIER_nondet_int();
    y = __VERIFIER_nondet_int();
    assume_abort_if_not(x >= 1);
    assume_abort_if_not(y >= 1);

    a = x;
    b = y;
    p = 1;
    q = 0;
    r = 0;
    s = 1;

    while (counter++<10) {
        if (!(b != 0))
            break;
        long long c, k;
        c = a;
        k = 0;

        while (counter++<10) {
            if (!(c >= b))
                break;
            long long d, v;
            d = 1;
            v = b;

            while (counter++<10) {
                __VERIFIER_assert(a == y * r + x * p);
                __VERIFIER_assert(b == x * q + y * s);
                __VERIFIER_assert(a == k * b + c);
                __VERIFIER_assert(v == b * d);

                if (!(c >= 2 * v))
                    break;
                d = 2 * d;
                v = 2 * v;
            }
            c = c - v;
            k = k + d;
        }

        a = b;
        b = c;
        long long temp;
        temp = p;
        p = q;
        q = temp - q * k;
        temp = r;
        r = s;
        s = temp - s * k;
    }
    __VERIFIER_assert(p*x - q*x + r*y - s*y  == a);
    return 0;
}

// It requires bitwuzla because the script currently runs with bitwuzla solver backend
// REQUIRES: bitwuzla
// REQUIRES: not-asan
// REQUIRES: not-msan
// REQUIRES: not-ubsan
// REQUIRES: not-darwin
// RUN: %kleef --property-file=%S/coverage-error-call.prp --max-memory=7000000000 --max-cputime-soft=60 --32 --debug --write-ktests %s 2>&1 | FileCheck %s
// CHECK: KLEE: WARNING: 100.00% Reachable Reachable
