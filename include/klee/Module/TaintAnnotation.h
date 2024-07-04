#ifndef KLEE_TAINT_ANNOTATION_H
#define KLEE_TAINT_ANNOTATION_H

#include "nlohmann/json.hpp"

#include <optional>
#include <set>

using json = nlohmann::json;

namespace klee {

using source_ty = size_t;
using sink_ty = size_t;
using rule_ty = size_t;

struct TaintAnnotation final {
  using TaintHitsSink = std::map<source_ty, rule_ty>;
  using TaintHitsMap = std::map<sink_ty, TaintHitsSink>;

  TaintHitsMap hits;

  std::unordered_map<std::string, source_ty> sources;
  std::unordered_map<std::string, sink_ty> sinks;
  std::vector<std::string> rules;

  explicit TaintAnnotation(const std::string &path);
};

} // namespace klee

#endif // KLEE_TAINT_ANNOTATION_H
