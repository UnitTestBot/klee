#include "CXXTypeManager.h"
#include "../Memory.h"
#include "../TypeManager.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Module/KModule.h"
#include "klee/Module/KType.h"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <utility>

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace klee;

enum { DEMANGLER_BUFFER_SIZE = 4096, METADATA_SIZE = 16 };

CXXTypeManager::CXXTypeManager(KModule *parent) : TypeManager(parent) {}

/**
 * Factory method for KTypes. Note, that as there is no vector types in
 * C++, we interpret their type as their elements type.
 */
KType *CXXTypeManager::getWrappedType(llvm::Type *type) {
  if (typesMap.count(type) == 0) {
    cxxtypes::CXXKType *kt = nullptr;

    /* Special case when type is unknown */
    if (type == nullptr) {
      kt = new cxxtypes::CXXKType(type, this);
    } else {
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
      } else if (unwrappedRawType->isIntegerTy()) {
        kt = new cxxtypes::CXXKIntegerType(unwrappedRawType, this);
      } else if (unwrappedRawType->isFloatingPointTy()) {
        kt = new cxxtypes::CXXKFloatingPointType(unwrappedRawType, this);
      } else if (unwrappedRawType->isArrayTy()) {
        kt = new cxxtypes::CXXKArrayType(unwrappedRawType, this);
      } else if (unwrappedRawType->isFunctionTy()) {
        kt = new cxxtypes::CXXKFunctionType(unwrappedRawType, this);
      } else if (unwrappedRawType->isPointerTy()) {
        kt = new cxxtypes::CXXKPointerType(unwrappedRawType, this);
      } else {
        kt = new cxxtypes::CXXKType(unwrappedRawType, this);
      }
    }

    types.emplace_back(kt);
    typesMap.emplace(type, kt);
  }
  return typesMap[type];
}



/**
 * We think about allocated memory as a memory without effective type,
 * i.e. with llvm::Type == nullptr. 
 */
KType *CXXTypeManager::handleAlloc(ref<Expr> size) {
  cxxtypes::CXXKCompositeType *compositeType =
      new cxxtypes::CXXKCompositeType(getWrappedType(nullptr), this, size);
  types.emplace_back(compositeType);
  return compositeType;
}


/**
 * Creates a new type from this: copy all types from given 
 * type, which lie in segment [address, addres + size).
 */
KType *CXXTypeManager::handleRealloc(KType *type, ref<Expr> size) {
  /**
   * C standard says that realloc can be called on memory, that were allocated
   * using malloc, calloc, memalign, or realloc. Therefore, let's fail execution
   * if this did not get appropriate pointer.
   */
  cxxtypes::CXXKCompositeType *reallocFromType = dyn_cast<cxxtypes::CXXKCompositeType>(type);
  assert(reallocFromType && "handleRealloc called on non CompositeType");
  
  cxxtypes::CXXKCompositeType *resultType = dyn_cast<cxxtypes::CXXKCompositeType>(handleAlloc(size));
  assert(resultType && "handleAlloc returned non CompositeType");  
  
  /**
   * If we made realloc from simplified composite type or 
   * allocated object with symbolic size, just return previous,
   * as we do not care about inner structure of given type anymore. 
   */
  resultType->containsSymbolic = reallocFromType->containsSymbolic;
  ConstantExpr *constantSize = llvm::dyn_cast<ConstantExpr>(size); 
  if (reallocFromType->containsSymbolic || !constantSize) {
    resultType->insertedTypes = reallocFromType->insertedTypes;
    return resultType;
  }

  size_t sizeValue = constantSize->getZExtValue();
  for (auto offsetToTypeSizePair = reallocFromType->typesLocations.begin(),
            itEnd = reallocFromType->typesLocations.end();
       offsetToTypeSizePair != itEnd; ++offsetToTypeSizePair) {
    size_t prevOffset = offsetToTypeSizePair->first;
    KType *prevType = offsetToTypeSizePair->second.first;
    size_t prevSize = offsetToTypeSizePair->second.second;

    if (prevOffset < sizeValue) {
      resultType->handleMemoryAccess(prevType, ConstantExpr::alloc(prevOffset, Context::get().getPointerWidth()),
                                     ConstantExpr::alloc(prevSize, Context::get().getPointerWidth()));
    }
  }
  return resultType;
}



void CXXTypeManager::postInitModule() {
  for (auto &global : parent->module->globals()) {
    llvm::SmallVector<llvm::DIGlobalVariableExpression *, METADATA_SIZE>
        globalVariableInfo;
    global.getDebugInfo(globalVariableInfo);
    for (auto metaNode : globalVariableInfo) {
      llvm::DIGlobalVariable *variable = metaNode->getVariable();
      if (!variable) {
        continue;
      }

      llvm::DIType *type = variable->getType();
      if (!type) {
        continue;
      }

      if (type->getTag() == dwarf::Tag::DW_TAG_union_type) {
        KType *kt = getWrappedType(global.getValueType());
        (llvm::cast<cxxtypes::CXXKStructType>(kt))->isUnion = true;
      }

      break;
    }
  }
}

TypeManager *CXXTypeManager::getTypeManager(KModule *module) {
  CXXTypeManager *manager = new CXXTypeManager(module);
  manager->initModule();
  return manager;
}

/* C++ KType base class */
cxxtypes::CXXKType::CXXKType(llvm::Type *type, TypeManager *parent)
    : KType(type, parent) {
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

  assert(llvm::isa<CXXKPointerType>(accessingType) &&
         "Attempt to access to a memory via non-pointer type");

  return llvm::cast<CXXKPointerType>(accessingType)->isPointerToChar();
}

cxxtypes::CXXTypeKind cxxtypes::CXXKType::getTypeKind() const {
  return typeKind;
}

bool cxxtypes::CXXKType::classof(const KType *requestedType) {
  return requestedType->getTypeSystemKind() == TypeSystemKind::CXX;
}

/* Composite type */
cxxtypes::CXXKCompositeType::CXXKCompositeType(KType *type, TypeManager *parent, ref<Expr> objectSize)
    : CXXKType(type->getRawType(), parent) {
  typeKind = CXXTypeKind::COMPOSITE;
  
  if (ConstantExpr *CE = llvm::dyn_cast<ConstantExpr>(objectSize)) {
    size_t size = CE->getZExtValue();
    if (type->getRawType() == nullptr) {
      ++nonTypedMemorySegments[size];
    }
    typesLocations[0] = std::make_pair(type, size);
  }
  else {
    containsSymbolic = true;
  }
  insertedTypes.emplace(type);
}


void cxxtypes::CXXKCompositeType::handleMemoryAccess(KType *type, ref<Expr> offset, ref<Expr> size) {
  /*
   * We want to check adjacent types to ensure, that we did not overlapped
   * nothing, and if we overlapped, move bounds for types or even remove them.
   */
  ConstantExpr *offsetConstant = dyn_cast<ConstantExpr>(offset);
  ConstantExpr *sizeConstant = dyn_cast<ConstantExpr>(size);

  if (offsetConstant && sizeConstant && !containsSymbolic) {
    uint64_t offsetValue= offsetConstant->getZExtValue();
    uint64_t sizeValue = sizeConstant->getZExtValue();
     
    /* We support C-style principle of effective type. 
    TODO:  this might be not appropriate for C++, as C++ has 
    "placement new" operator, but in case of C it is OK. 
    Therefore we assume, that we write in memory with no
    effective type, i.e. does not overloap any objects 
    before. */

    auto it = std::prev(typesLocations.upper_bound(offsetValue));
    
    /* We do not overwrite types in memory */
    if (it->second.first->getRawType() != nullptr) {
      return;
    }
    if (std::next(it) != typesLocations.end() &&
        std::next(it)->first < offsetValue + sizeValue) {
      return;
    }

    size_t tail = 0;

    size_t prevOffsetValue = it->first;
    size_t prevSizeValue = it->second.second;
    
    /* Calculate number of non-typed bytes after object */
    if (prevOffsetValue + prevSizeValue > offsetValue + sizeValue) {
      tail = (prevOffsetValue + prevSizeValue) - (offsetValue + sizeValue);
    }

    /* We will possibly cut this object belowe. So let's 
    decrease counter immediately */
    if (--nonTypedMemorySegments[prevSizeValue] == 0) {
      nonTypedMemorySegments.erase(prevSizeValue);
    }

    /* Calculate space remaining for non-typed memory in the beginning */
    if (offsetValue - prevOffsetValue != 0) {
      it->second.second = std::min(prevSizeValue, offsetValue - prevOffsetValue);
      ++nonTypedMemorySegments[prevSizeValue];
    } else {  
      typesLocations.erase(it);  
    }

    typesLocations[offsetValue] = std::make_pair(type, sizeValue);
    if (typesLocations.count(offsetValue + sizeValue) == 0 && tail != 0) {
      ++nonTypedMemorySegments[tail];
      typesLocations[offsetValue + sizeValue] = std::make_pair(parent->getWrappedType(nullptr), tail);
    }

    if (nonTypedMemorySegments.empty()) {
      insertedTypes.erase(parent->getWrappedType(nullptr));
    }
  }
  else {
    /* 
     * If we have written object by a symbolic address, we will use 
     * simplified representation for Composite Type, as it is too
     * dificult to determine relative location of objects in memory
     * (requires query the solver at least).
    */
    containsSymbolic = true;
  }
  /* We do not want to add nullptr type to composite type */
  if (type->getRawType() != nullptr) {
    insertedTypes.emplace(type);
  }
}

bool cxxtypes::CXXKCompositeType::isAccessableFrom(
    CXXKType *accessingType) const {
  for (auto &it : insertedTypes) {
    if (it->isAccessableFrom(accessingType)) {
      return true;
    }
  }
  return false;
}


bool cxxtypes::CXXKCompositeType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::COMPOSITE);
}

/* Integer type */
cxxtypes::CXXKIntegerType::CXXKIntegerType(llvm::Type *type,
                                           TypeManager *parent)
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::INTEGER;
}

bool cxxtypes::CXXKIntegerType::isAccessableFrom(
    CXXKType *accessingType) const {
  if (llvm::isa<CXXKIntegerType>(accessingType)) {
    return innerIsAccessableFrom(cast<CXXKIntegerType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKIntegerType::innerIsAccessableFrom(
    CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKIntegerType::innerIsAccessableFrom(
    CXXKIntegerType *accessingType) const {
  return (accessingType->type == type);
}

bool cxxtypes::CXXKIntegerType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::INTEGER);
}

/* Floating point type */
cxxtypes::CXXKFloatingPointType::CXXKFloatingPointType(llvm::Type *type,
                                                       TypeManager *parent)
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::FP;
}

bool cxxtypes::CXXKFloatingPointType::isAccessableFrom(
    CXXKType *accessingType) const {
  if (llvm::isa<CXXKFloatingPointType>(accessingType)) {
    return innerIsAccessableFrom(cast<CXXKFloatingPointType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKFloatingPointType::innerIsAccessableFrom(
    CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKFloatingPointType::innerIsAccessableFrom(
    CXXKFloatingPointType *accessingType) const {
  return (accessingType->getRawType() == type);
}

bool cxxtypes::CXXKFloatingPointType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::FP);
}

/* Struct type */
cxxtypes::CXXKStructType::CXXKStructType(llvm::Type *type, TypeManager *parent)
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::STRUCT;
  /* Hard coded union identification, as we can not always
  get this info from metadata. */
  isUnion = type->getStructName().startswith("union.");
}

bool cxxtypes::CXXKStructType::isAccessableFrom(CXXKType *accessingType) const {
  /* FIXME: this is a temporary hack for vtables in C++. Ideally, we
   * should demangle global variables to get additional info, at least
   * that global object is "special" (here it is about vtable).
   */
  if (llvm::isa<CXXKPointerType>(accessingType) &&
      llvm::cast<CXXKPointerType>(accessingType)->isPointerToFunction()) {
    return true;
  }

  if (isUnion) {
    return true;
  }

  for (auto &innerTypesToOffsets : innerTypes) {
    CXXKType *innerType = cast<CXXKType>(innerTypesToOffsets.first);

    /* To prevent infinite recursion */
    if (isa<CXXKStructType>(innerType)) {
      if (innerType == accessingType) {
        return true;
      }
    } else if (innerType->isAccessableFrom(accessingType)) {
      return true;
    }
  }
  return false;
}

bool cxxtypes::CXXKStructType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::STRUCT);
}

/* Array type */
cxxtypes::CXXKArrayType::CXXKArrayType(llvm::Type *type, TypeManager *parent)
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::ARRAY;

  llvm::Type *rawArrayType = llvm::cast<llvm::ArrayType>(type);
  KType *elementKType =
      parent->getWrappedType(rawArrayType->getArrayElementType());
  assert(llvm::isa<CXXKType>(elementKType) &&
         "Type manager returned non CXX type for array element");
  elementType = cast<CXXKType>(elementKType);
  arrayElementsCount = rawArrayType->getArrayNumElements();
}

bool cxxtypes::CXXKArrayType::isAccessableFrom(CXXKType *accessingType) const {
  if (llvm::isa<CXXKArrayType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<CXXKArrayType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(
    CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr) ||
         elementType->isAccessableFrom(accessingType);
}

bool cxxtypes::CXXKArrayType::innerIsAccessableFrom(
    CXXKArrayType *accessingType) const {
  /// TODO: support arrays of unknown size
  return (arrayElementsCount == accessingType->arrayElementsCount) &&
         elementType->isAccessableFrom(accessingType->elementType);
}

bool cxxtypes::CXXKArrayType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::ARRAY);
}

/* Function type */
cxxtypes::CXXKFunctionType::CXXKFunctionType(llvm::Type *type,
                                             TypeManager *parent)
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::FUNCTION;

  assert(type->isFunctionTy() &&
         "Given non-function type to construct KFunctionType!");
  llvm::FunctionType *function = llvm::cast<llvm::FunctionType>(type);
  returnType = llvm::dyn_cast<cxxtypes::CXXKType>(
      parent->getWrappedType(function->getReturnType()));
  assert(returnType != nullptr && "Type manager returned non CXXKType");

  for (auto argType : function->params()) {
    KType *argKType = parent->getWrappedType(argType);
    assert(llvm::isa<cxxtypes::CXXKType>(argKType) &&
           "Type manager return non CXXType for function argument");
    arguments.push_back(cast<cxxtypes::CXXKType>(argKType));
  }
}

bool cxxtypes::CXXKFunctionType::isAccessableFrom(
    CXXKType *accessingType) const {
  if (llvm::isa<CXXKFunctionType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<CXXKFunctionType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKFunctionType::innerIsAccessableFrom(
    CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKFunctionType::innerIsAccessableFrom(
    CXXKFunctionType *accessingType) const {
  unsigned currentArgCount = type->getFunctionNumParams();
  unsigned accessingArgCount = accessingType->type->getFunctionNumParams();

  if (!type->isFunctionVarArg() && currentArgCount != accessingArgCount) {
    return false;
  }

  for (unsigned idx = 0; idx < std::min(currentArgCount, accessingArgCount);
       ++idx) {
    if (type->getFunctionParamType(idx) !=
        accessingType->type->getFunctionParamType(idx)) {
      return false;
    }
  }

  /*
   * FIXME: We need to check return value, but it can differ though in llvm IR.
   * E.g., first member in structs is i32 (...), that can be accessed later
   * by void (...). Need a research how to maintain it properly.
   */
  return true;
}

bool cxxtypes::CXXKFunctionType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::FUNCTION);
}

/* Pointer type */
cxxtypes::CXXKPointerType::CXXKPointerType(llvm::Type *type,
                                           TypeManager *parent)
    : CXXKType(type, parent) {
  typeKind = CXXTypeKind::POINTER;

  elementType =
      cast<CXXKType>(parent->getWrappedType(type->getPointerElementType()));
}

bool cxxtypes::CXXKPointerType::isAccessableFrom(
    CXXKType *accessingType) const {
  if (llvm::isa<CXXKPointerType>(accessingType)) {
    return innerIsAccessableFrom(llvm::cast<CXXKPointerType>(accessingType));
  }
  return innerIsAccessableFrom(accessingType);
}

bool cxxtypes::CXXKPointerType::innerIsAccessableFrom(
    CXXKType *accessingType) const {
  return (accessingType->getRawType() == nullptr);
}

bool cxxtypes::CXXKPointerType::innerIsAccessableFrom(
    CXXKPointerType *accessingType) const {
  return elementType->isAccessableFrom(accessingType->elementType);
}

bool cxxtypes::CXXKPointerType::isPointerToChar() const {
  if (llvm::isa<CXXKIntegerType>(elementType)) {
    return (elementType->getRawType()->getIntegerBitWidth() == 8);
  }
  return false;
}

bool cxxtypes::CXXKPointerType::isPointerToFunction() const {
  if (llvm::isa<CXXKFunctionType>(elementType)) {
    return true;
  }
  return false;
}

bool cxxtypes::CXXKPointerType::classof(const KType *requestedType) {
  if (!llvm::isa<CXXKType>(requestedType)) {
    return false;
  }

  return (llvm::cast<CXXKType>(requestedType)->getTypeKind() ==
          cxxtypes::CXXTypeKind::POINTER);
}
