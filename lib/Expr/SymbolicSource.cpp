#include "klee/Expr/SymbolicSource.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/ExprUtil.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"

#include <vector>

namespace klee {
int SymbolicSource::compare(const SymbolicSource &b) const {
  return internalCompare(b);
}

bool SymbolicSource::equals(const SymbolicSource &b) const {
  return compare(b) == 0;
}

void SymbolicSource::print(llvm::raw_ostream &os) const {
  ExprPPrinter::printSignleSource(os, const_cast<SymbolicSource *>(this));
}

void SymbolicSource::dump() const {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

std::string SymbolicSource::toString() const {
  std::string str;
  llvm::raw_string_ostream output(str);
  this->print(output);
  return str;
}

std::set<const Array *> LazyInitializationSource::getRelatedArrays() const {
  std::vector<const Array *> objects;
  findObjects(pointer, objects);
  return std::set<const Array *>(objects.begin(), objects.end());
}

unsigned ConstantSource::computeHash() {
  unsigned res = 0;
  for (unsigned i = 0, e = constantValues.size(); i != e; ++i) {
    res =
        (res * SymbolicSource::MAGIC_HASH_CONSTANT) + constantValues[i]->hash();
  }
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + getKind();
  hashValue = res;
  return hashValue;
}

unsigned SymbolicSizeConstantSource::computeHash() {
  unsigned res =
      (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + defaultValue;
  hashValue = res;
  return hashValue;
}

unsigned SymbolicSizeConstantAddressSource::computeHash() {
  unsigned res =
      (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + defaultValue;
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + version;
  hashValue = res;
  return hashValue;
}

unsigned MakeSymbolicSource::computeHash() {
  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + version;
  for (unsigned i = 0, e = name.size(); i != e; ++i) {
    res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + name[i];
  }
  hashValue = res;
  return hashValue;
}

unsigned LazyInitializationSource::computeHash() {
  unsigned res =
      (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + pointer->hash();
  hashValue = res;
  return hashValue;
}

unsigned ArgumentSource::computeHash() {
  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + index;
  auto parent = allocSite.getParent();
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) +
        km->functionIDMap.at(parent);
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + allocSite.getArgNo();
  hashValue = res;
  return hashValue;
}

unsigned InstructionSource::computeHash() {
  unsigned res = (getKind() * SymbolicSource::MAGIC_HASH_CONSTANT) + index;
  auto function = allocSite.getParent()->getParent();
  auto kf = km->functionMap.at(function);
  auto block = allocSite.getParent();
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) +
        km->functionIDMap.at(function);
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) + kf->blockMap[block]->id;
  res = (res * SymbolicSource::MAGIC_HASH_CONSTANT) +
        kf->instructionMap[&allocSite]->index;
  hashValue = res;
  return hashValue;
}

} // namespace klee
