//===-- SarifReport.h --------------------------------------------*- C++-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SARIF_REPORT_H
#define KLEE_SARIF_REPORT_H

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "klee/ADT/Ref.h"
#include "llvm/IR/IntrinsicInst.h"
#include <nlohmann/json.hpp>
#include <nonstd/optional.hpp>

using json = nlohmann::json;
using nonstd::optional;

namespace nlohmann {
template <typename T> struct adl_serializer<nonstd::optional<T>> {
  static void to_json(json &j, const nonstd::optional<T> &opt) {
    if (opt == nonstd::nullopt) {
      j = nullptr;
    } else {
      j = *opt;
    }
  }

  static void from_json(const json &j, nonstd::optional<T> &opt) {
    if (j.is_null()) {
      opt = nonstd::nullopt;
    } else {
      opt = j.get<T>();
    }
  }
};
} // namespace nlohmann

namespace klee {
enum ReachWithError {
  DoubleFree = 0,
  UseAfterFree,
  MayBeNullPointerException,  // void f(int *x) { *x = 42; } - should it error?
  MustBeNullPointerException, // MayBeNPE = yes, MustBeNPE = no
  NullCheckAfterDerefException,
  Reachable,
  None,
};
using ReachWithErrors = std::vector<ReachWithError>;

const char *getErrorString(ReachWithError error);
std::string getErrorsString(const ReachWithErrors &errors);

struct FunctionInfo;
struct KBlock;

struct ArtifactLocationJson {
  optional<std::string> uri;
};

struct Message {
  std::string text;
};

struct RegionJson {
  optional<unsigned int> startLine;
  optional<unsigned int> endLine;
  optional<unsigned int> startColumn;
  optional<unsigned int> endColumn;
  optional<Message> message;
};

struct PhysicalLocationJson {
  optional<ArtifactLocationJson> artifactLocation;
  optional<RegionJson> region;
};

struct LocationJson {
  optional<PhysicalLocationJson> physicalLocation;
};

struct ThreadFlowLocationJson {
  optional<LocationJson> location;
  optional<json> metadata;
};

struct ThreadFlowJson {
  std::vector<ThreadFlowLocationJson> locations;
};

struct CodeFlowJson {
  std::vector<ThreadFlowJson> threadFlows;
};

struct Fingerprints {
  std::string cooddy_uid;
};

static void to_json(json &j, const Fingerprints &p) {
  j = json{{"cooddy.uid", p.cooddy_uid}};
}

static void from_json(const json &j, Fingerprints &p) {
  j.at("cooddy.uid").get_to(p.cooddy_uid);
}

struct ResultJson {
  optional<std::string> ruleId;
  optional<Message> message;
  optional<unsigned> id;
  optional<Fingerprints> fingerprints;
  std::vector<LocationJson> locations;
  std::vector<CodeFlowJson> codeFlows;
};

struct DriverJson {
  std::string name;
};

struct ToolJson {
  DriverJson driver;
};

struct RunJson {
  std::vector<ResultJson> results;
  ToolJson tool;
};

struct SarifReportJson {
  std::vector<RunJson> runs;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArtifactLocationJson, uri)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RegionJson, startLine, endLine,
                                                startColumn, endColumn, message)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicalLocationJson,
                                                artifactLocation, region)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LocationJson, physicalLocation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowLocationJson,
                                                location, metadata)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowJson, locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CodeFlowJson, threadFlows)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Message, text)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResultJson, ruleId, message, id,
                                                fingerprints, codeFlows,
                                                locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DriverJson, name)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToolJson, driver)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RunJson, results, tool)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SarifReportJson, runs)

enum class Precision { NotFound = 0, Line = 1, Column = 2, Instruction = 3 };

template <class T> struct WithPrecision {
  T *ptr;
  Precision precision;

  explicit WithPrecision(T *p, Precision pr) : ptr(p), precision(pr) {}
  explicit WithPrecision(T *p) : WithPrecision(p, Precision::NotFound) {}
  explicit WithPrecision() : WithPrecision(nullptr) {}

  void setNotFound() { precision = Precision::NotFound; }
  bool isNotFound() const { return precision == Precision::NotFound; }
};

struct KBlock;
struct KInstruction;

using BlockWithPrecision = WithPrecision<KBlock>;
using InstrWithPrecision = WithPrecision<KInstruction>;

inline size_t hash_combine2(std::size_t s, std::size_t v) {
  return s ^ (v + 0x9e3779b9 + (s << 6) + (s >> 2));
}

template <class T> inline void hash_combine(std::size_t &s, const T &v) {
  std::hash<T> h;
  s = hash_combine2(s, h(v));
}

enum class ReachWithoutError {
  Reach = 0,
  Return,
  NPESource,
  Free,
  BranchFalse,
  BranchTrue,
  Call,
  AfterCall
};

struct EventKind final {
  const bool isError;
  const ReachWithErrors kinds;
  const ReachWithoutError kind;
  size_t hashValue = 0;
  void computeHash() {
    hash_combine(hashValue, isError);
    for (auto k : kinds)
      hash_combine(hashValue, k);
    hash_combine(hashValue, kind);
  }
  EventKind(ReachWithErrors &&kinds)
      : isError(true), kinds(kinds), kind(ReachWithoutError::Reach) {
    computeHash();
  }
  EventKind(ReachWithoutError kind) : isError(false), kind(kind) {
    computeHash();
  }
};
} // namespace klee

namespace std {
template <> struct hash<klee::EventKind> {
  size_t operator()(const klee::EventKind &k) const { return k.hashValue; }
};
} // namespace std

namespace klee {
enum class ToolName { Unknown = 0, SecB, clang, CppCheck, Infer, Cooddy };

class LineColumnRange;

struct LocRange {
  virtual LineColumnRange getRange() const = 0;
  virtual ~LocRange() = default;
  virtual Precision maxPrecision() const = 0;
  virtual size_t hash() const = 0;
  virtual std::string toString() const = 0;
  bool hasInside(KInstruction *ki) const;
  void hasInside(InstrWithPrecision &kp);
  virtual void setRange(const KInstruction *ki) = 0;

protected:
  virtual bool hasInsideInternal(InstrWithPrecision &kp) const = 0;
};

class LineColumnRange final : public LocRange {
  size_t startLine;
  size_t startColumn;
  size_t endLine;
  size_t endColumn;
  static const size_t empty = std::numeric_limits<size_t>::max();

  bool inline onlyLine() const { return startColumn == empty; }

public:
  explicit LineColumnRange(size_t startLine, size_t startColumn, size_t endLine,
                           size_t endColumn)
      : startLine(startLine), startColumn(startColumn), endLine(endLine),
        endColumn(endColumn) {
    assert(startLine <= endLine);
    assert(startLine != endLine || startColumn <= endColumn);
  }
  explicit LineColumnRange(size_t startLine, size_t endLine)
      : LineColumnRange(startLine, empty, endLine, empty) {}
  explicit LineColumnRange(const KInstruction *ki) { setRange(ki); }

  void setRange(const KInstruction *ki) final;

  LineColumnRange getRange() const final { return *this; }

  void clearColumns() { startColumn = (endColumn = empty); }

  Precision maxPrecision() const final {
    return onlyLine() ? Precision::Line : Precision::Column;
  }

  size_t hash() const final {
    size_t hashValue = 0;
    hashValue = hash_combine2(hashValue, startLine);
    hashValue = hash_combine2(hashValue, endLine);
    hashValue = hash_combine2(hashValue, startColumn);
    return hash_combine2(hashValue, endColumn);
  }

  std::string toString() const final {
    if (onlyLine())
      return std::to_string(startLine) + "-" + std::to_string(endLine);
    return std::to_string(startLine) + ":" + std::to_string(startColumn) + "-" +
           std::to_string(endLine) + ":" + std::to_string(endColumn);
  }

  bool hasInsideInternal(InstrWithPrecision &kp) const final;

  bool operator==(const LineColumnRange &p) const {
    return startLine == p.startLine && endLine == p.endLine &&
           startColumn == p.startColumn && endColumn == p.endColumn;
  }
};

using OpCode = unsigned;

struct Location {
  struct LocationHash {
    std::size_t operator()(const Location *l) const { return l->hash(); }
  };

  struct LocationCmp {
    bool operator()(const Location *a, const Location *b) const {
      return a == b;
    }
  };

  struct EquivLocationCmp {
    bool operator()(const Location *a, const Location *b) const {
      if (a == NULL || b == NULL)
        return false;
      return *a == *b;
    }
  };
  std::string filename;
  std::unique_ptr<LocRange> range;

  static ref<Location> create(std::string &&filename_, unsigned int startLine_,
                              optional<unsigned int> endLine_,
                              optional<unsigned int> startColumn_,
                              optional<unsigned int> endColumn_,
                              ToolName toolName, EventKind &kind);

  virtual ~Location();
  virtual std::size_t hash() const { return hashValue; }

  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  bool operator==(const Location &other) const;

  bool isInside(const std::string &name) const;

  using Instructions = std::unordered_map<
      unsigned int,
      std::unordered_map<unsigned int, std::unordered_set<OpCode>>>;

  void isInside(InstrWithPrecision &kp, const Instructions &origInsts) const;
  void isInside(BlockWithPrecision &bp, const Instructions &origInsts) const;

  std::string toString() const;

private:
  typedef std::unordered_set<Location *, LocationHash, EquivLocationCmp>
      EquivLocationHashSet;
  typedef std::unordered_set<Location *, LocationHash, LocationCmp>
      LocationHashSet;

  static EquivLocationHashSet cachedLocations;
  static LocationHashSet locations;

  size_t hashValue = 0;
  void computeHash(EventKind &kind);

  static Location *createCooddy(std::string &&filename_, LineColumnRange &range,
                                EventKind &kind);

protected:
  Location(std::string &&filename_, std::unique_ptr<LocRange> range,
           EventKind &kind)
      : filename(std::move(filename_)), range(std::move(range)) {
    computeHash(kind);
  }

  virtual void isInsideInternal(BlockWithPrecision &bp,
                                const Instructions &origInsts) const;
};

struct RefLocationHash {
  unsigned operator()(const ref<Location> &t) const { return t->hash(); }
};

struct RefLocationCmp {
  bool operator()(const ref<Location> &a, const ref<Location> &b) const {
    return a.get() == b.get();
  }
};

struct Result {
  std::vector<ref<Location>> locations;
  const std::vector<optional<json>> metadatas;
  const std::string id;
  const ReachWithErrors errors;
};

struct SarifReport {
  std::vector<Result> results;

  bool empty() const { return results.empty(); }
};

SarifReport convertAndFilterSarifJson(const SarifReportJson &reportJson);

} // namespace klee

#endif /* KLEE_SARIF_REPORT_H */
