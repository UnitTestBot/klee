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
  /// If type is complex, pull types from it inner types
  // if (llvm::StructType *structType = dyn_cast_or_null<StructType>(type)) {
  //   const StructLayout *structLayout = parent->targetData->getStructLayout(structType);
  //   for (unsigned idx = 0; idx < structType->getNumElements(); ++idx) {
  //     llvm::Type *structTypeMember = structType->getStructElementType(idx);
  //     uint64_t offset = structLayout->getElementOffset(idx);
  //     for (auto &subtypeLocations 
  //         : parent->computeKType(structTypeMember)->innerTypes) {
  //       llvm::Type *subtype = subtypeLocations.first;
  //       const std::vector<uint64_t> &subtypeOffsets = subtypeLocations.second;
  //       for (auto subtypeOffset : subtypeOffsets) {
  //         innerTypes[subtype].push_back(offset + subtypeOffset);
  //       }
  //     }
  //   }
  // }
  /// Type itself can be reached at offset 0
  innerTypes[type].push_back(0);
}

bool KType::isAccessableFrom(KType *accessingType) const {
  return true;
}

llvm::Type *KType::getRawType() const {
  return type;
}