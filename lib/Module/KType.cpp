#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
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

void KType::imprintType(KType *, ref<Expr>, ref<Expr>) {}