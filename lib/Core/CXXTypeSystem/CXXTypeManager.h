#ifndef KLEE_CXXTYPEMANAGER_H
#define KLEE_CXXTYPEMANAGER_H

#include "../TypeManager.h"
#include "klee/Module/KType.h"

#include <string>
#include <map>
#include <unordered_map>

namespace llvm {
  class Type;
}

namespace klee {

namespace cxxtypes {
class KModule;
class KCompositeType;
class CXXKStructType;
class CXXKIntegerType;
class CXXKFloatingPointType;
class CXXKArrayType;
class CXXKPointerType;
}

class CXXTypeManager final : public TypeManager {
private:
  std::unordered_map<std::string, cxxtypes::CXXKStructType*> memberToStruct;

public:
  CXXTypeManager(KModule *);
  virtual KType *getWrappedType(llvm::Type *) override;
  virtual void handleFunctionCall(KFunction *, std::vector<MemoryObject *> &) const override;
};



/**
 * Classes for cpp type system. Rules described below
 * relies on Strict-Aliasing-Rule. See more on this page:
 * https://en.cppreference.com/w/cpp/language/reinterpret_cast
 */
namespace cxxtypes {


class CXXKType : public KType {
protected:
  CXXKType(llvm::Type *, TypeManager *);
  static bool isAccessingFromChar(KType *accessingType);

public:
  virtual bool isAccessableFrom(KType *) const final override;
  virtual bool isAccessableFrom(CXXKType *) const;
  virtual bool innerIsAccessableFrom(CXXKType *) const;
};

/**
 * Composite type represents multuple kinds of types in one memory
 * location. E.g., this type can apper if we use placement new 
 * on array.
 */
class KCompositeType : public CXXKType {
  friend CXXTypeManager;

private:
  std::map<size_t, KType *> typesLocations;

protected:
  KCompositeType(llvm::Type *, TypeManager *);

public:
  void insert(KType *, size_t);
  virtual bool isAccessableFrom(CXXKType *) const override;
};


/**
 * Struct type can be accessed from all other types, that 
 * it might contain inside. Holds additional information
 * about types inside.
 */
class CXXKStructType : public CXXKType {
  friend CXXTypeManager;

protected:
  CXXKStructType(llvm::Type *, TypeManager *);

public:
  std::vector<llvm::Type *> getAccessibleInnerTypes(CXXKType *) const;
  virtual bool isAccessableFrom(CXXKType *) const override;
  virtual bool innerIsAccessableFrom(CXXKType *) const override;
};


/**
 * Function type can be accessed obly from another
 * function type.
 */
class KFunctionType : public CXXKType {
  friend CXXTypeManager;

private:
  CXXKType *returnType;
  std::vector<KType *> arguments;

protected:
  KFunctionType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(CXXKType *) const override;
  virtual bool innerIsAccessableFrom(CXXKType *) const override;
  bool innerIsAccessableFrom(KFunctionType *) const;
};


/**
 * Integer type can be accessed from another integer type
 * of the same type.
 */
class CXXKIntegerType : public CXXKType {
  friend CXXTypeManager;

private:
  const size_t bitness;

protected:
  CXXKIntegerType(llvm::Type *, TypeManager *);


public:  
  virtual bool isAccessableFrom(CXXKType *) const override;
  virtual bool innerIsAccessableFrom(CXXKType *) const override;
  bool innerIsAccessableFrom(CXXKIntegerType *) const;
};


/**
 * Floating point type can be access from another floating
 * point type of the same type.
 */
class CXXKFloatingPointType : public CXXKType {
  friend CXXTypeManager;
protected:
  CXXKFloatingPointType(llvm::Type *, TypeManager *);

public:

  virtual bool isAccessableFrom(CXXKType *) const override;
  virtual bool innerIsAccessableFrom(CXXKType *) const override;
  bool innerIsAccessableFrom(CXXKFloatingPointType *) const;
};


/**
 * Array type can be accessed from another array type of the 
 * same size or pointer type. Types of array elements must be 
 * the same.
 */
class CXXKArrayType : public CXXKType {
  friend CXXTypeManager;

private:
  CXXKType *elementType;
  const size_t arraySize;

protected:
  CXXKArrayType(llvm::Type *, TypeManager *);

public:  
  virtual bool isAccessableFrom(CXXKType *) const override;
  virtual bool innerIsAccessableFrom(CXXKType *) const override;
  bool innerIsAccessableFrom(CXXKPointerType *) const;
  bool innerIsAccessableFrom(CXXKArrayType *) const;
};


/**
 * Pointer Type can be accessed from another pointer type.
 * Pointer elements type must be the same.
 */
class CXXKPointerType : public CXXKType {
  friend CXXTypeManager;
  friend CXXKArrayType;

private:
  CXXKType *elementType;

protected:
  CXXKPointerType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(CXXKType *) const override;
  virtual bool innerIsAccessableFrom(CXXKType *) const override;
  bool innerIsAccessibleFrom(CXXKPointerType *) const;
};

} /*namespace cxxtypes*/


} /*namespace klee*/

#endif /*KLEE_CXXTYPEMANAGER_H*/