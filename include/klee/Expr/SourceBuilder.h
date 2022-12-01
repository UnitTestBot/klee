#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "SymbolicSource.h"
#include <unordered_map>

namespace klee {

class SourceBuilder {
private:
  ref<SymbolicSource> constantSource;
  ref<SymbolicSource> makeSymbolicSource;
  ref<SymbolicAddressSource> symbolicAddressSource;

public:
  SourceBuilder();

  SymbolicSource *constant() const;
  SymbolicSource *makeSymbolic() const;
  SymbolicSource *symbolicAddress() const;
};

};

#endif /* KLEE_EXPRBUILDER_H */
