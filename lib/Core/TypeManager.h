#ifndef KLEE_TYPEMANAGER_H
#define KLEE_TYPEMANAGER_H

#include <memory>
#include <unordered_map>
#include <vector>

namespace llvm {
class Type;
class Function;
}

namespace klee {

class Expr;
class KType;
class KModule;
struct KInstruction;
template <class> class ref;
template <class> class ExprHashMap;

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
  std::unordered_map<llvm::Type *, KType *> typesMap;

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
  
  virtual KType *handleAlloc(ref<Expr> size);
  virtual KType *handleRealloc(KType *, ref<Expr>);

  virtual ~TypeManager() = default;

  static TypeManager *getTypeManager(KModule *);
};

} /*namespace klee*/

#endif /* KLEE_TYPEMANAGER_H */
