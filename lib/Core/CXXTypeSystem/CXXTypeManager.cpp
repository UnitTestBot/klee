#include "CXXTypeManager.h"
#include "klee/Module/KType.h"
#include "klee/Module/KModule.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Demangle/Demangle.h"

#include <cassert>

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace klee;


enum {
  DEMANGLER_BUFFER_SIZE = 4096
};


CXXTypeManager::CXXTypeManager(KModule *parent) : TypeManager(parent) {
} 

/**
 * Factory method for KTypes. Note, that as there is no vector types in
 * C++, we interpret their type as their elements type.  
 */
KType *CXXTypeManager::getWrappedType(llvm::Type *type) {
  if (typesMap.count(type) == 0) {
    KType *kt = nullptr;
    
    /* Special case when type is unknown */
    if (type == nullptr) {
      kt = new cxxtypes::CXXKType(type, this);
    }
    else {
      /* Vector types are considered as their elements type */ 
      llvm::Type *unwrappedRawType = type;
      if (unwrappedRawType->isVectorTy()) {
        unwrappedRawType = unwrappedRawType->getVectorElementType();
      }

      /// TODO: debug
      // type->print(llvm::outs() << "Registered ");
      // unwrappedRawType->print(llvm::outs() << " as ");
      // llvm::outs() << "\n";
      
      if (unwrappedRawType->isStructTy()) {
        kt = new cxxtypes::CXXKStructType(unwrappedRawType, this);
      }
      else if (unwrappedRawType->isIntegerTy()) {
        kt = new cxxtypes::CXXKIntegerType(unwrappedRawType, this);
      }
      else if (unwrappedRawType->isFloatingPointTy()) {
        kt = new cxxtypes::CXXKFloatingPointType(unwrappedRawType, this);
      } 
      else if (unwrappedRawType->isArrayTy()) {
        kt = new cxxtypes::CXXKArrayType(unwrappedRawType, this);
      }
      else if (unwrappedRawType->isFunctionTy()) {
        kt = new cxxtypes::CXXKFunctionType(unwrappedRawType, this);
      }
      else if (unwrappedRawType->isPointerTy()) {
        kt = new cxxtypes::CXXKPointerType(unwrappedRawType, this);
      }
      else {
        /// FIXME: We need to support void * type? 
        /// But we cannot access through this type.
        kt = new cxxtypes::CXXKType(unwrappedRawType, this);
      }
    }
    
    types.emplace_back(kt);
    typesMap.emplace(type, kt);
  }
  return typesMap[type];
}

/**
 * Handles function calls for constructors, as they can modify
 * type, written in memory. Also notice, that whit function
 * takes arguments by non-constant reference to modify types
 * in memory object. 
 */
void CXXTypeManager::handleFunctionCall(KFunction *kf, std::vector<MemoryObject *> &args) const {
  if (!kf->function || !kf->function->hasName() || args.size() == 0) {
    return;
  }

  llvm::ItaniumPartialDemangler demangler;
  if (!demangler.partialDemangle(kf->function->getName().begin()) &&
      demangler.isCtorOrDtor()) {
    size_t size = DEMANGLER_BUFFER_SIZE;
    char buf[DEMANGLER_BUFFER_SIZE];

    /* Determine if it is a ctor */
    if (demangler.getFunctionName(buf, &size)[0] != '~') {
      /// TODO: make composite type in arg[0]
    }
  }
}



TypeManager *CXXTypeManager::getTypeManager(KModule *module) {
  CXXTypeManager *manager = new CXXTypeManager(module);
  manager->initModule();
  return manager;
}



/* C++ KType base class */
cxxtypes::CXXKType::CXXKType(llvm::Type *type, TypeManager *parent) : KType(type, parent) {
  typeSystemKind = TypeSystemKind::CXX;
  typeKind = DEFAULT;
}

bool cxxtypes::CXXKType::isAccessableFrom(CXXKType *accessingType) const {
  return true;
}

bool cxxtypes::CXXKType::isAccessableFrom(KType *accessingType) const {
  assert(accessingType && "Accessing type is nullptr!");
  if (isa<CXXKType>(accessingType)) {
    CXXKType *accessingCXXType = cast<CXXKType>(accessingType);
    if (isAccessingFromChar(accessingCXXType)) {
      return true;
    }

    /* TODO: debug output. Maybe put it in aditional log */
    // type->print(llvm::outs() << "Accessing ");
    // accessingType->getRawType()->print(llvm::outs() << " from ");
    bool ok = isAccessableFrom(accessingCXXType);
    // llvm::outs() << " : " << (ok ? "succeed" : "rejected") << "\n";

    return ok;
  }
  assert(false && "Attempted to compare raw llvm type with C++ type!");
}

bool cxxtypes::CXXKType::isAccessingFromChar(CXXKType *accessingType) {
  /* Special case for unknown type */
  if (accessingType->getRawType() == nullptr) {
    return true;
  }

  assert(llvm::isa<CXXKPointerType>(accessingType) && "Attempt to access to a memory via non-pointer type");

  return llvm::cast<CXXKPointerType>(accessingType)->isPointerToChar();
}

cxxtypes::CXXTypeKind cxxtypes::CXXKType::getTypeKind() const {
  return typeKind;
}

bool cxxtypes::CXXKType::classof(const KType *requestedType) {
  return requestedType->getTypeSystemKind() == TypeSystemKind::CXX;
}




/* Composite type */
cxxtypes::KCompositeType::KCompositeType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  typeKind = CXXTypeKind::COMPOSITE;
} 

void cxxtypes::KCompositeType::insert(KType *type, size_t offset) {
  /*
   * We want to check adjacent types to ensure, that we did not overlapped nothing,
   * and if we overlapped, move bounds for types or even remove them. 
   */
}

bool cxxtypes::KCompositeType::isAccessableFrom(CXXKType *accessingType) const {
  for (auto &it : typesLocations) {
    if (accessingType->isAccessableFrom(it.second)) {
      return true;
    }
  }
  return false;
}

bool cxxtypes::KCompositeType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::COMPOSITE); 
}




/* Integer type */
cxxtypes::CXXKIntegerType::CXXKIntegerType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  typeKind = CXXTypeKind::INTEGER;
}

bool cxxtypes::CXXKIntegerType::isAccessableFrom(CXXKType *accessingType) const {
  if (llvm::isa<CXXKIntegerType>(accessingType)) {
    return innerIsAccessableFrom(cast<CXXKIntegerType>(accessingType)); 
  } 
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKIntegerType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKIntegerType::innerIsAccessableFrom(CXXKIntegerType *accessingType) const {
  return (accessingType->type == type);
}

bool cxxtypes::CXXKIntegerType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::INTEGER); 
}




/* Floating point type */
cxxtypes::CXXKFloatingPointType::CXXKFloatingPointType(llvm::Type *type, TypeManager *parent) 
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::FP;
}

bool cxxtypes::CXXKFloatingPointType::isAccessableFrom(CXXKType *accessingType) const {
  if (llvm::isa<CXXKFloatingPointType>(accessingType)) {
    return innerIsAccessableFrom(cast<CXXKFloatingPointType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKFloatingPointType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKFloatingPointType::innerIsAccessableFrom(CXXKFloatingPointType *accessingType) const {
  return (accessingType->getRawType() == type);
}

bool cxxtypes::CXXKFloatingPointType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::FP); 
}




/* Struct type */
cxxtypes::CXXKStructType::CXXKStructType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  typeKind = CXXTypeKind::STRUCT;
}

bool cxxtypes::CXXKStructType::isAccessableFrom(CXXKType *accessingType) const {  
  for (auto &innerTypesToOffsets : innerTypes) {
    CXXKType *innerType = cast<CXXKType>(innerTypesToOffsets.first);
    
    /* To prevent infinite recursion */
    if (isa<CXXKStructType>(innerType)) {
      if (innerType == accessingType) {
        return true;
      }
    }
    else if (innerType->isAccessableFrom(accessingType)) {
      return true;
    }
  }
  return false;
}



bool cxxtypes::CXXKStructType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::STRUCT); 
}




/* Array type */
cxxtypes::CXXKArrayType::CXXKArrayType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  typeKind = CXXTypeKind::ARRAY;

  llvm::Type *rawArrayType = llvm::cast<llvm::ArrayType>(type);
  KType *elementKType = parent->getWrappedType(rawArrayType->getArrayElementType());
  assert(llvm::isa<CXXKType>(elementKType) && "Type manager returned non CXX type for array element");
  elementType = cast<CXXKType>(elementKType);
  arraySize = rawArrayType->getArrayNumElements(); 
}

bool cxxtypes::CXXKArrayType::isAccessableFrom(CXXKType *accessingType) const {
  if (llvm::isa<CXXKArrayType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<CXXKArrayType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr) || 
    elementType->isAccessableFrom(accessingType);
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(CXXKArrayType *accessingType) const {
  /// TODO: support arrays of unknown size
  return (arraySize == accessingType->arraySize) && elementType->isAccessableFrom(accessingType);
}

bool cxxtypes::CXXKArrayType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::ARRAY); 
}


 

/* Function type */
cxxtypes::CXXKFunctionType::CXXKFunctionType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  typeKind = CXXTypeKind::FUNCTION;

  assert(type->isFunctionTy() && "Given non-function type to construct KFunctionType!");
  llvm::FunctionType *function = llvm::cast<llvm::FunctionType>(type);
  returnType = llvm::dyn_cast<cxxtypes::CXXKType>(parent->getWrappedType(function->getReturnType()));
  assert(returnType != nullptr && "Type manager returned non CXXKType");
  
  for (auto argType : function->params()) {
    KType *argKType = parent->getWrappedType(argType);
    assert(llvm::isa<cxxtypes::CXXKType>(argKType) && "Type manager return non CXXType for function argument");
    arguments.push_back(cast<cxxtypes::CXXKType>(argKType));
  }
}

bool cxxtypes::CXXKFunctionType::isAccessableFrom(CXXKType *accessingType) const {
  if (llvm::isa<CXXKFunctionType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<CXXKFunctionType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKFunctionType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKFunctionType::innerIsAccessableFrom(CXXKFunctionType *accessingType) const {
  /// TODO: support variadic arguments
  return (accessingType->type == type);
}

bool cxxtypes::CXXKFunctionType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::FUNCTION); 
}




/* Pointer type */
cxxtypes::CXXKPointerType::CXXKPointerType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  typeKind = CXXTypeKind::POINTER;

  elementType = cast<CXXKType>(parent->getWrappedType(type->getPointerElementType()));
}

bool cxxtypes::CXXKPointerType::isAccessableFrom(CXXKType *accessingType) const {
  if (llvm::isa<CXXKPointerType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<CXXKPointerType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKPointerType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKPointerType::innerIsAccessableFrom(CXXKPointerType *accessingType) const {
  return elementType->isAccessableFrom(accessingType->elementType);
}

bool cxxtypes::CXXKPointerType::isPointerToChar() const {
  if (llvm::isa<CXXKIntegerType>(elementType)) {
    /// FIXME: we do not want to access raw type
    return (elementType->getRawType()->getIntegerBitWidth() == 8);
  }
  return false;
}

bool cxxtypes::CXXKPointerType::classof(const CXXKType *requestedType) {
  return (requestedType->getTypeKind() == cxxtypes::CXXTypeKind::POINTER); 
}
