#include "klee/Expr/SourceBuilder.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Expr.h"

using namespace klee;

SourceBuilder::SourceBuilder() {
  constantSource = ref<SymbolicSource>(new ConstantSource());
  makeSymbolicSource = ref<SymbolicSource>(new MakeSymbolicSource());
  symbolicAddressSource = ref<SymbolicSource>(new SymbolicAddressSource());
  lazyInitializationSymbolicSource =
      ref<SymbolicSource>(new LazyInitializationSymbolicSource());
}

SymbolicSource *SourceBuilder::constant() const{
  return constantSource.get();
}

SymbolicSource *SourceBuilder::makeSymbolic() const{
  return makeSymbolicSource.get();
}

SymbolicSource *SourceBuilder::symbolicAddress() const {
  return symbolicAddressSource.get();
}

SymbolicSource *SourceBuilder::lazyInitializationMakeSymbolic() const {
  return lazyInitializationSymbolicSource.get();
}
