#include "klee/Expr/SymbolicSource.h"

#include "klee/Expr/Expr.h"
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

std::string LazyInitializationSource::toString() const {
  return "LI" + pointer->toString() + "<" + getName() + ">";
};

std::string ArgumentSource::toString() const {
  std::string repr = allocSite.getParent()->getName().str();
  std::string label;
  llvm::raw_string_ostream label_stream(label);
  label_stream << allocSite;
  size_t regNum = label_stream.str().find('%');
  repr += label_stream.str().substr(regNum);
  repr += "#" + llvm::itostr(index);
  return repr + "<" + getName() + ">";
}

std::string InstructionSource::toString() const {
  std::string repr = allocSite.getParent()->getParent()->getName().str();
  std::string label;
  llvm::raw_string_ostream label_stream(label);
  label_stream << allocSite;
  size_t regNum = label_stream.str().find('=');
  repr += label_stream.str().substr(2, regNum - 3);
  repr += "#" + llvm::itostr(index);
  return repr + "<" + getName() + ">";
}

} // namespace klee
