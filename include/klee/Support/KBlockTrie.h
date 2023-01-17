//===-- KBlockTrie.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KBLOCK_TRIE_H
#define KLEE_KBLOCK_TRIE_H

#include "klee/Module/InstructionInfoTable.h"
#include "klee/Support/SarifReport.h"
#include "klee/Module/KModule.h"

#include <vector>
#include <unordered_map>
#include <string>

namespace klee {
    using InstructionsMap = std::unordered_map<std::string, std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>>>;

    void infoTableToInstructionsMap(const InstructionInfoTable& infoTable, InstructionsMap& result);
    
    std::vector<const llvm::Instruction*> getInstructions(const PhysicalLocation& location, const InstructionsMap& instructionMap);
}

#endif /* KLEE_KBLOCK_TRIE_H */
