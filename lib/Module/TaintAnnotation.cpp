#include "klee/Module/TaintAnnotation.h"
#include "klee/Support/ErrorHandling.h"

#include <fstream>

namespace klee {

TaintAnnotation::TaintAnnotation(const std::string &path) {
  if (path.empty()) {
    return;
  }

  std::ifstream taintAnnotationsFile(path);
  if (!taintAnnotationsFile.good()) {
    klee_error("Taint annotation: Opening %s failed.", path.c_str());
  }
  json taintAnnotationsJson = json::parse(taintAnnotationsFile, nullptr, false);
  if (taintAnnotationsJson.is_discarded()) {
    klee_error("Taint annotation: Parsing JSON %s failed.", path.c_str());
  }

  std::set<std::string> sourcesStr;
  std::set<std::string> rulesStr;
  for (auto &item : taintAnnotationsJson.items()) {
    if (!item.value().is_array()) {
      klee_error("Taint annotations: Incorrect file format");
    }
    for (auto &taintHitJson : item.value()) {
      sourcesStr.insert(taintHitJson["source"]);
      rulesStr.insert(taintHitJson["rule"]);
    }
  }

  rules = std::vector(rulesStr.begin(), rulesStr.end());
  std::map<std::string, rule_ty> rulesMap;
  for (size_t i = 0; i < rules.size(); ++i) {
    rulesMap[rules[i]] = i;
  }

  size_t sourcesCounter = 0;
  for (auto &sourceStr : sourcesStr) {
    sources[sourceStr] = sourcesCounter;
    sourcesCounter++;
  }

  size_t sinksCounter = 0;
  for (auto &item : taintAnnotationsJson.items()) {
    sinks[item.key()] = sinksCounter;
    if (!item.value().is_array()) {
      klee_error("Taint annotations: Incorrect file format");
    }

    std::map<source_ty, rule_ty> hitsForSink;
    for (auto &taintHitJson : item.value()) {
      hitsForSink[sources[taintHitJson["source"]]] =
          rulesMap[taintHitJson["rule"]];
    }
    hits[sinksCounter] = hitsForSink;
    sinksCounter++;
  }
}

} // namespace klee
