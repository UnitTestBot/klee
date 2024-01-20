#include "klee/Core/MockBuilder.h"

#include "klee/Config/Version.h"
#include "klee/Module/Annotation.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BuildLibCalls.h"

#include <memory>
#include <utility>

namespace klee {

template <typename T>
void inline removeAliases(const llvm::Module *userModule,
                          std::map<std::string, T *> &externals) {
  for (const auto &alias : userModule->aliases()) {
    auto it = externals.find(alias.getName().str());
    if (it != externals.end()) {
      externals.erase(it);
    }
  }
}

std::map<std::string, llvm::FunctionType *>
MockBuilder::getExternalFunctions() {
  std::map<std::string, llvm::FunctionType *> externals;
  for (const auto &f : userModule->functions()) {
    if (f.isDeclaration() && !f.use_empty() &&
        !ignoredExternals.count(f.getName().str())) {
      // NOTE: here we detect all the externals, even linked.
      externals.insert(std::make_pair(f.getName(), f.getFunctionType()));
    }
  }
  removeAliases(userModule, externals);

  return externals;
}

std::map<std::string, llvm::Type *> MockBuilder::getExternalGlobals() {
  std::map<std::string, llvm::Type *> externals;
  for (const auto &global : userModule->globals()) {
    if (global.isDeclaration() &&
        !ignoredExternals.count(global.getName().str())) {
      externals.insert(std::make_pair(global.getName(), global.getValueType()));
    }
  }
  removeAliases(userModule, externals);

  for (const auto &e : externals) {
    klee_message("Mocking external variable %s", e.first.c_str());
  }

  return externals;
}

MockBuilder::MockBuilder(
    const llvm::Module *initModule, const Interpreter::ModuleOptions &opts,
    const Interpreter::InterpreterOptions &interpreterOptions,
    const std::set<std::string> &ignoredExternals,
    std::vector<std::pair<std::string, std::string>> &redefinitions,
    InterpreterHandler *interpreterHandler,
    std::set<std::string> &mainModuleFunctions,
    std::set<std::string> &mainModuleGlobals,
    const AnnotationsData &annotationsData)
    : userModule(initModule), opts(opts),
      interpreterOptions(interpreterOptions),
      ignoredExternals(ignoredExternals), redefinitions(redefinitions),
      interpreterHandler(interpreterHandler),
      mainModuleFunctions(mainModuleFunctions),
      mainModuleGlobals(mainModuleGlobals), annotationsData(annotationsData) {}

std::unique_ptr<llvm::Module> MockBuilder::build() {
  initMockModule();
  buildMockMain();
  buildExternalFunctionsDefinitions();

  if (!mockModule) {
    klee_error("Unable to generate mocks");
  }

  {
    std::unique_ptr<llvm::raw_fd_ostream> of(
        interpreterHandler->openOutputFile("redefinitions.txt"));
    for (const auto &i : redefinitions) {
      *of << i.first << " " << i.second << "\n";
    }
  }

  {
    auto mainFn = mockModule->getFunction(opts.MainCurrentName);
    mainFn->setName(opts.EntryPoint);
    std::unique_ptr<llvm::raw_fd_ostream> of(
        interpreterHandler->openOutputFile("externals.ll"));
    *of << *mockModule;
    mainFn->setName(opts.MainCurrentName);
  }

  return std::move(mockModule);
}

void MockBuilder::initMockModule() {
  mockModule = std::make_unique<llvm::Module>(userModule->getName().str() +
                                                  "__klee_externals",
                                              userModule->getContext());
  mockModule->setTargetTriple(userModule->getTargetTriple());
  mockModule->setDataLayout(userModule->getDataLayout());
  builder = std::make_unique<llvm::IRBuilder<>>(mockModule->getContext());
}

// Set up entrypoint in new module. Here we'll define external globals and then
// call user's entrypoint.
void MockBuilder::buildMockMain() {
  mainModuleFunctions.insert(opts.MainNameAfterMock);
  llvm::Function *userMainFn = userModule->getFunction(opts.MainCurrentName);
  if (!userMainFn) {
    klee_error("Entry function '%s' not found in module.",
               opts.MainCurrentName.c_str());
  }
  userMainFn->setName(opts.MainNameAfterMock);

  mockModule->getOrInsertFunction(opts.MainCurrentName,
                                  userMainFn->getFunctionType(),
                                  userMainFn->getAttributes());
  llvm::Function *mockMainFn = mockModule->getFunction(opts.MainCurrentName);
  if (!mockMainFn) {
    klee_error("Mock: Entry function '%s' not found in module",
               opts.MainCurrentName.c_str());
  }
  mockMainFn->setDSOLocal(true);
  auto globalsInitBlock =
      llvm::BasicBlock::Create(mockModule->getContext(), "", mockMainFn);
  builder->SetInsertPoint(globalsInitBlock);
  // Define all the external globals
  if (interpreterOptions.Mock == MockPolicy::All ||
      interpreterOptions.MockMutableGlobals == MockMutableGlobalsPolicy::All) {
    buildExternalGlobalsDefinitions();
  }

  auto userMainCallee = mockModule->getOrInsertFunction(
      opts.MainNameAfterMock, userMainFn->getFunctionType());
  std::vector<llvm::Value *> args;
  args.reserve(userMainFn->arg_size());
  for (auto &arg : mockMainFn->args()) {
    args.push_back(&arg);
  }
  auto callUserMain = builder->CreateCall(userMainCallee, args);
  builder->CreateRet(callUserMain);
}

void MockBuilder::buildExternalGlobalsDefinitions() {
  auto externalGlobals = getExternalGlobals();
  for (const auto &[extName, type] : externalGlobals) {
    mockModule->getOrInsertGlobal(extName, type);
    auto *global = mockModule->getGlobalVariable(extName);
    if (!global) {
      klee_error("Mock: Unable to add global variable '%s' to module",
                 extName.c_str());
    }
    mainModuleGlobals.insert(extName);
    global->setLinkage(llvm::GlobalValue::ExternalLinkage);
    if (!type->isSized()) {
      continue;
    }

    auto *zeroInitializer = llvm::Constant::getNullValue(type);
    if (!zeroInitializer) {
      klee_error("Mock: Unable to get zero initializer for '%s'",
                 extName.c_str());
    }
    global->setInitializer(zeroInitializer);
    buildCallKleeMakeSymbolic("klee_make_symbolic", global, type,
                              "external_" + extName);
  }
}

void MockBuilder::buildExternalFunctionsDefinitions() {
  std::map<std::string, llvm::FunctionType *> externalFunctions;
  if (interpreterOptions.Mock == MockPolicy::All) {
    externalFunctions = getExternalFunctions();
  }

  if (!opts.AnnotateOnlyExternal) {
    for (const auto &annotation : annotationsData.annotations) {
      llvm::Function *func = userModule->getFunction(annotation.first);
      if (func) {
        auto ext = externalFunctions.find(annotation.first);
        if (ext == externalFunctions.end()) {
          externalFunctions[annotation.first] = func->getFunctionType();
        }
      }
    }
  }

  for (const auto &[extName, type] : externalFunctions) {
    mockModule->getOrInsertFunction(extName, type);
    llvm::Function *func = mockModule->getFunction(extName);
    if (!func) {
      klee_error("Mock: Unable to find function '%s' in module",
                 extName.c_str());
    }
    if (func->isIntrinsic()) {
      klee_message("Mock: Skip intrinsic function '%s'", extName.c_str());
      continue;
    }
    mainModuleFunctions.insert(extName);
    if (!func->empty()) {
      continue;
    }
    auto *BB =
        llvm::BasicBlock::Create(mockModule->getContext(), "entry", func);
    builder->SetInsertPoint(BB);

    const auto nameToAnnotations = annotationsData.annotations.find(extName);
    if (nameToAnnotations != annotationsData.annotations.end()) {
      klee_message("Annotation function %s", extName.c_str());
      const auto &annotation = nameToAnnotations->second;

      buildAnnotationForExternalFunctionArgs(func, annotation.argsStatements);
      buildAnnotationForExternalFunctionReturn(func,
                                               annotation.returnStatements);
      buildAnnotationForExternalFunctionProperties(func, annotation.properties);
    } else {
      klee_message("Mocking external function %s", extName.c_str());
      // Default annotation for externals return
      buildAnnotationForExternalFunctionReturn(
          func, {std::make_shared<Statement::MaybeInitNull>()});
    }
  }
}

void MockBuilder::buildCallKleeMakeSymbolic(
    const std::string &kleeMakeSymbolicFunctionName, llvm::Value *source,
    llvm::Type *type, const std::string &symbolicName) {
  auto *kleeMakeSymbolicName = llvm::FunctionType::get(
      llvm::Type::getVoidTy(mockModule->getContext()),
      {llvm::Type::getInt8PtrTy(mockModule->getContext()),
       llvm::Type::getInt64Ty(mockModule->getContext()),
       llvm::Type::getInt8PtrTy(mockModule->getContext())},
      false);
  auto kleeMakeSymbolicCallee = mockModule->getOrInsertFunction(
      kleeMakeSymbolicFunctionName, kleeMakeSymbolicName);
  auto bitCastInst = builder->CreateBitCast(
      source, llvm::Type::getInt8PtrTy(mockModule->getContext()));
  auto globalSymbolicName = builder->CreateGlobalString("@" + symbolicName);
  auto gep = builder->CreateConstInBoundsGEP2_64(
      globalSymbolicName->getValueType(), globalSymbolicName, 0, 0);
  builder->CreateCall(
      kleeMakeSymbolicCallee,
      {bitCastInst,
       llvm::ConstantInt::get(
           mockModule->getContext(),
           llvm::APInt(64, mockModule->getDataLayout().getTypeStoreSize(type),
                       false)),
       gep});
}

std::pair<llvm::Value *, llvm::Value *>
MockBuilder::goByOffset(llvm::Value *value,
                        const std::vector<std::string> &offset) {
  llvm::Value *prev = nullptr;
  llvm::Value *current = value;
  for (const auto &inst : offset) {
    if (inst == "*") {
      if (!current->getType()->isPointerTy()) {
        klee_error("Incorrect annotation offset.");
      }
      prev = current;
      current = builder->CreateLoad(current->getType()->getPointerElementType(),
                                    current);
    } else if (inst == "&") {
      auto addr = builder->CreateAlloca(current->getType());
      prev = current;
      current = builder->CreateStore(current, addr);
    } else {
      const size_t index = std::stol(inst);
      if (!(current->getType()->isPointerTy() ||
            current->getType()->isArrayTy())) {
        klee_error("Incorrect annotation offset.");
      }
      prev = current;
      current = builder->CreateConstInBoundsGEP1_64(current->getType(), current,
                                                    index);
    }
  }
  return {prev, current};
}

inline llvm::Type *getTypeByOffset(llvm::Type *value,
                                   const std::vector<std::string> &offset) {
  llvm::Type *current = value;
  for (const auto &inst : offset) {
    if (inst == "*") {
      if (!current->isPointerTy()) {
        return nullptr;
      }
      current = current->getPointerElementType();
    } else if (inst == "&") {
      // no needed
    } else {
      const size_t index = std::stol(inst);
      if (current->isArrayTy() || current->isPointerTy()) {
        current = current->getContainedType(index);
      } else {
        return nullptr;
      }
    }
  }
  return current;
}

inline bool isCorrectStatements(const std::vector<Statement::Ptr> &statements,
                                const llvm::Argument *arg) {
  return std::any_of(statements.begin(), statements.end(),
                     [arg](const Statement::Ptr &statement) {
                       auto argType =
                           getTypeByOffset(arg->getType(), statement->offset);
                       switch (statement->getKind()) {
                       case Statement::Kind::Deref:
                       case Statement::Kind::InitNull:
                       case Statement::Kind::TaintOutput:
                       case Statement::Kind::TaintPropagation:
                         return argType->isPointerTy();
                       case Statement::Kind::AllocSource:
                         assert(false);
                       case Statement::Kind::Unknown:
                       default:
                         return true;
                       }
                     });
}

bool tryAlign(llvm::Function *func,
              const std::vector<std::vector<Statement::Ptr>> &statements,
              std::vector<std::vector<Statement::Ptr>> &res) {
  if (func->arg_size() == statements.size()) {
    res = statements;
    return true;
  }

  for (size_t i = 0, j = 0; j < func->arg_size() && i < statements.size();) {
    while (true) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(10, 0)
      auto arg = func->getArg(j);
#else
      auto arg = &func->arg_begin()[j];
#endif
      if (isCorrectStatements(statements[i], arg)) {
        break;
      }
      res.emplace_back();
      j++;
      if (j >= func->arg_size()) {
        break;
      }
    }
    res.push_back(statements[i]);
    j++;
    i++;
  }
  if (func->arg_size() == statements.size()) {
    return true;
  }
  return false;
}

std::map<std::vector<std::string>, std::vector<Statement::Ptr>>
unifyByOffset(const std::vector<Statement::Ptr> &statements) {
  std::map<std::vector<std::string>, std::vector<Statement::Ptr>> res;
  for (const auto &i : statements) {
    res[i->offset].push_back(i);
  }
  return res;
}

void MockBuilder::buildAnnotationTaintOutput(llvm::Value *elem,
                                             const Statement::Ptr &statement) {
  auto taintOutputPtr = (Statement::TaintOutput *)statement.get();
  const auto source = annotationsData.taintAnnotation.sources.find(
      taintOutputPtr->getTaintType());
  if (source == annotationsData.taintAnnotation.sources.end()) {
    klee_warning("Annotation: unknown TaintOutput source %s",
                 taintOutputPtr->getTaintType().c_str());
    return;
  }
  buildCallKleeTaintFunction("klee_add_taint", elem, source->second, false);
}

void MockBuilder::buildAnnotationTaintPropagation(
    llvm::Value *elem, const Statement::Ptr &statement, llvm::Function *func,
    const std::string &target) {
  auto taintPropagationPtr = (Statement::TaintPropagation *)statement.get();
  const std::string sourceTypeLower =
      taintPropagationPtr->getTaintTypeAsLower();
  const auto source = annotationsData.taintAnnotation.sources.find(
      taintPropagationPtr->getTaintType());
  if (source == annotationsData.taintAnnotation.sources.end()) {
    klee_warning("Annotation: unknown TaintPropagation source %s",
                 taintPropagationPtr->getTaintType().c_str());
    return;
  }

  // TODO: support variable arg list
  if (taintPropagationPtr->propagationParameterIndex >= func->arg_size()) {
    klee_warning(
        "Annotation: ignore TaintPropagation because not support arg lists");
    return;
  }

  const std::string propagateCondName = "condition_taint_propagate_" +
                                        sourceTypeLower + target +
                                        func->getName().str();

  llvm::BasicBlock *fromIf = builder->GetInsertBlock();
  llvm::Function *curFunc = fromIf->getParent();

  llvm::BasicBlock *propagateBB = llvm::BasicBlock::Create(
      mockModule->getContext(), propagateCondName, curFunc);
  llvm::BasicBlock *contBB = llvm::BasicBlock::Create(
      mockModule->getContext(), "continue_" + propagateCondName);

  llvm::Value *propagationValue =
      func->getArg(taintPropagationPtr->propagationParameterIndex);
  auto brValuePropagate = buildCallKleeTaintFunction(
      "klee_check_taint_source", propagationValue, source->second, true);
  builder->CreateCondBr(brValuePropagate, propagateBB, contBB);

  builder->SetInsertPoint(propagateBB);
  buildCallKleeTaintFunction("klee_add_taint", elem, source->second, false);
  builder->CreateBr(contBB);

  curFunc->getBasicBlockList().push_back(contBB);
  builder->SetInsertPoint(contBB);
}

void MockBuilder::buildAnnotationTaintSink(llvm::Value *elem,
                                           const Statement::Ptr &statement,
                                           llvm::Function *func,
                                           const std::string &target) {
  auto taintSinkPtr = (Statement::TaintSink *)statement.get();
  const std::string sinkTypeLower = taintSinkPtr->getTaintTypeAsLower();
  const auto sink =
      annotationsData.taintAnnotation.sinks.find(taintSinkPtr->getTaintType());
  if (sink == annotationsData.taintAnnotation.sinks.end()) {
    klee_warning("Annotation: unknown TaintSink sink %s",
                 taintSinkPtr->getTaintType().c_str());
    return;
  }

  const std::string sinkCondName =
      "condition_taint_sink_" + sinkTypeLower + target + func->getName().str();

  llvm::BasicBlock *fromIf = builder->GetInsertBlock();
  llvm::Function *curFunc = fromIf->getParent();

  llvm::BasicBlock *sinkBB =
      llvm::BasicBlock::Create(mockModule->getContext(), sinkCondName, curFunc);
  llvm::BasicBlock *contBB = llvm::BasicBlock::Create(
      mockModule->getContext(), "continue_" + sinkCondName);
  auto brValueSink = buildCallKleeTaintFunction("klee_check_taint_sink", elem,
                                                sink->second, true);
  builder->CreateCondBr(brValueSink, sinkBB, contBB);

  builder->SetInsertPoint(sinkBB);
  std::string sinkHitCondName = "condition_taint_sink_hit_" + sinkTypeLower +
                                target + func->getName().str();

  auto intType = llvm::IntegerType::get(mockModule->getContext(), 1);
  auto *sinkHitCond = builder->CreateAlloca(intType, nullptr);
  buildCallKleeMakeSymbolic("klee_make_mock", sinkHitCond, intType,
                            sinkHitCondName);
  fromIf = builder->GetInsertBlock();
  curFunc = fromIf->getParent();
  llvm::BasicBlock *sinkHitBB = llvm::BasicBlock::Create(
      mockModule->getContext(), sinkHitCondName, curFunc);
  auto brValueSinkHit = builder->CreateLoad(intType, sinkHitCond);
  builder->CreateCondBr(brValueSinkHit, sinkHitBB, contBB);

  builder->SetInsertPoint(sinkHitBB);
  buildCallKleeTaintSinkHit(sink->second);
  builder->CreateBr(contBB);

  curFunc->getBasicBlockList().push_back(contBB);
  builder->SetInsertPoint(contBB);
}

llvm::CallInst *
MockBuilder::buildCallKleeTaintFunction(const std::string &functionName,
                                        llvm::Value *source, size_t taint,
                                        bool returnBool) {
  const auto returnType = returnBool
                              ? llvm::Type::getInt1Ty(mockModule->getContext())
                              : llvm::Type::getVoidTy(mockModule->getContext());

  auto *kleeTaintFunctionType = llvm::FunctionType::get(
      returnType,
      {llvm::Type::getInt8PtrTy(mockModule->getContext()),
       llvm::Type::getInt64Ty(mockModule->getContext())},
      false);
  auto kleeTaintFunctionCallee =
      mockModule->getOrInsertFunction(functionName, kleeTaintFunctionType);

  //  //TODO: that's not all:
  //  // - add var arg list (now it is not even considered
  //  //                 when passing through the function
  //  //                 parameters and never gets here)
  //  // - think about **
  //  // - add going by pointers
  //  llvm::Value *beginPtr;
  //  llvm::Value *countBytes;
  //  if (!source->getType()->isPointerTy() && !source->getType()->isArrayTy())
  //  {
  //    beginPtr = builder->CreateAlloca(source->getType());
  //    builder->CreateStore(source, beginPtr);
  //
  //    const size_t bytes = mockModule->getDataLayout().getTypeStoreSize(
  //      source->getType()->getPointerElementType());
  //    countBytes = llvm::ConstantInt::get(
  //      mockModule->getContext(),
  //      llvm::APInt(64, bytes, false));
  //  } else if (source->getType()->isPointerTy()) {
  //    beginPtr = builder->CreateBitCast(
  //      source, llvm::Type::getInt8PtrTy(mockModule->getContext()));
  //
  //    if (source->getType()->getPointerElementType()->isIntegerTy(8)) {
  //      auto *TLI = new
  //      llvm::TargetLibraryInfo(llvm::TargetLibraryInfoImpl()); countBytes =
  //      llvm::emitStrLen(source, *builder,
  //                                    mockModule->getDataLayout(),TLI);
  //    }
  //    else {
  //      const size_t bytes = mockModule->getDataLayout().getTypeStoreSize(
  //        source->getType()->getPointerElementType());
  //      countBytes = llvm::ConstantInt::get(
  //        mockModule->getContext(),
  //        llvm::APInt(64, bytes, false));
  //    }
  //  }

  llvm::Value *beginPtr;
  if (!source->getType()->isPointerTy() && !source->getType()->isArrayTy()) {
    beginPtr = builder->CreateAlloca(source->getType());
    builder->CreateStore(source, beginPtr);
    beginPtr = builder->CreateBitCast(
        beginPtr, llvm::Type::getInt8PtrTy(mockModule->getContext()));
  } else {
    beginPtr = builder->CreateBitCast(
        source, llvm::Type::getInt8PtrTy(mockModule->getContext()));
  }

  return builder->CreateCall(
      kleeTaintFunctionCallee,
      {beginPtr, llvm::ConstantInt::get(mockModule->getContext(),
                                        llvm::APInt(64, taint, false))});
}

void MockBuilder::buildCallKleeTaintSinkHit(size_t taintSink) {
  auto *kleeTaintSinkHitType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(mockModule->getContext()),
      {llvm::Type::getInt64Ty(mockModule->getContext())}, false);
  auto kleeTaintSinkHitCallee = mockModule->getOrInsertFunction(
      "klee_taint_sink_hit", kleeTaintSinkHitType);

  builder->CreateCall(
      kleeTaintSinkHitCallee,
      {llvm::ConstantInt::get(mockModule->getContext(),
                              llvm::APInt(64, taintSink, false))});
}

void MockBuilder::buildAnnotationForExternalFunctionArgs(
    llvm::Function *func,
    const std::vector<std::vector<Statement::Ptr>> &statementsNotAlign) {
  std::vector<std::vector<Statement::Ptr>> statements;
  bool flag = tryAlign(func, statementsNotAlign, statements);
  if (!flag) {
    klee_warning("Annotation: can't align function arguments %s",
                 func->getName().str().c_str());
    return;
  }
  for (size_t i = 0; i < statements.size(); i++) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(10, 0)
    const auto arg = func->getArg(i);
#else
    const auto arg = &func->arg_begin()[i];
#endif
    auto statementsMap = unifyByOffset(statements[i]);
    for (const auto &[offset, statementsOffset] : statementsMap) {
      auto [prev, elem] = goByOffset(arg, offset);

      Statement::Alloc *allocSourcePtr = nullptr;
      Statement::Free *freePtr = nullptr;
      Statement::InitNull *initNullPtr = nullptr;

      for (const auto &statement : statementsOffset) {
        switch (statement->getKind()) {
        case Statement::Kind::Deref: {
          if (!elem->getType()->isPointerTy()) {
            klee_error("Annotation: Deref arg not pointer");
          }

          std::string derefCondName = "condition_deref_arg_" +
                                      std::to_string(i) + "_deref_" +
                                      func->getName().str();

          auto intType = llvm::IntegerType::get(mockModule->getContext(), 1);
          auto *derefCond = builder->CreateAlloca(intType, nullptr);
          buildCallKleeMakeSymbolic("klee_make_mock", derefCond, intType,
                                    derefCondName);

          llvm::BasicBlock *fromIf = builder->GetInsertBlock();
          llvm::Function *curFunc = fromIf->getParent();

          llvm::BasicBlock *derefBB = llvm::BasicBlock::Create(
              mockModule->getContext(), derefCondName, curFunc);
          llvm::BasicBlock *contBB = llvm::BasicBlock::Create(
              mockModule->getContext(), "continue_" + derefCondName);
          auto brValue = builder->CreateLoad(intType, derefCond);
          builder->CreateCondBr(brValue, derefBB, contBB);

          builder->SetInsertPoint(derefBB);
          builder->CreateLoad(elem->getType()->getPointerElementType(), elem);
          builder->CreateBr(contBB);

          curFunc->getBasicBlockList().push_back(contBB);
          builder->SetInsertPoint(contBB);
          break;
        }
        case Statement::Kind::AllocSource: {
          if (prev != nullptr) {
            allocSourcePtr = (Statement::Alloc *)statement.get();
          } else {
            klee_message("Annotation: not valid annotation %s",
                         statement->toString().c_str());
          }
          break;
        }
        case Statement::Kind::InitNull: {
          if (prev != nullptr) {
            initNullPtr = (Statement::InitNull *)statement.get();
          } else {
            klee_message("Annotation: not valid annotation %s",
                         statement->toString().c_str());
          }
          break;
        }
        case Statement::Kind::Free: {
          if (elem->getType()->isPointerTy()) {
            freePtr = (Statement::Free *)statement.get();
          } else {
            klee_message("Annotation: not valid annotation %s",
                         statement->toString().c_str());
          }
          break;
        }
        case Statement::Kind::TaintOutput: {
          if (!elem->getType()->isPointerTy()) {
            klee_error("Annotation: TaintOutput arg is not pointer");
          }
          buildAnnotationTaintOutput(elem, statement);
          break;
        }
        case Statement::Kind::TaintPropagation: {
          if (!elem->getType()->isPointerTy()) {
            klee_error("Annotation: TaintPropagation arg is not pointer");
          }
          buildAnnotationTaintPropagation(elem, statement, func,
                                          "_arg_" + std::to_string(i) + "_");
          break;
        }
        case Statement::Kind::TaintSink: {
          buildAnnotationTaintSink(elem, statement, func,
                                   "_arg_" + std::to_string(i) + "_");
          break;
        }
        case Statement::Kind::Unknown:
        default:
          klee_message("Annotation: not implemented %s",
                       statement->toString().c_str());
          break;
        }
      }
      if (freePtr) {
        buildFree(elem, freePtr);
      }
      processingValue(prev, elem->getType(), allocSourcePtr, initNullPtr);
    }
  }
}

void MockBuilder::processingValue(llvm::Value *prev, llvm::Type *elemType,
                                  const Statement::Alloc *allocSourcePtr,
                                  bool initNullPtr) {
  if (initNullPtr) {
    auto intType = llvm::IntegerType::get(mockModule->getContext(), 1);
    auto *allocCond = builder->CreateAlloca(intType, nullptr);
    buildCallKleeMakeSymbolic("klee_make_mock", allocCond, intType,
                              "initPtrCond");

    llvm::BasicBlock *fromIf = builder->GetInsertBlock();
    llvm::Function *curFunc = fromIf->getParent();

    llvm::BasicBlock *initNullBB =
        llvm::BasicBlock::Create(mockModule->getContext(), "initNullBR");
    llvm::BasicBlock *contBB =
        llvm::BasicBlock::Create(mockModule->getContext(), "continueBR");
    auto brValue = builder->CreateLoad(intType, allocCond);
    if (allocSourcePtr) {
      llvm::BasicBlock *allocBB = llvm::BasicBlock::Create(
          mockModule->getContext(), "allocArg", curFunc);
      builder->CreateCondBr(brValue, allocBB, initNullBB);
      builder->SetInsertPoint(allocBB);
      buildAllocSource(prev, elemType, allocSourcePtr);
      builder->CreateBr(contBB);
    } else {
      builder->CreateCondBr(brValue, initNullBB, contBB);
    }
    curFunc->getBasicBlockList().push_back(initNullBB);
    builder->SetInsertPoint(initNullBB);
    builder->CreateStore(
        llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(elemType)),
        prev);
    builder->CreateBr(contBB);

    curFunc->getBasicBlockList().push_back(contBB);
    builder->SetInsertPoint(contBB);
  } else if (allocSourcePtr) {
    buildAllocSource(prev, elemType, allocSourcePtr);
  }
}

void MockBuilder::buildAllocSource(llvm::Value *prev, llvm::Type *elemType,
                                   const Statement::Alloc *allocSourcePtr) {
  if (allocSourcePtr->value != Statement::Alloc::ALLOC) {
    klee_warning("Annotation: AllocSource \"%d\" not implemented use alloc",
                 allocSourcePtr->value);
  }
  auto valueType = elemType->getPointerElementType();
  auto sizeValue = llvm::ConstantInt::get(
      mockModule->getContext(),
      llvm::APInt(64, mockModule->getDataLayout().getTypeStoreSize(valueType),
                  false));
  auto int8PtrTy = llvm::IntegerType::getInt64Ty(mockModule->getContext());
  auto mallocInstr =
      llvm::CallInst::CreateMalloc(builder->GetInsertBlock(), int8PtrTy,
                                   valueType, sizeValue, nullptr, nullptr);
  auto mallocValue = builder->Insert(mallocInstr, llvm::Twine("MallocValue"));
  builder->CreateStore(mallocValue, prev);
}

void MockBuilder::buildFree(llvm::Value *elem, const Statement::Free *freePtr) {
  if (freePtr->value != Statement::Free::FREE) {
    klee_warning("Annotation: AllocSource \"%d\" not implemented use free",
                 freePtr->value);
  }
  auto freeInstr = llvm::CallInst::CreateFree(elem, builder->GetInsertBlock());
  builder->Insert(freeInstr);
}

void MockBuilder::buildAnnotationForExternalFunctionReturn(
    llvm::Function *func, const std::vector<Statement::Ptr> &statements) {
  auto returnType = func->getReturnType();
  if (!returnType->isSized()) { // void return type
    builder->CreateRet(nullptr);
    return;
  }

  Statement::Alloc *allocSourcePtr = nullptr;
  Statement::InitNull *mustInitNull = nullptr;
  Statement::MaybeInitNull *maybeInitNull = nullptr;

  std::vector<Statement::Ptr> taintStatements;
  for (const auto &statement : statements) {
    switch (statement->getKind()) {
    case Statement::Kind::Deref:
      klee_warning("Annotation: unused Deref for return function \"%s\"",
                   func->getName().str().c_str());
      break;
    case Statement::Kind::AllocSource: {
      allocSourcePtr = returnType->isPointerTy()
                           ? (Statement::Alloc *)statement.get()
                           : nullptr;
      break;
    }
    case Statement::Kind::InitNull: {
      mustInitNull = returnType->isPointerTy()
                         ? (Statement::InitNull *)statement.get()
                         : nullptr;
      break;
    }
    case Statement::Kind::MaybeInitNull: {
      maybeInitNull = returnType->isPointerTy()
                          ? (Statement::MaybeInitNull *)statement.get()
                          : nullptr;
      break;
    }
    case Statement::Kind::Free: {
      klee_warning("Annotation: unused \"Free\" for return");
      break;
    }
    case Statement::Kind::TaintOutput:
    case Statement::Kind::TaintPropagation:
    case Statement::Kind::TaintSink: {
      taintStatements.push_back(statement);
      break;
    }
    case Statement::Kind::Unknown:
    default:
      klee_message("Annotation: not implemented %s",
                   statement->toString().c_str());
      break;
    }
  }
  std::string retName = "ret_" + func->getName().str();
  llvm::Value *retValuePtr = builder->CreateAlloca(returnType, nullptr);

  //  TODO: fix strange type ("fopen" mock, store instruction)
  //  if (func->getName() == "fopen") {
  //    buildCallKleeMakeSymbolic("klee_make_mock", retValuePtr, returnType,
  //                              func->getName().str());
  //    llvm::Value *retValue = builder->CreateLoad(returnType, retValuePtr,
  //    retName); builder->CreateRet(retValue); return;
  //  }

  if (returnType->isPointerTy() && (allocSourcePtr || mustInitNull)) {
    processingValue(retValuePtr, returnType, allocSourcePtr,
                    mustInitNull || maybeInitNull);
  } else {
    buildCallKleeMakeSymbolic("klee_make_mock", retValuePtr, returnType,
                              func->getName().str());
    if (returnType->isPointerTy() && !maybeInitNull) {
      llvm::Value *retValue =
          builder->CreateLoad(returnType, retValuePtr, retName);
      auto cmpResult =
          builder->CreateICmpNE(retValue,
                                llvm::ConstantPointerNull::get(
                                    llvm::cast<llvm::PointerType>(returnType)),
                                "condition_init_null_" + retName);

      auto *kleeAssumeType = llvm::FunctionType::get(
          llvm::Type::getVoidTy(mockModule->getContext()),
          {llvm::Type::getInt64Ty(mockModule->getContext())}, false);

      auto kleeAssumeFunc =
          mockModule->getOrInsertFunction("klee_assume", kleeAssumeType);
      auto cmpResult64 = builder->CreateZExt(
          cmpResult, llvm::Type::getInt64Ty(mockModule->getContext()));
      builder->CreateCall(kleeAssumeFunc, {cmpResult64});
    }
  }

  for (const auto &statement : taintStatements) {
    switch (statement->getKind()) {
    case Statement::Kind::TaintOutput: {
      buildAnnotationTaintOutput(retValuePtr, statement);
      break;
    }
    case Statement::Kind::TaintPropagation: {
      buildAnnotationTaintPropagation(retValuePtr, statement, func, "_ret_");
      break;
    }
    case Statement::Kind::TaintSink: {
      klee_warning("Annotation: unused TaintSink for return function \"%s\"",
                   func->getName().str().c_str());
      break;
    }
    default:
      __builtin_unreachable();
    }
  }

  llvm::Value *retValue = builder->CreateLoad(returnType, retValuePtr, retName);
  builder->CreateRet(retValue);
}

void MockBuilder::buildAnnotationForExternalFunctionProperties(
    llvm::Function *func, const std::set<Statement::Property> &properties) {
  for (const auto &property : properties) {
    switch (property) {
    case Statement::Property::Deterministic:
    case Statement::Property::Noreturn:
    case Statement::Property::Unknown:
    default:
      klee_message("Property not implemented");
      break;
    }
  }
}

} // namespace klee
