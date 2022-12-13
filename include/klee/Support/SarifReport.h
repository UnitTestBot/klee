//===-- SarifReport.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SARIF_REPORT_H
#define KLEE_SARIF_REPORT_H

#include <vector>
#include <string>

#include <klee/Misc/json.hpp>
using json = nlohmann::json;

namespace klee {
    template<typename T>
    struct Option {
        T value = {};
        bool is_empty = true;
    };

    template<typename T>
    void to_json(json& j, const Option<T>& p) {
        if (p.is_empty) {
            j = nullptr;
        } else {
            j = p.value;
        }
    }

    template<typename T>
    void from_json(const json& j, Option<T>& p) {
        if (j.is_null()) {
            p.is_empty = true;
        } else {
            p.value = j.get<T>();
            p.is_empty = false;
        }
    }

    struct Message {
        Option<std::string> text;
        Option<std::string> id;
    };

    struct ArtifactLocation {
        Option<std::string> uri;
    };

    struct Region {
        Option<int> startLine;
        Option<int> endLine;
        Option<int> startColumn;
        Option<int> endColumn;
    };

    struct PhysicalLocation {
        Option<ArtifactLocation> artifactLocation;
        Option<Region> region;
    };

    struct Location {
        Option<PhysicalLocation> physicalLocation;
    };

    struct ThreadFlowLocation {
        Option<Location> location; 
        Option<int> nestingLevel;
    };

    struct ThreadFlow {
        std::vector<ThreadFlowLocation> locations;
    };

    struct CodeFlow {
        std::vector<ThreadFlow> threadFlows;
    };

    struct Result {
        Message message;
        Option<std::string> ruleId;
        std::vector<Location> locations;
        std::vector<CodeFlow> codeFlows;
    };

    struct Run {
        std::vector<Result> results;
    };

    struct SarifReport {
        std::vector<Run> runs;
    };


    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Message, text, id)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArtifactLocation, uri)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Region, startLine, endLine, startColumn, endColumn)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicalLocation, artifactLocation, region)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Location, physicalLocation)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowLocation, location, nestingLevel)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlow, locations)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CodeFlow, threadFlows)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Result, message, ruleId, codeFlows, locations)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Run, results)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SarifReport, runs)
}

#endif /* KLEE_SARIF_REPORT_H */
