#ifndef KLEE_TYPEMANAGER_H
#define KLEE_TYPEMANAGER_H

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
private:
  void initTypesFromGlobals();
  void initTypesFromStructs();
  void initTypesFromInstructions();

protected:
  KModule *parent;
  std::vector<std::unique_ptr<KType>> types;
  std::unordered_map<llvm::Type*, KType*> typesMap;

  TypeManager(KModule *);
  TypeManager(const TypeManager &) = delete;
  TypeManager &operator=(const TypeManager &) = delete;
  TypeManager(TypeManager &&) = delete;
  TypeManager &operator=(TypeManager &&) = delete;
  
  virtual void init();

public:
  virtual KType *getWrappedType(llvm::Type *);
  virtual void handleFunctionCall(KFunction *, std::vector<MemoryObject *> &) const;
 
  virtual ~TypeManager() = default;

  /// FIXME: we need a factory though.
  static TypeManager *getTypeManager(KModule *);
};

} /*namespace klee*/

#endif /* KLEE_TYPEMANAGER_H */
