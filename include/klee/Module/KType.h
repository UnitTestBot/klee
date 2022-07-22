#ifndef KLEE_KTYPE_H
#define KLEE_KTYPE_H

#include <unordered_map>
#include <vector>
namespace llvm {
    class Type;
}

namespace klee {    
class TypeManager;

struct KType {
  friend TypeManager;

public:
  /**
   * Wrapped type.
   */
  llvm::Type *type;
  
  TypeManager *parent;      
  
  /**
   * Innner types. Maps type to their offsets in current
   * type. Should contain type itself and 
   * all types, that can be found in that object.
   * For example, if object of type A contains object 
   * of type B, then all types in B can be accessed via A. 
   */
  std::unordered_map<llvm::Type*, std::vector<uint64_t>> innerTypes;
  
public:
  /**
   * Method to check if 2 types are compatible.
   */
  virtual bool isAccessableFrom(KType *accessingType) const;

  virtual ~KType() = default;     

private:
  KType(llvm::Type *, TypeManager *);
};
}

#endif