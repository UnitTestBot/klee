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

    outs() << "Comparing : ";
    type->print(outs());
    outs() << "; ";
    anotherType.type->print(outs());
    outs() << "\n";

    /// If type is char/uint8_t/..., than type is always
    /// can be accessed through it.
    if (((anotherType.type->isPointerTy() && 
            anotherType.type->getPointerElementType()->isIntegerTy())) &&
            anotherType.type->getPointerElementType()->getIntegerBitWidth() == 8) {
        return true;
    }
    
    /// Same check as above, except it is
    /// for non pointer types
    if ((!anotherType.type->isPointerTy() && 
            anotherType.type->isIntegerTy()) &&
            anotherType.type->getIntegerBitWidth() == 8) {
        return true;
    }

    /// Check if types are similar
    return isTypesSimilar(*this, anotherType);
}


bool KType::isTypesSimilar(const KType &firstType, const KType &secondType) {
    if (firstType.type == secondType.type) {
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

    return false;
}
