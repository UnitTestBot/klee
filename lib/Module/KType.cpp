#include "klee/Module/KType.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

using namespace klee;
using namespace llvm;

bool KType::isAccessableFrom(const KType &anotherType) const {
    /// If any of the types were not defined, say, that
    /// them compatible
    if (type == nullptr || anotherType.type == nullptr) {
        return true;
    }

    // outs() << "Comparing : ";
    // type->print(outs());
    // outs() << "; ";
    // anotherType.type->print(outs());
    // outs() << "\n";


    // FIXME: we need to collect info about types in struct
    if (type->isStructTy() || anotherType.type->isStructTy()) {
        return true;
    }



    /// If type is char/uint8_t/..., than type is always
    /// can be accessed through it.
    if (anotherType.type->isPointerTy() && 
            anotherType.type->getPointerElementType()->isIntegerTy() &&
            anotherType.type->getPointerElementType()->getIntegerBitWidth() == 8) {
        return true;
    }



    /// Checks for vector types
    if (!anotherType.type->isPointerTy() && 
            anotherType.type->isVectorTy() &&
            anotherType.type->getVectorElementType()->isIntegerTy() &&
            anotherType.type->getVectorElementType()->getIntegerBitWidth() == 8) {
        return true;
    }


    /// Checks for primitive types
    if (!anotherType.type->isPointerTy() &&
            !anotherType.type->isVectorTy() &&
            anotherType.type->isIntegerTy() &&
            anotherType.type->getIntegerBitWidth() == 8) {
        return true;
    }

    return isTypesSimilar(*this, anotherType);
}


bool KType::isTypesSimilar(const KType &firstType, const KType &secondType) {
    if (firstType.type == secondType.type) {
        return true;
    }

    if ((firstType.type->isArrayTy() && 
            firstType.type->getArrayElementType() == secondType.type) || 
            (secondType.type->isArrayTy() && 
            secondType.type->getArrayElementType() == firstType.type)) {
        return true;
    }

    if (secondType.type->isArrayTy() && 
            secondType.type->getArrayElementType() == firstType.type) {
        return true;
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

    // TODO: add check for similarity of class members
    return false;
}
