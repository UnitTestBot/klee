#include "klee/klee.h"
#include "klee_rint.h"

float klee_internal_rintf(float arg) {
    return klee_rintf(arg);
}

double klee_internal_rint(double arg) {
    return klee_rint(arg);
}

long double klee_internal_rintl(long double arg) {
    return klee_rintl(arg);
}

float nearbyintf(float arg) {
    return klee_rintf(arg);
}

double nearbyint(double arg) {
    return klee_rint(arg);
}

long double nearbyintl(long double arg) {
    return klee_rintl(arg);
}