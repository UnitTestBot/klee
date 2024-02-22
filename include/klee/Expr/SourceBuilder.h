#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "klee/Expr/SymbolicSource.h"

#include "llvm/IR/Function.h"

namespace klee {

class KInstruction;
class KGlobalVariable;

template <typename T, typename Eq> class SparseStorage;
template <typename T> class ref;

class SourceBuilder {
public:
  SourceBuilder() = delete;

  static ref<SymbolicSource>
  constant(SparseStorage<ref<ConstantExpr>> constantValues);

  static ref<SymbolicSource> uninitialized(unsigned version,
                                           const KInstruction *allocSite);
  static ref<SymbolicSource>
  symbolicSizeConstantAddress(unsigned version, const KInstruction *allocSite,
                              ref<Expr> size);
  static ref<SymbolicSource>
  symbolicSizeConstantAddress(unsigned version,
                              const KGlobalVariable *allocSite, ref<Expr> size);

  static ref<SymbolicSource> makeSymbolic(const std::string &name,
                                          unsigned version);
  static ref<SymbolicSource> lazyInitializationAddress(ref<Expr> pointer);
  static ref<SymbolicSource> lazyInitializationSize(ref<Expr> pointer);
  static ref<SymbolicSource> lazyInitializationContent(ref<Expr> pointer);
  static ref<SymbolicSource> argument(const llvm::Argument &_allocSite,
                                      int _index, KModule *km);
  static ref<SymbolicSource> instruction(const llvm::Instruction &_allocSite,
                                         int _index, KModule *km);
  static ref<SymbolicSource> value(const llvm::Value &_allocSite, int _index,
                                   KModule *km);
  static ref<SymbolicSource> irreproducible(const std::string &name);
  static ref<SymbolicSource> mockNaive(const KModule *kModule,
                                       const llvm::Function &kFunction,
                                       unsigned version);
  static ref<SymbolicSource>
  mockDeterministic(const KModule *kModule, const llvm::Function &kFunction,
                    const std::vector<ref<Expr>> &args);
  static ref<SymbolicSource> alpha(int _index);
};

}; // namespace klee

#endif /* KLEE_EXPRBUILDER_H */
