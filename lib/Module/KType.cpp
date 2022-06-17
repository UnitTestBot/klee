#include "klee/Module/KType.h"
#include "klee/Module/KModule.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Casting.h"

#include "llvm/Support/raw_ostream.h"

using namespace klee;
using namespace llvm;

KType::KType(llvm::Type *type, KModule *parent) : type(type), parent(parent) {
    /// If type is complex, pull types from it inner types
    if (llvm::StructType *structType = dyn_cast_or_null<StructType>(type)) {
        const StructLayout *structLayout = parent->targetData->getStructLayout(structType);

        for (unsigned idx = 0; idx < structType->getNumElements(); ++idx) {
            llvm::Type *structTypeMember = structType->getStructElementType(idx);
            uint64_t offset = structLayout->getElementOffset(idx);
            
            for (auto &[subtype, subtypeOffsets] 
                    : parent->typesMap[structTypeMember]->innerTypes) {

                for (auto subtypeOffset : subtypeOffsets) {
                    innerTypes.emplace(subtype, offset + subtypeOffset);
                }
            }

        }
    }
    
    /// Type itself can be reached at offset 0
    innerTypes.emplace(type, 0);
}

bool KType::isAccessableFrom(const llvm::Type *anotherType) const {
    /// If any of the types were not defined, say, that
    /// them compatible
    if (type == nullptr || anotherType == nullptr) {
        return true;
    }

    // outs() << "Comparing : ";
    // type->print(outs());
    // outs() << "; ";
    // anotherType.type->print(outs());
    // outs() << "\n";


    // FIXME: we need to collect info about types in struct
    if (type->isStructTy()) {
        return true;
    }



    /// If type is char/uint8_t/..., than type is always
    /// can be accessed through it.
    if (anotherType->isPointerTy() && 
            anotherType->getPointerElementType()->isIntegerTy() &&
            anotherType->getPointerElementType()->getIntegerBitWidth() == 8) {
        return true;
    }

    /// Checks for vector types
    if (!anotherType->isPointerTy() && 
            anotherType->isVectorTy() &&
            anotherType->getVectorElementType()->isIntegerTy() &&
            anotherType->getVectorElementType()->getIntegerBitWidth() == 8) {
        return true;
    }


    /// Checks for primitive types
    if (!anotherType->isPointerTy() &&
            !anotherType->isVectorTy() &&
            anotherType->isIntegerTy() &&
            anotherType->getIntegerBitWidth() == 8) {
        return true;
    }

    return isTypesSimilar(*this, anotherType);
}


bool KType::isTypesSimilar(const KType &firstType, const KType &secondType) {
    if (firstType.type == secondType.type) {
        return true;
    }

    if (firstType.type->isArrayTy() ) {
        return isTypesSimilar(firstType.type->getArrayElementType(), secondType.type);
    } 

    if (secondType.type->isArrayTy()) {
        return isTypesSimilar(firstType.type, secondType.type->getArrayElementType());
    }
            
    
    if (firstType.type->isArrayTy() && secondType.type->isArrayTy()) {
        return firstType.type->getArrayNumElements() == secondType.type->getArrayNumElements() &&
                isTypesSimilar(firstType.type->getArrayElementType(), 
                                secondType.type->getArrayElementType());
    }

    if (firstType.type->isPointerTy() && secondType.type->isPointerTy()) {
        return isTypesSimilar(firstType.type->getPointerElementType(), 
                                secondType.type->getPointerElementType());
    }

    return false;
}
