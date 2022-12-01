#ifndef KLEE_SYMBOLICSOURCE_H
#define KLEE_SYMBOLICSOURCE_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Assignment.h"

#include "llvm/IR/Function.h"

namespace klee {

class Expr;

class SymbolicSource {
public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  enum class Kind {
    Constant,
    MakeSymbolic,
    SymbolicAddress
  };

public:
  virtual ~SymbolicSource() {}
  virtual Kind getKind() const = 0;
  static bool classof(const SymbolicSource *) { return true; }
};

class ConstantSource : public SymbolicSource {
public:
  Kind getKind() const { return Kind::Constant; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Constant;
  }
  static bool classof(const ConstantSource *) { return true; }
};

class MakeSymbolicSource : public SymbolicSource {
public:
  Kind getKind() const { return Kind::MakeSymbolic; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::MakeSymbolic;
  }
  static bool classof(const MakeSymbolicSource *) { return true; }
};

class SymbolicAddressSource: public SymbolicSource {
public:
  Kind getKind() const { return Kind::SymbolicAddress; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::SymbolicAddress;
  }
  static bool classof(const SymbolicAddressSource *) { return true; }
};

}  // End klee namespace

#endif /* KLEE_SYMBOLICSOURCE_H */
