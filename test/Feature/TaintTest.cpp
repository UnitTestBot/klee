// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --cex-cache-validity-cores --solver-backend=z3 --check-out-of-memory --suppress-external-warnings --libc=klee --skip-not-lazy-initialized --external-calls=all --output-source=true --output-istats=false --output-stats=false --max-time=1200s --max-sym-size-alloc=32 --max-forks=6400 --max-solver-time=5s --smart-resolve-entry-function --extern-calls-can-return-null --align-symbolic-pointers=false --use-lazy-initialization=only --min-number-elements-li=18 --use-sym-size-li=false --rewrite-equalities=simple --symbolic-allocation-threshold=2048 --search=random-path --max-memory=16000 --mock-mutable-globals=all --mock-strategy=naive --mock-policy=all --annotate-only-external=false --annotations=%annotations --taint-annotations=%taint_annotations  %t1.bc

#include <cstdio>
#include <cstdlib>

int main()
{
    const char *libvar = std::getenv("PATH");
    printf(libvar);
}
