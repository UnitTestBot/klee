//===-- KBlockTree.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/KBlockTrie.h"

namespace klee {

    void infoTableToInstructionsMap(const InstructionInfoTable& infoTable, InstructionsMap& result) {
        result.clear();

        for (const auto& p : infoTable.getInfos()) {
            const InstructionInfo& info = infoTable.getInfo(*p.first);
            result[info.file][info.line][info.column].push_back(p.first);
        }
    }

    std::vector<const llvm::Instruction*> getInstructions(const PhysicalLocation& location, InstructionsMap& instructionMap) {
        std::vector<const llvm::Instruction*> result;

        if (location.artifactLocation.is_empty) {
            return result;
        }

        const Option<std::string>& filename = location.artifactLocation.value.uri;

        if (filename.is_empty) {
            return result;
        }

        std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>>& instructionsInFile = instructionMap[filename.value];
        if (location.region.is_empty) {
            return result;
        }

        const Region& region = location.region.value;

        if (region.startLine.is_empty) {
            return result;
        }

        if (!region.endLine.is_empty && region.endLine.value != region.startLine.value) {
            return result;
        }

        std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>& instructionsInLine = instructionsInFile[region.startLine.value];

        if (region.startColumn.is_empty || region.endColumn.is_empty) {
            for (const auto& p : instructionsInLine) {
                result.insert(result.end(), p.second.begin(), p.second.end());
            }
        } else {
            for (const auto& p : instructionsInLine) {
                if (p.first >= region.startColumn.value && p.first <= region.endColumn.value) {
                    result.insert(result.end(), p.second.begin(), p.second.end());
                }
            }
        }

        return result;
    }

    void Trie::addCodeFlow(std::vector<std::vector<const llvm::Instruction*>>& codeFlow, SarifError error) {
        root->addCodeFlow(codeFlow, error);
    }

    void TrieNode::addCodeFlow(std::vector<std::vector<const llvm::Instruction*>>& codeFlow, SarifError error) {
        if (codeFlow.size() == 0) {
            errors[error] = false;
            return;
        }

        std::vector<const llvm::Instruction*> insts = std::move(codeFlow.back());
        codeFlow.pop_back();

        for (const llvm::Instruction* step : insts) {
            addCodeFlow(step, codeFlow, error);
        }
    }

    void TrieNode::addCodeFlow(const llvm::Instruction* step, std::vector<std::vector<const llvm::Instruction*>>& restCodeFlow, SarifError error) {
        if (successors.count(step) == 0) {
            successors[step] = std::make_unique<TrieNode>();
        }

        successors[step]->addCodeFlow(restCodeFlow, error);
    }

} // namespace klee
