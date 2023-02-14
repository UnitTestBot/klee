//===-- TargetedExecutionManager.cpp --------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetedExecutionManager.h"

#include "ExecutionState.h"
#include "klee/Core/TerminationTypes.h"
#include "klee/Module/KInstruction.h"
#include "klee/Support/ErrorHandling.h"

using namespace llvm;
using namespace klee;

class LocatedEventManager {
  using FilenameCache = std::unordered_map<std::string, bool>;
  std::unordered_map<std::string, FilenameCache *> filenameCacheMap;
  FilenameCache *filenameCache = nullptr;

public:
  LocatedEventManager() {}

  void prefetchFindFilename(const std::string &filename) {
    auto it = filenameCacheMap.find(filename);
    if (it != filenameCacheMap.end())
      filenameCache = it->second;
    else
      filenameCache = nullptr;
  }

  bool isInside(Location &loc, const klee::FunctionInfo &fi) {
    bool isInside = false;
    if (filenameCache == nullptr) {
      isInside = loc.isInside(fi);
      filenameCache = new FilenameCache();
      filenameCacheMap.insert(std::make_pair(fi.file, filenameCache));
      filenameCache->insert(std::make_pair(loc.filename, isInside));
    } else {
      auto it = filenameCache->find(loc.filename);
      if (it == filenameCache->end()) {
        isInside = loc.isInside(fi);
        filenameCache->insert(std::make_pair(loc.filename, isInside));
      } else {
        isInside = it->second;
      }
    }
    // llvm::errs() << loc.filename << ' ' << fi.file << ' ' << isInside << '\n';
    return isInside;
  }
};

struct TargetedExecutionManager::TargetPreparator {
  TargetedExecutionManager *tem;
  KModule *origModule;
  KModule *kmodule;
  std::vector<LocatedEvent *> filenameLineLocations;
  std::vector<LocatedEvent *> functionOffsetLocations;

TargetPreparator(TargetedExecutionManager *tem, KModule *origModule, KModule *kmodule, PathForest *pathForest)
  : tem(tem), origModule(origModule), kmodule(kmodule) {
  collectLocations(pathForest);
}

void collectLocations(PathForest *pathForest) {
  std::unordered_set<LocatedEvent *> result;
  std::vector<PathForest *> q{pathForest};
  while (!q.empty()) {
    auto forest = q.back();
    q.pop_back();
    for (const auto &p : forest->layer) {
      result.insert(p.first);
      if (p.second != nullptr)
        q.push_back(p.second);
    }
  }
  for (auto loc : result) {
    if (loc->hasFunctionWithOffset())
      functionOffsetLocations.push_back(loc);
    else
      filenameLineLocations.push_back(loc);
  }
}

ref<Target> addNewLocation(LocatedEvent *le, KBlock *block) const {
  auto error = le->getError();
  auto error2targetIt = tem->location2targets.find(le);
  TargetedExecutionManager::Error2Targets *error2target = nullptr;
  if (error2targetIt == tem->location2targets.end()) {
    error2target = new TargetedExecutionManager::Error2Targets();
    tem->location2targets.insert(error2targetIt, std::make_pair(le, error2target));
  } else {
    error2target = error2targetIt->second;
  }
  auto targetsIt = error2target->find(error);
  TargetedExecutionManager::Targets *targets = nullptr;
  if (targetsIt == error2target->end()) {
    targets = new TargetedExecutionManager::Targets();
    error2target->insert(targetsIt, std::make_pair(error, targets));
  } else {
    targets = targetsIt->second;
  }

  ref<Target> target;
  if (error != None) {
    target = Target::create(le, block);
  } else {
    target = Target::create(block);
  }

  targets->push_back(target);
  return target;
}

void prepareFilenameLineLocations(std::unordered_map<LocatedEvent *, TargetForest::TargetsSet *> &loc2targets) {
  auto infos = kmodule->infos.get();
  LocatedEventManager lem;
  for (const auto &kfunc : kmodule->functions) {
    const auto &fi = infos->getFunctionInfo(*kfunc->function);
    lem.prefetchFindFilename(fi.file);
    for (int i = filenameLineLocations.size() - 1; i >= 0; i--) {
      auto le = filenameLineLocations[i];
      TargetForest::TargetsSet *targetsForThisLoc = nullptr;
      auto targetsForThisLocIt = loc2targets.find(le);
      if (targetsForThisLocIt == loc2targets.end()) {
        targetsForThisLoc = new TargetForest::TargetsSet();
        loc2targets.insert(targetsForThisLocIt, std::make_pair(le, targetsForThisLoc));
      } else {
        targetsForThisLoc = targetsForThisLocIt->second;
      }
      auto loc = le->getLocation();
      if (!lem.isInside(*loc, fi))
        continue;
      bool addedNewTargets = false;
      for (const auto &kblock : kfunc->blocks) {
        auto b = kblock.get();
        if (!loc->isInside(b))
          continue;
        auto target = addNewLocation(le, b);
        targetsForThisLoc->insert(target);
        addedNewTargets = true;
      }
      if (!addedNewTargets)
        continue;
      filenameLineLocations[i] = filenameLineLocations.back();
      filenameLineLocations.pop_back();
    }
  }
}

void prepareFunctionOffsetLocations(
  std::unordered_map<LocatedEvent *, TargetForest::TargetsSet  *> &loc2targets,
  std::unordered_set<unsigned> &broken_traces)
{
  for (auto le : functionOffsetLocations) {
    auto loc = le->getLocation();
    KBlock *block = nullptr;
    auto it = tem->location2targets.find(le);
    if (it == tem->location2targets.end()) {
      std::ostringstream error;
      auto instrs = loc->initInstructions(origModule, kmodule, error);
      if (instrs.empty()) {
        klee_warning("Trace %u is malformed! %s at location %s, so skipping this trace.",
          le->getId(), error.str().c_str(), loc->toString().c_str());
        broken_traces.insert(le->getId());
        continue;
      }
      for (auto instr : instrs) {
        block = instr->parent;
        addNewLocation(le, block);
      }
    }
  }
  for (auto loc : functionOffsetLocations) {
    auto it = tem->location2targets.find(loc);
    if (it == tem->location2targets.end())
      continue;
    auto targets = new TargetForest::TargetsSet();
    for (auto &p : *it->second) {
      for (auto &t : *p.second) {
        if (t->shouldFailOnThisTarget() && t->getId() != loc->getId())
          continue;
        targets->insert(t);
      }
    }
    loc2targets.insert(std::make_pair(loc, targets));
  }
}

}; // struct TargetPreparator

void addBrokenTraces(PathForest *pathForest, std::unordered_set<unsigned> &all_broken_traces) {
  for (auto &p : pathForest->layer) {
    all_broken_traces.insert(p.first->getId());
    addBrokenTraces(p.second, all_broken_traces);
  }
}

void collectBrokenTraces(PathForest *pathForest,
  std::unordered_map<LocatedEvent *, TargetForest::TargetsSet *> &loc2Targets,
  std::unordered_set<unsigned> &all_broken_traces) {
  if (pathForest == nullptr)
    return;
  for (auto &p : pathForest->layer) {
    auto it = loc2Targets.find(p.first);
    if (it == loc2Targets.end()) {
      all_broken_traces.insert(p.first->getId());
      addBrokenTraces(p.second, all_broken_traces);
      continue;
    }
    collectBrokenTraces(p.second, loc2Targets, all_broken_traces);
  }
}

std::unordered_map<KFunction *, ref<TargetForest>>
TargetedExecutionManager::prepareTargets(KModule *origModule, KModule *kmodule, PathForest *pathForest) {
  pathForest->normalize();

  TargetPreparator all_locs(this, origModule, kmodule, pathForest);
  std::unordered_map<LocatedEvent *, TargetForest::TargetsSet *> loc2targets;
  all_locs.prepareFilenameLineLocations(loc2targets);
  all_locs.prepareFunctionOffsetLocations(loc2targets, broken_traces);

  std::unordered_set<unsigned> all_broken_traces;
  collectBrokenTraces(pathForest, loc2targets, all_broken_traces);
  for (auto id : all_broken_traces) {
    if (!broken_traces.count(id)) {
      broken_traces.insert(id);
      klee_warning("Trace %u is malformed! So skipping this trace.", id);
    }
  }

  std::unordered_map<KFunction *, ref<TargetForest>> whitelists;
  for (const auto &funcAndPaths : pathForest->layer) {
    auto le = funcAndPaths.first;
    if (loc2targets.count(le)) {
      auto kf = (*loc2targets.at(le)->begin())->getBlock()->parent;
      ref<TargetForest> whitelist = new TargetForest(funcAndPaths.second, loc2targets, broken_traces, kf);
      whitelists[kf] = whitelist;
    }
  }

  for (auto p : loc2targets)
    delete p.second;

  return whitelists;
}

void TargetedExecutionManager::reportFalseNegative(ExecutionState &state,
                                                   ReachWithError error) {
  klee_warning("100.00%% %s False Negative at: %s", ReachWithErrorNames[error],
               state.prevPC->getSourceLocation().c_str());
}

bool TargetedExecutionManager::reportTruePositive(ExecutionState &state, ReachWithError error) {
  bool atLeastOneReported = false;
  for (auto kvp : state.targetForest) {
    auto target = kvp.first;
    if (target->getError() != error || broken_traces.count(target->getId()))
      continue;
    auto expectedLocation = target->getLocation();

    /// The following code checks if target is a `call ...` instruction and we failed somewhere *inside* call
    auto possibleInstruction = state.prevPC;
    int i = state.stack.size() - 1;
    bool found = true;

    while (!expectedLocation->isTheSameAsIn(possibleInstruction)) { //TODO: target->getBlock() == possibleInstruction should also be checked, but more smartly
      if (i <= 0) {
        found = false;
        break;
      }
      possibleInstruction = state.stack[i].caller;
      i--;
    }
    if (!found)
      continue;

    state.error = error;
    atLeastOneReported = true;
    assert(!target->isReported);
    if (target->getError() == ReachWithError::Reachable) {
      klee_warning("100.00%% %s Reachable at trace %u", target->getErrorString(), target->getId());
    } else {
      klee_warning("100.00%% %s True Positive at trace %u", target->getErrorString(), target->getId());
    }
    target->isReported = true;
  }
  return atLeastOneReported;
}

TargetedExecutionManager::~TargetedExecutionManager() {
  for (auto p : location2targets)
    delete p.second;
}
