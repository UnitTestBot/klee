//===-- FreezeLower.cpp----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Passes.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include <llvm-16/llvm/IR/Value.h>

namespace klee {

using namespace llvm;

char FreezeLower::ID;

bool FreezeLower::runOnFunction(Function &F) {
  bool changed = false;
  std::vector<Instruction *> ToErase;

  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    for (BasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE;
         ++BI) {
      if (FreezeInst *inst = dyn_cast<FreezeInst>(&*BI)) {
        changed = true;
        Value *V = inst->getOperand(0);
        if (isa<UndefValue>(V))
          V = Constant::getNullValue(V->getType());
        inst->replaceAllUsesWith(V);
        inst->dropAllReferences();
        ToErase.push_back(inst);
      }
    }
  }
  for (Instruction *V : ToErase) {
    assert(V->user_empty());
    V->eraseFromParent();
  }

  return changed;
}
} // namespace klee
