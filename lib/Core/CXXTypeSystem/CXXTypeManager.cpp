#include "CXXTypeManager.h"
#include "klee/Module/KType.h"
#include "klee/Module/KModule.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Demangle/Demangle.h"

#include <cassert>


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
      kt = TypeManager::getWrappedType(type);
    }
    else {
      /* Vector types are considered as their elements type */ 
      llvm::Type *unwrappedRawType = type;
      if (unwrappedRawType->isVectorTy()) {
        unwrappedRawType = unwrappedRawType->getVectorElementType();
      }
      
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
        kt = new cxxtypes::KFunctionType(unwrappedRawType, this);
      }
      else if (unwrappedRawType->isPointerTy()) {
        kt = new cxxtypes::CXXKPointerType(unwrappedRawType, this);
      }
      else {
        assert(false && "Unknown kind of type!");
      }
    }
    
    types.emplace_back(kt);
    typesMap.emplace(type, types.back().get());
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


cxxtypes::CXXKType::CXXKType(llvm::Type *type, TypeManager *parent) : KType(type, parent) {}


bool cxxtypes::CXXKType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return true;
}

bool cxxtypes::CXXKType::isAccessableFrom(CXXKType *accessingType) const {
  return true;
}

bool cxxtypes::CXXKType::isAccessableFrom(KType *accessingType) const {
  assert(false && "Attempted to compare raw llvm type with C++ type!");
  return false;
}

bool cxxtypes::CXXKType::isAccessingFromChar(KType *accessingType) {
  llvm::Type *rawType = accessingType->getRawType();

  /* Special case for unknown type */
  if (rawType == nullptr) {
    return true;
  }

  assert(rawType->isPointerTy() && "Attempt to access to a memory via a non-pointer type");

  /* Case for char *, int8_t *, ... */
  if (rawType->getPointerElementType()->isIntegerTy() &&
      rawType->getPointerElementType()->getIntegerBitWidth() == 8) {
    return true;
  }

  return false;
}


/* Composite type */
cxxtypes::KCompositeType::KCompositeType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {}

void cxxtypes::KCompositeType::insert(KType *type, size_t offset) {
  /*
   * We want to check adjacent types to ensure, that we did not overlapped nothing,
   * and if we overlapped, move bounds for types or even remove them. 
   */
  
  /// TODO: types can not overlap 
  size_t typeSize = 0;
  auto typePosition = typesLocations.lower_bound(offset);
}

bool cxxtypes::KCompositeType::isAccessableFrom(CXXKType *accessingType) const {
  if (CXXKType::isAccessingFromChar(accessingType)) {
    return true;
  }

  for (auto &it : typesLocations) {
    if (accessingType->isAccessableFrom(it.second)) {
      return true;
    }
  }
  return false;
}


/* Integer type */
cxxtypes::CXXKIntegerType::CXXKIntegerType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent), bitness(0) {}

bool cxxtypes::CXXKIntegerType::isAccessableFrom(CXXKType *accessingType) const {
  if (isAccessingFromChar(accessingType)) {
    return true;
  }
  return innerIsAccessableFrom(accessingType); 
}

bool cxxtypes::CXXKIntegerType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKIntegerType::innerIsAccessableFrom(CXXKIntegerType *accessingType) const {
  return (accessingType->type == type);
}



/* Floating point type */
cxxtypes::CXXKFloatingPointType::CXXKFloatingPointType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {}

bool cxxtypes::CXXKFloatingPointType::isAccessableFrom(CXXKType *accessingType) const {
  if (isAccessingFromChar(accessingType)) {
    return true;
  }
  return innerIsAccessableFrom(cast<CXXKType>(accessingType));
}

bool cxxtypes::CXXKFloatingPointType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKFloatingPointType::innerIsAccessableFrom(CXXKFloatingPointType *accessingType) const {
  return (accessingType->getRawType() == type);
}



/* Struct type */
cxxtypes::CXXKStructType::CXXKStructType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {}

bool cxxtypes::CXXKStructType::isAccessableFrom(CXXKType *accessingType) const {
  assert(isa<CXXKType>(accessingType) && "Given raw llvm type!");
  if (isAccessingFromChar(accessingType)) {
    return true;
  }

  return innerIsAccessableFrom(cast<CXXKType>(accessingType));
}

bool cxxtypes::CXXKStructType::innerIsAccessableFrom(CXXKType *accessingType) const {
  if (isAccessingFromChar(accessingType)) {
    return true;
  }

  return true;
}

std::vector<llvm::Type *> cxxtypes::CXXKStructType::getAccessibleInnerTypes(CXXKType *accessingType) const {
  std::vector<llvm::Type *> result;
  return result;
}



/* Array type */
cxxtypes::CXXKArrayType::CXXKArrayType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent), arraySize(0) {
  // TODO: get instance for child
}

bool cxxtypes::CXXKArrayType::isAccessableFrom(CXXKType *accessingType) const {
  if (isAccessingFromChar(accessingType)) {
    return true;
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(CXXKArrayType *accessingType) const {
  return true;
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(CXXKPointerType *accessingType) const {
  return elementType->innerIsAccessableFrom(accessingType); 
}



/* Function type */
cxxtypes::KFunctionType::KFunctionType(llvm::Type *type, TypeManager *parent) : CXXKType(type, parent) {
  // TODO: get info about all arguments
}

bool cxxtypes::KFunctionType::isAccessableFrom(CXXKType *accessingType) const {
  if (isAccessingFromChar(accessingType)) {
    return true;
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::KFunctionType::innerIsAccessableFrom(CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::KFunctionType::innerIsAccessableFrom(KFunctionType *accessingType) const {
  /// TODO: support variadic arguments 
  return (accessingType->type == type);
}