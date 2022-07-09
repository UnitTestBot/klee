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
            for (auto &subtypeLocations 
                    : parent->computeKType(structTypeMember)->innerTypes) {
                llvm::Type *subtype = subtypeLocations.first;
                const std::vector<uint64_t> &subtypeOffsets = subtypeLocations.second;
                for (auto subtypeOffset : subtypeOffsets) {
                    innerTypes[subtype].push_back(offset + subtypeOffset);
                }
            }
        }
    }
    
    /// Type itself can be reached at offset 0
    innerTypes[type].push_back(0);
}

bool KType::isAccessableFrom(llvm::Type *anotherType) const {
    /// If any of the types were not defined, say, that
    /// them compatible
    if (type == nullptr || anotherType == nullptr) {
        return true;
    }

    // type->print(llvm::outs() << "Comparing ");
    // anotherType->print(llvm::outs() << " with ");
    // llvm::outs() << '\n';

    /// If type is `char`/`uint8_t`/..., than type is always
    /// can be accessed through it. Note, that is not 
    /// safe behavior, as memory access via `char` 
    /// and `int8_t` differs.
    if (anotherType->isPointerTy()) {
        llvm::Type *checkInt8Type = anotherType->getPointerElementType();
        // if (checkInt8Type->isArrayTy()) {
        //     checkInt8Type = checkInt8Type->getArrayElementType();
        // }
        // else     
        if (checkInt8Type->isVectorTy()) {
            checkInt8Type = checkInt8Type->getVectorElementType();
        }

        if (checkInt8Type->isIntegerTy() &&
            checkInt8Type->getIntegerBitWidth() == 8) {
            return true;
        }
    }
    

    return isTypesSimilar(type, anotherType);
}


bool KType::isTypesSimilar(llvm::Type *firstType, llvm::Type *secondType) const {
    /// Ensure that all types were initialized before
    // assert(parent->typesMap.count(firstType) && "Given type was not initialized!");


    for (auto &typeLocations : parent->computeKType(firstType)->innerTypes) {
        llvm::Type *innerType = typeLocations.first;
        if (innerType == nullptr || innerType == secondType) {
            return true;
        }
        // assert(parent->typesMap.count(innerType) && "Given type was not initialized!");
        if (!innerType->isStructTy() && 
            innerType != firstType &&
            isTypesSimilar(innerType, secondType)) {
            return true;
        }
    }
    
            
    if (firstType->isArrayTy() && secondType->isArrayTy()) {
        return firstType->getArrayNumElements() == secondType->getArrayNumElements() &&
                isTypesSimilar(firstType->getArrayElementType(), 
                                secondType->getArrayElementType());
    }


    if (firstType->isArrayTy()) {
        llvm::Type *arrayElementPointer = firstType->getArrayElementType();        
            
        if (isTypesSimilar(arrayElementPointer, secondType)) {
            return true;
        }
    }

    
    if (firstType->isVectorTy()) {
        llvm::Type *vectorElementPointer = PointerType::get(
            firstType->getVectorElementType(),
            parent->targetData->getProgramAddressSpace()
        );
        if (isTypesSimilar(vectorElementPointer, secondType)) {
            return true;
        }
    }


    if (firstType->isPointerTy() && secondType->isPointerTy()) {
        return isTypesSimilar(firstType->getPointerElementType(), 
                                secondType->getPointerElementType());
    }

    return false;
}


std::vector<llvm::Type *> KType::getAccessibleInnerTypes(llvm::Type *typeAccessedFrom) const {
    std::vector<llvm::Type *> result;

    for (auto typeLocations: innerTypes) {
        llvm::Type *type = typeLocations.first;
        
        if (typeAccessedFrom == nullptr || type == nullptr) {
            result.emplace_back(type);
            continue;
        }

        // assert(parent->typesMap.count(type) && "Inner type were not reistered!");
        if (type->isStructTy()) {
            if (type == typeAccessedFrom) {
                result.emplace_back(typeAccessedFrom);
            }
        }
        else {
            const KType *kt = parent->computeKType(type);
            if (kt->isAccessableFrom(typeAccessedFrom)) {
                result.emplace_back(type);
            }
        }
    }

    return result;
}
