#ifndef KLEE_TYPAMANGER_H
#define KLEE_TYPAMANGER_H

#include <memory>
#include <vector>
#include <unordered_map>

namespace llvm {
  class Type;
}

namespace klee {

class KType;
class KModule;
struct KFunction;
struct KInstruction;
class MemoryObject;

/**
 * Default class for managing type system.
 * Works with *raw* llvm types. By extending this
 * class you can add more rules to type system.
 */
class TypeManager {
protected:
  KModule *parent;
  std::vector<std::unique_ptr<KType>> types;
  std::unordered_map<llvm::Type*, KType*> typesMap;

public:
  virtual KType *getWrappedType(llvm::Type *);
  virtual void handleFunctionCall(KFunction *, std::vector<MemoryObject *> &) const;

  TypeManager(KModule *);
  virtual ~TypeManager() = default;

private:
  void initTypesFromGlobals();
  void initTypesFromStructs();
  void initTypesFromInstructions();
};

} /*namespace klee*/

#endif /* KLEE_TYPAMANGER_H */