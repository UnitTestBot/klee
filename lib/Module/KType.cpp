#include "klee/Module/KType.h"
#include "klee/Module/KModule.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace klee;
using namespace llvm;

KType::KType(llvm::Type *type, TypeManager *parent) : type(type), parent(parent) {
  typeSystemKind = TypeSystemKind::LLVM;

  /* Type itself can be reached at offset 0 */
  innerTypes[this].emplace_back(0);
}

bool KType::isAccessableFrom(KType *accessingType) const {
  return true;
}

llvm::Type *KType::getRawType() const {
  return type;
}

KType::TypeSystemKind KType::getTypeSystemKind() const {
  return typeSystemKind; 
}

std::vector<KType *> KType::getAccessableInnerTypes(KType *accessingType) const {
  std::vector<KType *> result;
  for (auto &innerTypesToOffsets : innerTypes) {
    KType *innerType = innerTypesToOffsets.first;
    if (innerType->isAccessableFrom(accessingType)) {
      result.emplace_back(innerType);
    }
  }
  return result;
}