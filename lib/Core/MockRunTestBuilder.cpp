#include "klee/Core/MockRunTestBuilder.h"

#include "klee/Config/Version.h"
#include "klee/Support/ErrorHandling.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <memory>

klee::MockRunTestBuilder::MockRunTestBuilder(
    const llvm::Module *initModule, const std::string &entrypoint,
    const std::set<std::string> &undefinedVariables,
    const std::set<std::string> &undefinedFunctions)
    : entrypoint(entrypoint), undefinedVariables(undefinedVariables),
      undefinedFunctions(undefinedFunctions) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(7, 0)
  mockModule = CloneModule(*initModule);
#else
  mockModule = CloneModule(initModule);
#endif

  builder = std::make_unique<llvm::IRBuilder<>>(mockModule->getContext());

  llvm::FunctionType *klee_mk_symb_type = llvm::FunctionType::get(
      llvm::Type::getVoidTy(mockModule->getContext()),
      {
          llvm::Type::getInt8PtrTy(mockModule->getContext()),
          llvm::Type::getInt64Ty(mockModule->getContext()),
          llvm::Type::getInt8PtrTy(mockModule->getContext())
      },
      false);
  mockModule->getOrInsertFunction("klee_make_symbolic", klee_mk_symb_type);
  kleeMakeSymbolicFunction = mockModule->getFunction("klee_make_symbolic");
}

llvm::Module* klee::MockRunTestBuilder::build() {
  buildGlobalsDefinition();
  buildFunctionsDefinition();
  if (llvm::verifyModule(*mockModule)) {
    return nullptr;
  }
  return mockModule.get();
}

void klee::MockRunTestBuilder::buildGlobalsDefinition() {
  // Generate definitions for variables. To do it we change user's entrypoint
  // and generate our own main function that calls klee_make_symbolic for all
  // the undefined globals and then call user's entrypoint function.

  llvm::Function *mainFn = mockModule->getFunction(entrypoint);
  if (!mainFn) {
    klee_error("Entry function '%s' not found in module.", entrypoint.c_str());
  }
  mainFn->setName("__klee_mock_wrapped_main");
  mockModule->getOrInsertFunction(entrypoint, mainFn->getFunctionType());
  llvm::Function *newMainFn = mockModule->getFunction(entrypoint);
  if (!newMainFn) {
    klee_error("Failed to generate mock replay file");
  }
  llvm::BasicBlock *globalsInitBlock = llvm::BasicBlock::Create(mockModule->getContext(), "entry", newMainFn);
  builder->SetInsertPoint(globalsInitBlock);
  std::vector<llvm::Value *> args;
  args.reserve(mainFn->arg_size());
  for (llvm::Argument *it = mainFn->arg_begin(); it != mainFn->arg_end(); it++) {
    args.push_back(it);
  }

  for (llvm::Module::global_iterator global = mockModule->global_begin(),
                                     ie = mockModule->global_end();
       global != ie; ++global) {
    const std::string &extName = global->getName().str();
    if (!undefinedVariables.count(extName)) {
      continue;
    }

    llvm::Constant *zeroInitializer = llvm::Constant::getNullValue(global->getValueType());
    if (!zeroInitializer) {
      klee_error("Unable to get zero initializer for '%s'", extName.c_str());
    }
    global->setInitializer(zeroInitializer);

    if (!global->getValueType()->isSized()) {
      continue;
    }
    buildKleeMakeSymbolicCall(global->getBaseObject(), global->getType(), "@obj_" + global->getName().str());
  }

  llvm::CallInst *callMain = builder->CreateCall(mainFn, args);
  builder->CreateRet(callMain);
}

void klee::MockRunTestBuilder::buildFunctionsDefinition() {
  for (const auto &extName : undefinedFunctions) {
    llvm::Function *func = mockModule->getFunction(extName);
    if (!func) {
      klee_error("Unable to find function '%s' in module", extName.c_str());
    }

    if (!func->empty()) {
      continue;
    }

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(mockModule->getContext(), "entry", func);
    builder->SetInsertPoint(BB);

    if (!func->getReturnType()->isSized()) {
      builder->CreateRet(nullptr);
      continue;
    }

    llvm::AllocaInst *allocaInst = builder->CreateAlloca(func->getReturnType(), nullptr, "klee_var");
    buildKleeMakeSymbolicCall(allocaInst, func->getReturnType(), "@call_" + func->getName().str());
    llvm::LoadInst *loadInst = builder->CreateLoad(allocaInst, "klee_var");
    builder->CreateRet(loadInst);
  }
}

void klee::MockRunTestBuilder::buildKleeMakeSymbolicCall(llvm::Value *value, llvm::Type *type, const std::string &name) {
  auto bitcastInst = builder->CreateBitCast(value, llvm::Type::getInt8PtrTy(mockModule->getContext()));
  auto str_name = builder->CreateGlobalString(name);
  auto gep = builder->CreateConstInBoundsGEP2_64(str_name, 0, 0);
  builder->CreateCall(
      kleeMakeSymbolicFunction,
      {bitcastInst,
       llvm::ConstantInt::get(
           mockModule->getContext(),
           llvm::APInt(
               64,
               mockModule->getDataLayout().getTypeStoreSize(type),
               false)),
       gep});
}
