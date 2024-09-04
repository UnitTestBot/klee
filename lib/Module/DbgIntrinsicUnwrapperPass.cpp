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
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <unordered_map>

using namespace klee;

char DbgIntrinsicUnwrapperPass::ID = 0;

std::optional<std::pair<llvm::Instruction *, llvm::Instruction *>>
DbgIntrinsicUnwrapperPass::unwrapCall(llvm::CallInst &callInst) {
  auto &ctx = callInst.getContext();
  auto &module = *callInst.getModule();

  auto calledFunction = callInst.getCalledFunction();
  auto dbgLabelOptional = findDbgLabel(*calledFunction);
  if (!dbgLabelOptional) {
    return std::nullopt;
  }

  auto &intrinsic = **dbgLabelOptional;

  auto operands = intrinsic.getOperandList();
  auto functionCallee = module.getOrInsertFunction(
      intrinsic.getCalledFunction()->getName(),
      intrinsic.getCalledFunction()->getFunctionType());

  llvm::IRBuilder<> builder(ctx);
  auto callToDbgInst = builder.CreateCall(functionCallee, operands->get());
  callToDbgInst->setDebugLoc(intrinsic.getDebugLoc());

  return {{&callInst, callToDbgInst}};
}

void DbgIntrinsicUnwrapperPass::runOnBasicBlock(llvm::BasicBlock &block) {
  std::unordered_map<llvm::Instruction *, llvm::Instruction *> replaces;
  for (auto &instruction : block) {
    if (auto *callInst = llvm::dyn_cast<llvm::CallInst>(&instruction)) {
      if (wrappers.count(callInst->getCalledFunction()) == 0) {
        continue;
      }

      if (auto replace = unwrapCall(*callInst)) {
        replaces.insert(*replace);
      }
    }
  }

  for (auto [callInst, intrinsic] : replaces) {
    auto nextInstruction = callInst->eraseFromParent();
    intrinsic->insertBefore(&*nextInstruction);
  }
}

std::optional<llvm::DbgLabelInst *>
DbgIntrinsicUnwrapperPass::findDbgLabel(llvm::Function &function) {
  for (auto &block : function) {
    for (auto &instruction : block) {
      if (auto *dbgLabelInstr =
              llvm::dyn_cast<llvm::DbgLabelInst>(&instruction)) {
        return dbgLabelInstr;
      }
    }
  }
  return std::nullopt;
}

bool DbgIntrinsicUnwrapperPass::runOnModule(llvm::Module &module) {
  if (wrappers.empty()) {
    return false;
  }

  for (auto &function : module) {
    for (auto &block : function) {
      runOnBasicBlock(block);
    }
  }

  // llvm::CallGraph callGraph(module);
  // for (auto wrapper : wrappers) {
  //   // wrapper->deleteBody();
  //   callGraph.removeFunctionFromModule(callGraph.getOrInsertFunction(wrapper));
  //   //
  //   wrapper->replaceAllUsesWith(llvm::UndefValue::get(wrapper->getType()));
  //   wrapper->removeFromParent();
  // }

  return true;
}
