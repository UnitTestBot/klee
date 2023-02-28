//===-- SarifReport.h --------------------------------------------*- C++
//-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SARIF_REPORT_H
#define KLEE_SARIF_REPORT_H

#include <string>
#include <vector>

#include <klee/Misc/json.hpp>
#include <klee/Misc/optional.hpp>

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
struct ArtifactLocation {
  optional<std::string> uri;
};

struct Region {
  optional<int> startLine;
  optional<int> endLine;
  optional<int> startColumn;
  optional<int> endColumn;
};

struct PhysicalLocation {
  optional<ArtifactLocation> artifactLocation;
  optional<Region> region;
};

struct Location {
  optional<PhysicalLocation> physicalLocation;
};

struct ThreadFlowLocation {
  optional<Location> location;
};

struct ThreadFlow {
  std::vector<ThreadFlowLocation> locations;
};

struct CodeFlow {
  std::vector<ThreadFlow> threadFlows;
};

struct Result {
  optional<std::string> ruleId;
  std::vector<Location> locations;
  std::vector<CodeFlow> codeFlows;
};

struct Driver {
  std::string name;
};

struct Tool {
  Driver driver;
};

struct Run {
  std::vector<Result> results;
  Tool tool;
};

struct SarifReport {
  std::vector<Run> runs;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArtifactLocation, uri)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Region, startLine, endLine,
                                                startColumn, endColumn)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicalLocation,
                                                artifactLocation, region)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Location, physicalLocation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowLocation, location)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlow, locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CodeFlow, threadFlows)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Result, ruleId, codeFlows,
                                                locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Driver, name)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Tool, driver)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Run, results, tool)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SarifReport, runs)
} // namespace klee

#endif /* KLEE_SARIF_REPORT_H */
