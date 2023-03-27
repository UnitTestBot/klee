#ifndef KLEE_MOCKRUNTESTBUILDER_H
#define KLEE_MOCKRUNTESTBUILDER_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <set>
#include <string>

namespace klee {

class MockRunTestBuilder {
private:
  std::unique_ptr<llvm::Module> mockModule;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  llvm::Function *kleeMakeSymbolicFunction;

  const std::string &entrypoint;
  const std::set<std::string> &undefinedVariables;
  const std::set<std::string> &undefinedFunctions;

  void buildGlobalsDefinition();
  void buildFunctionsDefinition();
  void buildKleeMakeSymbolicCall(llvm::Value *value, llvm::Type *type, const std::string &name);

public:
  explicit MockRunTestBuilder(const llvm::Module *m,
                              const std::string &entrypoint,
                              const std::set<std::string> &undefinedVariables,
                              const std::set<std::string> &undefinedFunctions);

  llvm::Module *build();
};

} // namespace klee

#endif // KLEE_MOCKRUNTESTBUILDER_H
