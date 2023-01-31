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

        if (!location.artifactLocation) {
            return result;
        }

        const optional<std::string>& filename = location.artifactLocation->uri;

        if (!filename) {
            return result;
        }

        std::unordered_map<unsigned, std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>>& instructionsInFile = instructionMap[*filename];
        if (!location.region) {
            return result;
        }

        const Region& region = *location.region;

        if (!region.startLine) {
            return result;
        }

        if (region.endLine && *region.endLine != *region.startLine) {
            return result;
        }

        std::unordered_map<unsigned, std::vector<const llvm::Instruction*>>& instructionsInLine = instructionsInFile[*region.startLine];

        if (!region.startColumn || !region.endColumn) {
            for (const auto& p : instructionsInLine) {
                result.insert(result.end(), p.second.begin(), p.second.end());
            }
        } else {
            for (const auto& p : instructionsInLine) {
                if (p.first >= *region.startColumn && p.first <= *region.endColumn) {
                    result.insert(result.end(), p.second.begin(), p.second.end());
                }
            }
        }

        return result;
    }

    SourceCodeToByteCode::SourceCodeToByteCode(const std::vector<const llvm::Instruction*>& instructions) {
        for (const auto p : instructions) {
            blocks.insert(p->getParent());
        }

        if (instructions.size() == 1) {
            instruction = optional<const llvm::Instruction*>(instructions.back());
        }
    }

    void Trie::addCodeFlow(std::vector<SourceCodeToByteCode>& codeFlow, SarifError error) {
        root->addCodeFlow(codeFlow, error);
    }

    void TrieNode::addCodeFlow(std::vector<SourceCodeToByteCode>& codeFlow, SarifError error) {
        if (codeFlow.size() == 0) {
            errors[error] = false;
            return;
        }

        std::unordered_set<const llvm::BasicBlock*> blocks = std::move(codeFlow.back().blocks);
        codeFlow.pop_back();

        std::unordered_set<const llvm::BasicBlock*> newBlocks;

        for (const auto step : blocks) {
            bool newBlockAdded = addCodeFlow(step, codeFlow, error);
            if (newBlockAdded) {
                newBlocks.insert(step);
            }
        }

        if (newBlocks.size() > 0) {
            for (const auto block : newBlocks) {
                colorsDist[current_color].insert(block);
                colors[block] = current_color;
            }
            ++current_color;
        }

        std::unordered_map<size_t, std::unordered_set<const llvm::BasicBlock*>> dist;
        for (const auto block : blocks) {
            dist[colors[block]].insert(block);
        }

        for (const auto& p : dist) {
            if (p.second.size() != colorsDist[p.first].size()) {
                for (const auto block : p.second) {
                    colorsDist[p.first].erase(block);
                    colorsDist[current_color].insert(block);
                    colors[block] = current_color;
                }
                ++current_color;
            }
        }
    }

    bool TrieNode::addCodeFlow(const llvm::BasicBlock* step, std::vector<SourceCodeToByteCode>& restCodeFlow, SarifError error) {
        bool newBlockAdded = false;
        if (successors.count(step) == 0) {
            successors[step] = std::make_unique<TrieNode>();
            newBlockAdded = true;
        }

        successors[step]->addCodeFlow(restCodeFlow, error);
        return newBlockAdded;
    }

    void Trie::mergeNodes() {
        root->mergeNodes();
    }

    void TrieNode::mergeNodes() {
        for (const auto& p : colorsDist) {
            if (p.second.size() > 1) {
                std::shared_ptr<TrieNode> node = successors[*p.second.begin()];
                for (const auto block : p.second) {
                    successors[block] = node;
                }
                node->mergeNodes();
            }
        }
    }

} // namespace klee
