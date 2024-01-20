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

  std::set<std::string> sourcesStrs;
  for (auto &item : taintAnnotationsJson.items()) {
    if (!item.value().is_array()) {
      klee_error("Taint annotations: Incorrect file format");
    }
    const auto sourcesForThisSink = item.value().get<std::set<std::string>>();
    sourcesStrs.insert(sourcesForThisSink.begin(), sourcesForThisSink.end());
  }

  size_t sourcesCounter = 0;
  for (auto &sourceStr : sourcesStrs) {
    sources[sourceStr] = sourcesCounter;
    sourcesCounter++;
  }

  size_t sinksCounter = 0;
  for (auto &item : taintAnnotationsJson.items()) {
    sinks[item.key()] = sinksCounter;
    std::set<size_t> sourcesForThisSink;
    for (auto &sourceStr : item.value()) {
      sourcesForThisSink.insert(sources[sourceStr]);
    }
    sinksToSources[sinksCounter] = sourcesForThisSink;
    sinksCounter++;
  }
}

TaintAnnotation::~TaintAnnotation() = default;

} // namespace klee
