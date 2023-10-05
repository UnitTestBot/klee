//===-- SarifReport.cpp----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <list>

#include "klee/Module/SarifReport.h"

#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/ErrorHandling.h"
#include "llvm/Support/CommandLine.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/IntrinsicInst.h"
DISABLE_WARNING_POP

using namespace llvm;
using namespace klee;

namespace {
bool isOSSeparator(char c) { return c == '/' || c == '\\'; }

ReachWithErrors tryConvertRuleJson(const std::string &ruleId, ToolName toolName,
                                   const optional<Message> &errorMessage) {
  switch (toolName) {
  case ToolName::SecB:
    if ("NullDereference" == ruleId) {
      return {ReachWithError::MustBeNullPointerException};
    } else if ("CheckAfterDeref" == ruleId) {
      return {ReachWithError::NullCheckAfterDerefException};
    } else if ("DoubleFree" == ruleId) {
      return {ReachWithError::DoubleFree};
    } else if ("UseAfterFree" == ruleId) {
      return {ReachWithError::UseAfterFree};
    } else if ("Reached" == ruleId) {
      return {ReachWithError::Reachable};
    }
    return {};
  case ToolName::clang:
    if ("core.NullDereference" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException};
    } else if ("unix.Malloc" == ruleId) {
      if (errorMessage.has_value()) {
        if (errorMessage->text == "Attempt to free released memory") {
          return {ReachWithError::DoubleFree};
        } else if (errorMessage->text == "Use of memory after it is freed") {
          return {ReachWithError::UseAfterFree};
        } else {
          return {};
        }
      } else {
        return {ReachWithError::UseAfterFree, ReachWithError::DoubleFree};
      }
    } else if ("core.Reach" == ruleId) {
      return {ReachWithError::Reachable};
    }
    return {};
  case ToolName::CppCheck:
    if ("nullPointer" == ruleId || "ctunullpointer" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException}; // TODO: check it out
    } else if ("doubleFree" == ruleId) {
      return {ReachWithError::DoubleFree};
    }
    return {};
  case ToolName::Infer:
    if ("NULL_DEREFERENCE" == ruleId || "NULLPTR_DEREFERENCE" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException}; // TODO: check it out
    } else if ("USE_AFTER_DELETE" == ruleId || "USE_AFTER_FREE" == ruleId) {
      return {ReachWithError::UseAfterFree, ReachWithError::DoubleFree};
    }
    return {};
  case ToolName::Cooddy:
    if ("NULL.DEREF" == ruleId || "NULL.UNTRUSTED.DEREF" == ruleId) {
      return {ReachWithError::MayBeNullPointerException,
              ReachWithError::MustBeNullPointerException};
    } else if ("MEM.DOUBLE.FREE" == ruleId) {
      return {ReachWithError::DoubleFree};
    } else if ("MEM.USE.FREE" == ruleId) {
      return {ReachWithError::UseAfterFree};
    }
    return {};
  case ToolName::Unknown:
    return {};
  }
}

bool startsWith(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

bool endsWith(const std::string &s, const std::string &suffix) {
  ssize_t maybe_index = s.size() - suffix.size();
  return maybe_index > 0 &&
         (s.find(suffix, maybe_index) == (size_t)maybe_index);
}

ReachWithoutError tryConvertCooddyNoErrorKind(const klee::Message &msg) {
  const auto &message = msg.text;
  if ("Assume return here" == message)
    return ReachWithoutError::Return;
  if (startsWith(message, "Null pointer returned as"))
    return ReachWithoutError::AfterCall;
  if (startsWith(message, "Use after free returned as"))
    return ReachWithoutError::AfterCall;
  if ("Null pointer source" == message)
    return ReachWithoutError::NPESource;
  if ("Free" == message)
    return ReachWithoutError::Free;
  if (startsWith(message, "Function ") && endsWith(message, " is executed"))
    return ReachWithoutError::Call;
  if (startsWith(message, "Null pointer passed as"))
    return ReachWithoutError::Call;
  if (startsWith(message, "Use after free passed as"))
    return ReachWithoutError::Call;
  if (endsWith(message, " is false"))
    return ReachWithoutError::BranchFalse;
  if (endsWith(message, " is true"))
    return ReachWithoutError::BranchTrue;
  return ReachWithoutError::Reach;
}

ReachWithoutError tryConvertNoErrorKind(ToolName toolName,
                                        const klee::Message &message) {
  switch (toolName) {
  case ToolName::Cooddy:
    return tryConvertCooddyNoErrorKind(message);
  default:
    return ReachWithoutError::Reach;
  }
}

ReachWithErrors tryConvertErrorKind(ToolName toolName,
                                    const optional<std::string> &ruleId,
                                    const optional<Message> &msg) {
  if (!ruleId.has_value()) {
    return {ReachWithError::Reachable};
  } else {
    return tryConvertRuleJson(*ruleId, toolName, msg);
  }
}

EventKind convertKindJson(ToolName toolName,
                          const optional<std::string> &ruleId,
                          const std::optional<klee::Message> &mOpt) {
  if (!mOpt.has_value())
    return EventKind(ReachWithoutError::Reach);
  const auto &message = *mOpt;
  auto noError = tryConvertNoErrorKind(toolName, message);
  if (noError != ReachWithoutError::Reach)
    return EventKind(noError);
  return EventKind(tryConvertErrorKind(toolName, ruleId, mOpt));
}

optional<std::pair<ref<Location>, EventKind>>
tryConvertLocationJson(ToolName toolName, const optional<std::string> &ruleId,
                       const LocationJson &locationJson) {
  const auto &physicalLocation = locationJson.physicalLocation;
  if (!physicalLocation.has_value()) {
    return nonstd::nullopt;
  }

  const auto &artifactLocation = physicalLocation->artifactLocation;
  if (!artifactLocation.has_value() || !artifactLocation->uri.has_value()) {
    return nonstd::nullopt;
  }

  const auto &region = physicalLocation->region;
  if (!region.has_value() || !region->startLine.has_value()) {
    return nonstd::nullopt;
  }

  auto kind = convertKindJson(toolName, ruleId, region->message);
  auto filename = *(artifactLocation->uri);

  auto loc = Location::create(std::move(filename), *(region->startLine),
                              region->endLine, region->startColumn,
                              region->endColumn, toolName, kind);
  return std::make_pair(loc, std::move(kind));
}

void recoverCallChain(std::vector<ref<Location>> &locations,
                      std::vector<EventKind> &kinds) {
  std::list<ref<Location>> result;
  auto lit = locations.begin();
  auto kit = kinds.begin(), kite = kinds.end();
  for (; kit != kite; kit++, lit++) {
    if (kit->kind == ReachWithoutError::AfterCall)
      result.push_front(*lit);
    else
      result.push_back(*lit);
  }
  locations.assign(result.begin(), result.end());
}

optional<Result> tryConvertResultJson(const ResultJson &resultJson,
                                      ToolName toolName,
                                      const std::string &id) {
  auto errors =
      tryConvertErrorKind(toolName, resultJson.ruleId, resultJson.message);
  if (errors.size() == 0) {
    klee_warning("undefined error in %s result", id.c_str());
    return nonstd::nullopt;
  }

  std::vector<ref<Location>> locations;
  std::vector<EventKind> kinds;
  std::vector<optional<json>> metadatas;

  if (resultJson.codeFlows.size() > 0) {
    assert(resultJson.codeFlows.size() == 1);
    assert(resultJson.codeFlows[0].threadFlows.size() == 1);

    const auto &threadFlow = resultJson.codeFlows[0].threadFlows[0];
    for (auto &threadFlowLocation : threadFlow.locations) {
      if (!threadFlowLocation.location.has_value()) {
        return nonstd::nullopt;
      }

      auto mblk = tryConvertLocationJson(toolName, resultJson.ruleId,
                                         *threadFlowLocation.location);
      if (mblk.has_value()) {
        locations.push_back(mblk->first);
        kinds.push_back(mblk->second);
        metadatas.push_back(std::move(threadFlowLocation.metadata));
      }
    }
  } else {
    assert(resultJson.locations.size() == 1);
    auto mblk = tryConvertLocationJson(toolName, resultJson.ruleId,
                                       resultJson.locations[0]);
    if (mblk.has_value()) {
      locations.push_back(mblk->first);
      kinds.push_back(mblk->second);
    }
  }

  if (locations.empty()) {
    return nonstd::nullopt;
  }

  recoverCallChain(locations, kinds);

  return Result{std::move(locations), std::move(metadatas), id,
                std::move(errors)};
}
} // anonymous namespace

namespace klee {
llvm::cl::opt<bool> LocationAccuracy(
    "location-accuracy", cl::init(false),
    cl::desc("Check location with line and column accuracy (default=false)"));

static const char *ReachWithErrorNames[] = {
    "DoubleFree",
    "UseAfterFree",
    "MayBeNullPointerException",
    "NullPointerException", // for backward compatibility with SecB
    "NullCheckAfterDerefException",
    "Reachable",
    "None",
};

void LineColumnRange::setRange(const KInstruction *ki) {
  startLine = (endLine = ki->getLine());
  startColumn = (endColumn = ki->getColumn());
}

bool LineColumnRange::hasInsideInternal(InstrWithPrecision &kp) const {
  auto line = kp.ptr->getLine();
  if (!(startLine <= line && line <= endLine)) {
    kp.setNotFound();
    return false;
  }
  if (onlyLine()) {
    kp.precision = Precision::Line;
    return false;
  }
  auto column = kp.ptr->getColumn();
  auto ok = true;
  if (line == startLine)
    ok = column >= startColumn;
  if (line == endLine)
    ok = ok && column <= endColumn;
  if (ok)
    kp.precision = Precision::Column;
  else
    kp.precision = Precision::Line;
  return false;
}

const char *getErrorString(ReachWithError error) {
  return ReachWithErrorNames[error];
}

std::string getErrorsString(const ReachWithErrors &errors) {
  if (errors.size() == 1) {
    return getErrorString(*errors.begin());
  }

  std::string res = "(";
  size_t index = 0;
  for (auto err : errors) {
    res += getErrorString(err);
    if (index != errors.size() - 1) {
      res += "|";
    }
    index++;
  }
  res += ")";
  return res;
}

struct TraceId {
  virtual ~TraceId() {}
  virtual std::string toString() const = 0;
  virtual void getNextId(const klee::ResultJson &resultJson) = 0;
};

class CooddyTraceId : public TraceId {
  std::string uid = "";

public:
  std::string toString() const override { return uid; }
  void getNextId(const klee::ResultJson &resultJson) override {
    uid = resultJson.fingerprints.value().cooddy_uid;
  }
};

class GetterTraceId : public TraceId {
  unsigned id = 0;

public:
  std::string toString() const override { return std::to_string(id); }
  void getNextId(const klee::ResultJson &resultJson) override {
    id = resultJson.id.value();
  }
};

class NumericTraceId : public TraceId {
  unsigned id = 0;

public:
  std::string toString() const override { return std::to_string(id); }
  void getNextId(const klee::ResultJson &resultJson) override { id++; }
};

std::unique_ptr<TraceId>
createTraceId(ToolName toolName, const std::vector<klee::ResultJson> &results) {
  if (toolName == ToolName::Cooddy)
    return std::make_unique<CooddyTraceId>();
  else if (results.size() > 0 && results[0].id.has_value())
    return std::make_unique<GetterTraceId>();
  return std::make_unique<NumericTraceId>();
}

void setResultId(const ResultJson &resultJson, bool useProperId, unsigned &id) {
  if (useProperId) {
    assert(resultJson.id.has_value() && "all results must have a proper id");
    id = resultJson.id.value();
  } else {
    ++id;
  }
}

ToolName convertToolName(const std::string &toolName) {
  if ("SecB" == toolName)
    return ToolName::SecB;
  if ("clang" == toolName)
    return ToolName::clang;
  if ("CppCheck" == toolName)
    return ToolName::CppCheck;
  if ("Infer" == toolName)
    return ToolName::Infer;
  if ("Cooddy" == toolName)
    return ToolName::Cooddy;
  return ToolName::Unknown;
}

SarifReport convertAndFilterSarifJson(const SarifReportJson &reportJson) {
  SarifReport report;

  if (reportJson.runs.size() == 0) {
    return report;
  }

  assert(reportJson.runs.size() == 1);

  const RunJson &run = reportJson.runs[0];
  auto toolName = convertToolName(run.tool.driver.name);

  auto id = createTraceId(toolName, run.results);

  for (const auto &resultJson : run.results) {
    id->getNextId(resultJson);
    auto maybeResult =
        tryConvertResultJson(resultJson, toolName, id->toString());
    if (maybeResult.has_value()) {
      report.results.push_back(*maybeResult);
    }
  }

  return report;
}

Location::EquivLocationHashSet Location::cachedLocations;
Location::LocationHashSet Location::locations;

bool LocRange::hasInside(KInstruction *ki) const {
  if (isa<llvm::DbgInfoIntrinsic>(ki->inst))
    return false;
  InstrWithPrecision kp(ki);
  hasInsideInternal(kp);
  auto suitable = maxPrecision();
  return kp.precision >= suitable;
}

void LocRange::hasInside(InstrWithPrecision &kp) {
  if (kp.precision > maxPrecision()) {
    kp.setNotFound();
    return;
  }
  if (hasInsideInternal(kp))
    setRange(kp.ptr);
}

class InstructionRange : public LocRange {
  LineColumnRange range;
  OpCode opCode;

public:
  InstructionRange(LineColumnRange &&range, OpCode opCode)
      : range(std::move(range)), opCode(opCode) {}

  LineColumnRange getRange() const final { return range; }

  Precision maxPrecision() const final { return Precision::Instruction; }

  size_t hash() const final { return hash_combine2(range.hash(), opCode); }

  std::string toString() const final {
    return range.toString() + " " + std::to_string(opCode);
  }

  void setRange(const KInstruction *ki) final { range.setRange(ki); }

  bool hasInsideInternal(InstrWithPrecision &kp) const final {
    range.hasInsideInternal(kp);
    if (kp.isNotFound())
      return false;
    if (kp.ptr->inst->getOpcode() != opCode)
      return false;
    if (hasInsidePrecise(kp.ptr)) {
      kp.precision = Precision::Instruction;
      return true;
    } else {
      kp.precision = std::min(kp.precision, Precision::Column);
      return false;
    }
  }

protected:
  virtual bool hasInsidePrecise(const KInstruction *ki) const { return true; }
};

namespace Cooddy {
using namespace llvm;

class StoreNullRange final : public InstructionRange {
  using InstructionRange::InstructionRange;

public:
  StoreNullRange(LineColumnRange &&range)
      : InstructionRange(std::move(range), Instruction::Store) {}

  bool hasInsidePrecise(const KInstruction *ki) const final {
    auto stinst = dyn_cast<StoreInst>(ki->inst);
    if (!stinst)
      return false;
    auto value = dyn_cast<Constant>(stinst->getValueOperand());
    if (!value)
      return false;
    return value->isNullValue();
  }
};

class BranchRange final : public InstructionRange {
  using InstructionRange::InstructionRange;

public:
  BranchRange(LineColumnRange &&range)
      : InstructionRange(std::move(range), Instruction::Br) {}

  bool hasInsidePrecise(const KInstruction *ki) const final {
    return ki->inst->getNumSuccessors() == 2;
  }
};

struct OpCodeLoc final : public Location {
  using Location::Location;
  OpCodeLoc(std::string &&filename_, LineColumnRange &&range, EventKind &kind,
            OpCode opCode)
      : Location(std::move(filename_),
                 std::make_unique<InstructionRange>(std::move(range), opCode),
                 kind) {}
};

struct AfterLoc : public Location {
  using Location::Location;
  void isInsideInternal(BlockWithPrecision &bp,
                        const Instructions &origInsts) const final;

protected:
  virtual void isInsideInternal(BlockWithPrecision &bp,
                                const InstrWithPrecision &afterInst) const = 0;
};

struct AfterBlockLoc final : public AfterLoc {
  using AfterLoc::AfterLoc;
  AfterBlockLoc(std::string &&filename_, std::unique_ptr<LocRange> range,
                EventKind &kind, unsigned indexOfNext)
      : AfterLoc(std::move(filename_), std::move(range), kind),
        indexOfNext(indexOfNext) {}
  void isInsideInternal(BlockWithPrecision &bp,
                        const InstrWithPrecision &afterInst) const final;

private:
  unsigned indexOfNext;
};

struct AfterInstLoc final : public AfterLoc {
  using AfterLoc::AfterLoc;
  AfterInstLoc(std::string &&filename_, std::unique_ptr<LocRange> range,
               EventKind &kind, OpCode opCode)
      : AfterLoc(std::move(filename_), std::move(range), kind), opCode(opCode) {
  }
  void isInsideInternal(BlockWithPrecision &bp,
                        const InstrWithPrecision &afterInst) const final;

private:
  OpCode opCode;
};
} // namespace Cooddy

LineColumnRange create(unsigned int startLine_, optional<unsigned int> endLine_,
                       optional<unsigned int> startColumn_,
                       optional<unsigned int> endColumn_) {
  auto endLine = endLine_.has_value() ? *endLine_ : startLine_;
  if (LocationAccuracy && startColumn_.has_value()) {
    auto endColumn = endColumn_.has_value() ? *endColumn_ : *startColumn_;
    return LineColumnRange(startLine_, *startColumn_, endLine, endColumn);
  }
  return LineColumnRange(startLine_, endLine);
}

bool Location::operator==(const Location &other) const {
  return filename == other.filename &&
         range->getRange() == other.range->getRange();
}

void Location::computeHash(EventKind &kind) {
  hash_combine(hashValue, filename);
  hash_combine(hashValue, range->hash());
  hash_combine(hashValue, kind);
}

Location *Location::createCooddy(std::string &&filename_,
                                 LineColumnRange &range, EventKind &kind) {
  if (kind.isError) {
    if (kind.kinds.size() == 1) {
      switch (kind.kinds[0]) {
      case ReachWithError::DoubleFree:
        return new Cooddy::OpCodeLoc(std::move(filename_), std::move(range),
                                     kind, Instruction::Call);
      default:
        return nullptr;
      }
    } else if (std::find(kind.kinds.begin(), kind.kinds.end(),
                         ReachWithError::MustBeNullPointerException) !=
               kind.kinds.end()) {
      // the thing that Cooddy reports is too complex, so we fallback to just
      // lines
      range.clearColumns();
      return nullptr;
      // return new Cooddy::OpCodeLoc(std::move(filename_), std::move(range),
      // kind, Instruction::Load);
    }
    return nullptr;
  }
  switch (kind.kind) {
  case ReachWithoutError::Return:
    return new Cooddy::OpCodeLoc(std::move(filename_), std::move(range), kind,
                                 Instruction::Ret);
  case ReachWithoutError::NPESource:
    return new Location(
        std::move(filename_),
        std::make_unique<Cooddy::StoreNullRange>(std::move(range)), kind);
  case ReachWithoutError::BranchTrue:
  case ReachWithoutError::BranchFalse: {
    auto succ = kind.kind == ReachWithoutError::BranchTrue ? 0 : 1;
    return new Cooddy::AfterBlockLoc(
        std::move(filename_),
        std::make_unique<Cooddy::BranchRange>(std::move(range)), kind, succ);
  }
  case ReachWithoutError::Free:
  case ReachWithoutError::Call:
  case ReachWithoutError::AfterCall:
    return new Cooddy::OpCodeLoc(std::move(filename_), std::move(range), kind,
                                 Instruction::Call);
  case ReachWithoutError::Reach:
    return nullptr;
  }
}

ref<Location> Location::create(std::string &&filename_, unsigned int startLine_,
                               optional<unsigned int> endLine_,
                               optional<unsigned int> startColumn_,
                               optional<unsigned int> endColumn_,
                               ToolName toolName, EventKind &kind) {
  auto range = klee::create(startLine_, endLine_, startColumn_, endColumn_);
  Location *loc = nullptr;
  switch (toolName) {
  case ToolName::Cooddy:
    loc = createCooddy(std::move(filename_), range, kind);
    break;
  default:
    break;
  }
  if (!loc)
    loc =
        new Location(std::move(filename_),
                     std::make_unique<LineColumnRange>(std::move(range)), kind);
  std::pair<EquivLocationHashSet::const_iterator, bool> success =
      cachedLocations.insert(loc);
  if (success.second) {
    // Cache miss
    locations.insert(loc);
    return loc;
  }
  // Cache hit
  delete loc;
  loc = *(success.first);
  return loc;
}

Location::~Location() {
  if (locations.find(this) != locations.end()) {
    locations.erase(this);
    cachedLocations.erase(this);
  }
}

bool Location::isInside(const std::string &name) const {
  size_t suffixSize = 0;
  int m = name.size() - 1, n = filename.size() - 1;
  for (; m >= 0 && n >= 0 && name[m] == filename[n]; m--, n--) {
    suffixSize++;
    if (isOSSeparator(filename[n]))
      return true;
  }
  return suffixSize >= 3 && (n == -1 ? (m == -1 || isOSSeparator(name[m]))
                                     : (m == -1 && isOSSeparator(filename[n])));
}

void Location::isInside(InstrWithPrecision &kp,
                        const Instructions &origInsts) const {
  auto ki = kp.ptr;
  // TODO: exterminate origInsts!
  auto it = origInsts.find(ki->getLine());
  if (it == origInsts.end()) {
    kp.setNotFound();
    return;
  }
  auto it2 = it->second.find(ki->getColumn());
  if (it2 == it->second.end()) {
    kp.setNotFound();
    return;
  }
  if (!it2->second.count(ki->inst->getOpcode())) {
    kp.setNotFound();
    return;
  }
  range->hasInside(kp);
}

void Location::isInside(BlockWithPrecision &bp,
                        const Instructions &origInsts) const {
  if (bp.precision > range->maxPrecision())
    bp.setNotFound();
  else
    isInsideInternal(bp, origInsts);
}

void Location::isInsideInternal(BlockWithPrecision &bp,
                                const Instructions &origInsts) const {
  bool found = false;
  for (size_t i = 0; i < bp.ptr->numInstructions; ++i) {
    InstrWithPrecision kp(bp.ptr->instructions[i], bp.precision);
    isInside(kp, origInsts);
    if (kp.precision >= bp.precision) {
      bp.precision = kp.precision;
      found = true;
      if (kp.precision >= range->maxPrecision()) {
        bp.ptr = kp.ptr->parent;
        return;
      }
    }
  }
  if (!found)
    bp.setNotFound();
}

void Cooddy::AfterLoc::isInsideInternal(BlockWithPrecision &bp,
                                        const Instructions &origInsts) const {
  // if (x + y > z && aaa->bbb->ccc->ddd)
  // ^^^^^^^^^^^^^^^^^ first, skip all this
  // second skip this ^^^^^^^^ (where Cooddy event points)
  // finally, get this         ^ (real location of needed instruction)
  auto inside = false;
  InstrWithPrecision afterInst(nullptr, bp.precision);
  for (size_t i = 0; i < bp.ptr->numInstructions; ++i) {
    afterInst.ptr = bp.ptr->instructions[i];
    auto kp = afterInst;
    Location::isInside(kp, origInsts);
    if (kp.precision >= afterInst.precision) { // first: go until Cooddy event
      afterInst.precision = kp.precision;
      inside = true;   // first: reached Cooddy event
    } else if (inside) // second: skip until left Coody event
      break;
  }
  if (!inside) { // no Cooddy event in this Block
    bp.setNotFound();
    return;
  }
  isInsideInternal(bp, afterInst);
}

void Cooddy::AfterBlockLoc::isInsideInternal(
    BlockWithPrecision &bp, const InstrWithPrecision &afterInst) const {
  if (afterInst.precision != Precision::Instruction) {
    bp.setNotFound();
    return;
  }
  auto nextBlock =
      bp.ptr->basicBlock->getTerminator()->getSuccessor(indexOfNext);
  bp.ptr = bp.ptr->parent->blockMap.at(nextBlock);
  bp.precision = afterInst.precision;
  range->setRange(bp.ptr->getFirstInstruction());
}

void Cooddy::AfterInstLoc::isInsideInternal(
    BlockWithPrecision &bp, const InstrWithPrecision &afterInst) const {
  if (afterInst.ptr->inst->getOpcode() != opCode) {
    bp.precision = std::min(afterInst.precision, Precision::Column);
    return;
  }
  bp.precision = Precision::Instruction;
  range->setRange(afterInst.ptr);
}

std::string Location::toString() const {
  return filename + ":" + range->toString();
}
} // namespace klee
