// -*- C++ -*-
#ifndef KLEE_LEMMA_H
#define KLEE_LEMMA_H

#include "klee/Core/Interpreter.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/Path.h"

#include <fstream>
#include <memory>
#include <string>

namespace klee {
class KModule;

struct Lemma {
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  Path path;
  ExprOrderedSet constraints;

  Lemma(Path path, ExprOrderedSet constraints)
      : path(path), constraints(constraints) {}

  bool operator==(const Lemma &other) {
    return this->path == other.path && this->constraints == other.constraints;
  }

  bool operator!=(const Lemma &other) { return !(*this == other); }

  ref<Expr> asExpr();

  int compare(const Lemma &b) const {
    if (path == b.path && constraints == b.constraints) {
      return 0;
    }
    return (path < b.path || (path == b.path && constraints < b.constraints))
               ? -1
               : 1;
  }
};

class Summary {
public:
  void addLemma(ref<Lemma> lemma);
  void dumpToFile(KModule *km);
  void readFromFile(KModule *km, ArrayCache *cache);

  Summary(InterpreterHandler *ih) : ih(ih) {}

private:
  std::set<ref<Lemma>> lemmas;
  std::set<ref<Lemma>> dumped;

  std::string getFilename();

  InterpreterHandler *ih;
};

}; // namespace klee

#endif
