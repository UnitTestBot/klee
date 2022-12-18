#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "SymbolicSource.h"
#include <unordered_map>

namespace klee {

class SourceBuilder {
private:
  ref<SymbolicSource> constantSource;
  ref<SymbolicSource> makeSymbolicSource;
  ref<SymbolicSource> symbolicAddressSource;
  ref<SymbolicSource> lazyInitializationSymbolicSource;

public:
  SourceBuilder();

  SymbolicSource *constant() const;
  SymbolicSource *makeSymbolic() const;
  SymbolicSource *symbolicAddress() const;
  SymbolicSource *lazyInitializationMakeSymbolic() const;
};

};

#endif /* KLEE_EXPRBUILDER_H */
