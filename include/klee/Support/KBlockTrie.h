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
#include <memory>

namespace klee {
    enum SarifError {
    };

    using InstructionsMap = std::unordered_map<std::string, std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>>>;

    void infoTableToInstructionsMap(const InstructionInfoTable& infoTable, InstructionsMap& result);
    
    std::vector<const llvm::Instruction*> getInstructions(const PhysicalLocation& location, const InstructionsMap& instructionMap);

    struct TrieNode {
        std::unordered_map<const llvm::Instruction*, std::unique_ptr<TrieNode>> successors;
        std::unordered_map<SarifError, bool> errors;

        bool isLeaf() {
            return errors.empty();
        }

        void addCodeFlow(std::vector<std::vector<const llvm::Instruction*>>& codeFlow, SarifError error);
    private:
        void addCodeFlow(const llvm::Instruction* step, std::vector<std::vector<const llvm::Instruction*>>& restCodeFlow, SarifError error);
    };

    class Trie {
    public:
        const std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();

        void addCodeFlow(std::vector<std::vector<const llvm::Instruction*>>& codeFlow, SarifError error);
    };
}

#endif /* KLEE_KBLOCK_TRIE_H */
