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

#include "llvm/IR/Instruction.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

namespace klee {
    enum SarifError {
    };

    using InstructionsMap = std::unordered_map<std::string, std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>>>;

    void infoTableToInstructionsMap(const InstructionInfoTable& infoTable, InstructionsMap& result);
    
    std::vector<const llvm::Instruction*> getInstructions(const PhysicalLocation& location, const InstructionsMap& instructionMap);

    struct SourceCodeToByteCode {
        std::unordered_set<const llvm::BasicBlock*> blocks;
        optional<const llvm::Instruction*> instruction;

        SourceCodeToByteCode(const std::vector<const llvm::Instruction*>& instructions);
    };

    struct TrieNode {
        std::unordered_map<const llvm::BasicBlock*, std::shared_ptr<TrieNode>> successors;
        std::unordered_map<SarifError, bool> errors;
        std::unordered_map<size_t, std::unordered_set<const llvm::BasicBlock*>> colorsDist;
        std::unordered_map<const llvm::BasicBlock*, size_t> colors;
        size_t current_color = 0;

        bool isLeaf() {
            return errors.empty();
        }

        void addCodeFlow(std::vector<SourceCodeToByteCode>& codeFlow, SarifError error);
        void mergeNodes();
    private:
        bool addCodeFlow(const llvm::BasicBlock* step, std::vector<SourceCodeToByteCode>& restCodeFlow, SarifError error);
    };

    class Trie {
    public:
        const std::unique_ptr<TrieNode> root = std::make_unique<TrieNode>();

        void addCodeFlow(std::vector<SourceCodeToByteCode>& codeFlow, SarifError error);
        void mergeNodes();
    };
}

#endif /* KLEE_KBLOCK_TRIE_H */
