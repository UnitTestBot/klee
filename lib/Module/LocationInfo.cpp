//===-- LocationInfo.cpp ------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Module/LocationInfo.h"
#include "klee/Module/SarifReport.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include <llvm/IR/GlobalVariable.h>

#include <optional>

namespace klee {

PhysicalLocationJson LocationInfo::serialize() const {
  // clang-format off
  return PhysicalLocationJson{
    {
      ArtifactLocationJson {
        {file}
      }
    },
    {
      RegionJson {
        {line}, 
        std::nullopt, 
        column, 
        std::nullopt
      }
    }
  };
  // clang-format on
}

////////////////////////////////////////////////////////////////

LocationInfo::LocationInfoCacheSet LocationInfo::cachedLocationInfo;

ref<LocationInfo>
LocationInfo::createCachedLocationInfo(ref<LocationInfo> locationInfo) {
  std::pair<CacheType::const_iterator, bool> success =
      cachedLocationInfo.cache.insert(locationInfo.get());

  if (success.second) {
    // Cache miss
    locationInfo->isCached = true;
    return locationInfo;
  }
  // Cache hit
  return (ref<LocationInfo>)*(success.first);
}

ref<LocationInfo> getLocationInfo(const llvm::Function *func) {
  const auto dsub = func->getSubprogram();

  if (dsub != nullptr) {
    auto path = dsub->getFilename();
    return LocationInfo::create(path.str(), dsub->getLine(), {});
  }

  return LocationInfo::create("", 0, {});
}

ref<LocationInfo> getLocationInfo(const llvm::Instruction *inst) {
  // Retrieve debug information associated with instruction
  const auto &dl = inst->getDebugLoc();

  // Check if a valid debug location is assigned to the instruction.
  if (dl.get() != nullptr) {
    auto full_path = dl->getFilename();
    auto line = dl.getLine();
    auto column = dl.getCol();

    // Still, if the line is unknown, take the context of the instruction to
    // narrow it down
    if (line == 0) {
      if (auto LexicalBlock =
              llvm::dyn_cast<llvm::DILexicalBlock>(dl.getScope())) {
        line = LexicalBlock->getLine();
        column = LexicalBlock->getColumn();
      }
    }
    return LocationInfo::create(full_path.str(), line, {column});
  }

  return getLocationInfo(inst->getParent()->getParent());
}

ref<LocationInfo> getLocationInfo(const llvm::GlobalVariable *globalVar) {
  // Retrieve debug information associated with global variable.
  // LLVM does not expose API for getting single DINode with location
  // information.
  llvm::SmallVector<llvm::DIGlobalVariableExpression *, 16> debugInfo;
  globalVar->getDebugInfo(debugInfo);

  for (const llvm::DIGlobalVariableExpression *debugInfoEntry : debugInfo) {
    // Return location from any debug info for global variable.
    if (const llvm::DIGlobalVariable *debugInfoGlobalVar =
            debugInfoEntry->getVariable()) {
      return LocationInfo::create(debugInfoGlobalVar->getFilename().str(),
                                  debugInfoGlobalVar->getLine(), {});
    }
  }

  // For `extern` variables return `external` file.
  if (globalVar->hasExternalLinkage()) {
    return LocationInfo::create("external", 0, {});
  }

  // Fallback to empty location if there is no appropriate debug
  // info.
  return LocationInfo::create("", 0, {});
}

} // namespace klee
