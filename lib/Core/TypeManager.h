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
  
  /**
   * Initializes type system for current module.
   */
  void initModule();

  /**
   * Make specified post initialization in initModule(). Note, that 
   * it is intentionally separated from initModule, as initModule
   * order of function calls in it important. By default do nothing.
   */
  virtual void postInitModule();

public:
  virtual KType *getWrappedType(llvm::Type *);
  virtual void handleFunctionCall(KFunction *, std::vector<MemoryObject *> &);
 
  virtual ~TypeManager() = default;

  static TypeManager *getTypeManager(KModule *);
};

} /*namespace klee*/

#endif /* KLEE_TYPEMANAGER_H */
