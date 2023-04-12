#include "klee/Core/MockRunTestBuilder.h"

#include "klee/Support/ErrorHandling.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <memory>

klee::MockRunTestBuilder::MockRunTestBuilder(
    const llvm::Module *initModule, std::string mockEntrypoint,
    std::string userEntrypoint, std::map<std::string, llvm::Type *> externals)
    : userModule(initModule), externals(std::move(externals)),
      mockEntrypoint(std::move(mockEntrypoint)),
      userEntrypoint(std::move(userEntrypoint)) {}

std::unique_ptr<llvm::Module> klee::MockRunTestBuilder::build() {
  mockModule = std::make_unique<llvm::Module>(userModule->getName().str() +
                                                  "__klee_externals",
                                              userModule->getContext());
  mockModule->setTargetTriple(userModule->getTargetTriple());
  mockModule->setDataLayout(userModule->getDataLayout());
  builder = std::make_unique<llvm::IRBuilder<>>(mockModule->getContext());

  // Set up entrypoint in new module. Here we'll define globals and then call
  // user's entrypoint.
  llvm::Function *mainFn = userModule->getFunction(userEntrypoint);
  if (!mainFn) {
    klee_error("Entry function '%s' not found in module.",
               userEntrypoint.c_str());
  }
  mockModule->getOrInsertFunction(mockEntrypoint, mainFn->getFunctionType());

  buildGlobalsDefinition();
  buildFunctionsDefinition();
  return std::move(mockModule);
}

void klee::MockRunTestBuilder::buildGlobalsDefinition() {
  llvm::Function *mainFn = mockModule->getFunction(mockEntrypoint);
  if (!mainFn) {
    klee_error("Entry function '%s' not found in module.",
               mockEntrypoint.c_str());
  }
  llvm::BasicBlock *globalsInitBlock =
      llvm::BasicBlock::Create(mockModule->getContext(), "entry", mainFn);
  builder->SetInsertPoint(globalsInitBlock);

  for (const auto &it : externals) {
    if (it.second->isFunctionTy()) {
      continue;
    }
    const std::string &extName = it.first;
    llvm::Type *type = it.second;
    mockModule->getOrInsertGlobal(extName, type);
    llvm::GlobalVariable *global = mockModule->getGlobalVariable(extName);
    if (!global) {
      klee_error("Unable to add global variable '%s' to module",
                 extName.c_str());
    }
    global->setLinkage(llvm::GlobalValue::ExternalLinkage);
    if (!type->isSized()) {
      continue;
    }

    llvm::Constant *zeroInitializer = llvm::Constant::getNullValue(it.second);
    if (!zeroInitializer) {
      klee_error("Unable to get zero initializer for '%s'", extName.c_str());
    }
    global->setInitializer(zeroInitializer);

    llvm::FunctionType *klee_mk_symb_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(mockModule->getContext()),
        {llvm::Type::getInt8PtrTy(mockModule->getContext()),
         llvm::Type::getInt64Ty(mockModule->getContext()),
         llvm::Type::getInt8PtrTy(mockModule->getContext())},
        false);
    llvm::FunctionCallee kleeMakeSymbolicCallee =
        mockModule->getOrInsertFunction("klee_make_symbolic",
                                        klee_mk_symb_type);
    auto bitcastInst = builder->CreateBitCast(
        global, llvm::Type::getInt8PtrTy(mockModule->getContext()));
    auto str_name = builder->CreateGlobalString("@obj_" + extName);
    auto gep = builder->CreateConstInBoundsGEP2_64(str_name, 0, 0);
    builder->CreateCall(
        kleeMakeSymbolicCallee,
        {bitcastInst,
         llvm::ConstantInt::get(
             mockModule->getContext(),
             llvm::APInt(64, mockModule->getDataLayout().getTypeStoreSize(type),
                         false)),
         gep});
  }

  llvm::Function *userMainFn = userModule->getFunction(userEntrypoint);
  if (!userMainFn) {
    klee_error("Entry function '%s' not found in module.",
               userEntrypoint.c_str());
  }
  auto userMainCallee = mockModule->getOrInsertFunction(userEntrypoint, userMainFn->getFunctionType());
  std::vector<llvm::Value *> args;
  args.reserve(userMainFn->arg_size());
  for (llvm::Argument *it = userMainFn->arg_begin();
       it != userMainFn->arg_end(); it++) {
    args.push_back(it);
  }
  llvm::CallInst *callUserMain = builder->CreateCall(userMainCallee, args);
  builder->CreateRet(callUserMain);
}

void klee::MockRunTestBuilder::buildFunctionsDefinition() {
  for (const auto &it : externals) {
    if (!it.second->isFunctionTy()) {
      continue;
    }
    std::string extName = it.first;
    auto *type = llvm::cast<llvm::FunctionType>(it.second);
    mockModule->getOrInsertFunction(extName, type);
    llvm::Function *func = mockModule->getFunction(extName);
    if (!func) {
      klee_error("Unable to find function '%s' in module", extName.c_str());
    }
    if (!func->empty()) {
      continue;
    }
    llvm::BasicBlock *BB =
        llvm::BasicBlock::Create(mockModule->getContext(), "entry", func);
    builder->SetInsertPoint(BB);

    if (!func->getReturnType()->isSized()) {
      builder->CreateRet(nullptr);
      continue;
    }

    llvm::AllocaInst *mockReturnValue =
        builder->CreateAlloca(func->getReturnType(), nullptr);

    llvm::FunctionType *klee_mk_mock_type = llvm::FunctionType::get(
        llvm::Type::getVoidTy(mockModule->getContext()),
        {llvm::Type::getInt8PtrTy(mockModule->getContext()),
         llvm::Type::getInt64Ty(mockModule->getContext()),
         llvm::Type::getInt8PtrTy(mockModule->getContext())},
        false);
    llvm::FunctionCallee kleeMakeSymbolicCallee =
        mockModule->getOrInsertFunction("klee_make_mock", klee_mk_mock_type);
    auto bitcastInst = builder->CreateBitCast(
        mockReturnValue, llvm::Type::getInt8PtrTy(mockModule->getContext()));
    auto str_name = builder->CreateGlobalString("@call_" + extName);
    auto gep = builder->CreateConstInBoundsGEP2_64(str_name, 0, 0);
    builder->CreateCall(
        kleeMakeSymbolicCallee,
        {bitcastInst,
         llvm::ConstantInt::get(
             mockModule->getContext(),
             llvm::APInt(64, mockModule->getDataLayout().getTypeStoreSize(func->getReturnType()),
                         false)),
         gep});
    llvm::LoadInst *loadInst = builder->CreateLoad(mockReturnValue, "klee_var");
    builder->CreateRet(loadInst);
  }
}
