//===-- ModuleHelper.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MODULEHELPER_H
#define KLEE_MODULEHELPER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Module.h"

namespace klee {
enum class SwitchImplType {
  eSwitchTypeSimple,
  eSwitchTypeLLVM,
  eSwitchTypeInternal
};

void optimiseAndPrepare(bool OptimiseKLEECall, bool Optimize,
                        bool WithFPRuntime, SwitchImplType SwitchType,
                        std::string EntryPoint,
                        llvm::ArrayRef<const char *> preservedFunctions,
                        llvm::Module *module);
void checkModule(bool DontVerfify, llvm::Module *module);
void instrument(bool CheckDivZero, bool CheckOvershift, bool WithFPRuntime,
                llvm::Module *module);

void injectStaticConstructorsAndDestructors(llvm::Module *m,
                                            llvm::StringRef entryFunction);

void optimizeModule(llvm::Module *M,
                    llvm::ArrayRef<const char *> preservedFunctions);

// Mark function with functionName as part of the KLEE runtime
void addInternalFunction(const char *functionName);
// Replace std functions with KLEE intrinsics
void replaceFunction(const std::unique_ptr<llvm::Module> &m,
                     const char *original, const char *replacement);
} // namespace klee

#endif // KLEE_MODULEHELPER_H
