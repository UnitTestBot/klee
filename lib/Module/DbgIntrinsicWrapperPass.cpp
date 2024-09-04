//===-- LocalVarDeclarationFinderPass.cpp -----------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "Passes.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>

#include <string>
#include <unordered_map>

using namespace klee;

char DbgIntrinsicWrapperPass::ID = 0;

std::string DbgIntrinsicWrapperPass::generateNewWrapperName() {
  return "_dbg_intrinsic_wrapper_#" + std::to_string(wrappedCounter++);
}

llvm::Function &
DbgIntrinsicWrapperPass::wrapInFunction(llvm::DbgInfoIntrinsic &intrinsic) {
  std::string wrapperName = generateNewWrapperName();

  auto &ctx = intrinsic.getContext();
  auto &module = *intrinsic.getModule();

  /* Generate function */
  auto wrapperFunctionReturnType =
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {}, false);
  auto wrapperFunction = llvm::Function::Create(wrapperFunctionReturnType,
                                                llvm::Function::InternalLinkage,
                                                wrapperName, &module);

  wrapperFunction->addFnAttr(llvm::Attribute::OptimizeNone);
  wrapperFunction->addFnAttr(llvm::Attribute::NoInline);

  /* Generate function body */
  auto wrapperBlock = llvm::BasicBlock::Create(ctx, "", wrapperFunction);

  llvm::IRBuilder<> builder(ctx);
  builder.SetInsertPoint(wrapperBlock);

  auto operands = intrinsic.getOperandList();
  auto functionCallee = module.getOrInsertFunction(
      intrinsic.getCalledFunction()->getName(),
      intrinsic.getCalledFunction()->getFunctionType());

  auto callToDbgInst = builder.CreateCall(functionCallee, operands->get());
  callToDbgInst->setDebugLoc(intrinsic.getDebugLoc());

  builder.CreateRetVoid();

  assert(!llvm::verifyFunction(*wrapperFunction));
  return *wrapperFunction;
}

bool DbgIntrinsicWrapperPass::runOnBasicBlock(llvm::BasicBlock &block) {
  bool isModified = false;

  auto &module = *block.getModule();
  auto &ctx = block.getContext();

  std::unordered_map<llvm::DbgLabelInst *, llvm::Function *> wrappers;

  for (auto &instruction : block) {
    if (auto intrinsic =
            llvm::dyn_cast_or_null<llvm::DbgLabelInst>(&instruction)) {

      isModified = true;
      auto &function = wrapInFunction(*intrinsic);
      wrappers.emplace(intrinsic, &function);
      addedWrappers.emplace(&function);
    }
  }

  llvm::IRBuilder<> builder(ctx);
  for (auto &[intrinsic, function] : wrappers) {
    auto iteratorAfterIntrinsic = intrinsic->eraseFromParent();

    auto functionCallee = module.getOrInsertFunction(function->getName(),
                                                     function->getReturnType());

    auto wrapperCall = builder.CreateCall(functionCallee);
    wrapperCall->insertBefore(&*iteratorAfterIntrinsic);
  }

  return isModified;
}

bool DbgIntrinsicWrapperPass::runOnModule(llvm::Module &module) {
  bool isModified = false;

  for (auto &function : module) {
    if (addedWrappers.count(&function) != 0) {
      continue;
    }

    for (auto &block : function) {
      isModified |= runOnBasicBlock(block);
    }
  }

  return isModified;
}
