#ifndef KLEE_CXXTYPEMANAGER_H
#define KLEE_CXXTYPEMANAGER_H

#include "../TypeManager.h"
#include "klee/Module/KType.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace llvm {
  class Type;
}

namespace klee {

namespace cxxtypes {

enum CXXTypeKind {
  DEFAULT,
  COMPOSITE,
  STRUCT,
  INTEGER,
  FP, 
  ARRAY,
  POINTER,
  FUNCTION
};

class KModule;
class CXXKCompositeType;
class CXXKStructType;
class CXXKIntegerType;
class CXXKFloatingPointType;
class CXXKArrayType;
class CXXKPointerType;
class CXXKType;
class CXXKFunctionType;
}

class CXXTypeManager final : public TypeManager {
private:
  cxxtypes::CXXKCompositeType *createCompositeType(cxxtypes::CXXKType *);

protected:
  CXXTypeManager(KModule *);

  virtual void postInitModule() override;

public:
  virtual KType *getWrappedType(llvm::Type *) override;
  virtual void handleFunctionCall(KFunction *, std::vector<MemoryObject *> &) override;

  static TypeManager *getTypeManager(KModule *);
};



/**
 * Classes for cpp type system. Rules described below
 * relies on Strict-Aliasing-Rule. See more on this page:
 * https://en.cppreference.com/w/cpp/language/reinterpret_cast
 */
namespace cxxtypes {


class CXXKType : public KType {
  friend CXXTypeManager;
  friend CXXKStructType;

private:
  static bool isAccessingFromChar(CXXKType *accessingType);

protected:
/**
 * Field for llvm RTTI system.
 */
  CXXTypeKind typeKind;

  CXXKType(llvm::Type *, TypeManager *);

public:
 /**
  * Checks the first access to this type from specified.
  * Using isAccessingFromChar and then cast parameter
  * to CXXKType. Method is declared as final because it
  * is an 'edge' between LLVM and CXX type systems.
  */
  virtual bool isAccessableFrom(KType *) const final override;
  virtual bool isAccessableFrom(CXXKType *) const;

  CXXTypeKind getTypeKind() const;

  static bool classof(const KType *);
};

/**
 * Composite type represents multuple kinds of types in one memory
 * location. E.g., this type can apper if we use placement new 
 * on array.
 */
class CXXKCompositeType : public CXXKType {
  friend CXXTypeManager;

private:
  // FIXME: we want to use offsets
  // std::map<size_t, KType *> typesLocations;
  std::unordered_set<KType *> insertedTypes;

protected:
  CXXKCompositeType(KType *, TypeManager *);

public:
  void insert(KType *, size_t);
  virtual bool isAccessableFrom(CXXKType *) const override;

  static bool classof(const KType *);
};


/**
 * Struct type can be accessed from all other types, that 
 * it might contain inside. Holds additional information
 * about types inside.
 */
class CXXKStructType : public CXXKType {
  friend CXXTypeManager;
private:
  bool isUnion = false;

protected:
  CXXKStructType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(CXXKType *) const override;  

  static bool classof(const KType *);
};


/**
 * Function type can be accessed obly from another
 * function type.
 */
class CXXKFunctionType : public CXXKType {
  friend CXXTypeManager;

private:
  CXXKType *returnType;
  std::vector<KType *> arguments;

  bool innerIsAccessableFrom(CXXKType *) const;
  bool innerIsAccessableFrom(CXXKFunctionType *) const;

protected:
  CXXKFunctionType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(CXXKType *) const override;

  static bool classof(const KType *);
};


/**
 * Integer type can be accessed from another integer type
 * of the same type.
 */
class CXXKIntegerType : public CXXKType {
  friend CXXTypeManager;

private:
  bool innerIsAccessableFrom(CXXKType *) const;
  bool innerIsAccessableFrom(CXXKIntegerType *) const;

protected:
  CXXKIntegerType(llvm::Type *, TypeManager *);

public:  
  virtual bool isAccessableFrom(CXXKType *) const override;

  static bool classof(const KType *);
};


/**
 * Floating point type can be access from another floating
 * point type of the same type.
 */
class CXXKFloatingPointType : public CXXKType {
  friend CXXTypeManager;

private:
  bool innerIsAccessableFrom(CXXKType *) const;
  bool innerIsAccessableFrom(CXXKFloatingPointType *) const;

protected:
  CXXKFloatingPointType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(CXXKType *) const override;

  static bool classof(const KType *);
};


/**
 * Array type can be accessed from another array type of the 
 * same size or from type of its elements. Types of array elements
 * must be the same.
 */
class CXXKArrayType : public CXXKType {
  friend CXXTypeManager;

private:
  CXXKType *elementType;
  size_t arraySize;
  
  bool innerIsAccessableFrom(CXXKType *) const;
  bool innerIsAccessableFrom(CXXKArrayType *) const;

protected:
  CXXKArrayType(llvm::Type *, TypeManager *);

public:  
  virtual bool isAccessableFrom(CXXKType *) const override;

  static bool classof(const KType *);
};


/**
 * Pointer Type can be accessed from another pointer type.
 * Pointer elements type must be the same.
 */
class CXXKPointerType : public CXXKType {
  friend CXXTypeManager;

private:
  CXXKType *elementType;
  bool innerIsAccessableFrom(CXXKType *) const;
  bool innerIsAccessableFrom(CXXKPointerType *) const;

protected:
  CXXKPointerType(llvm::Type *, TypeManager *);

public:
  virtual bool isAccessableFrom(CXXKType *) const override;
  bool isPointerToChar() const; 
  bool isPointerToFunction() const;

  static bool classof(const KType *);
};

} /*namespace cxxtypes*/


} /*namespace klee*/

#endif /*KLEE_CXXTYPEMANAGER_H*/