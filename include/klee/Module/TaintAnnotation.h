#ifndef KLEE_TAINT_ANNOTATION_H
#define KLEE_TAINT_ANNOTATION_H

#include "nlohmann/json.hpp"
#include "nonstd/optional.hpp"

#include "map"
#include "set"
#include "string"
#include "vector"

using json = nlohmann::json;

namespace klee {

using source_ty = size_t;
using sink_ty = size_t;
using rule_ty = size_t;

struct TaintHitInfo final {
  source_ty source;
  sink_ty sink;

  explicit TaintHitInfo(source_ty source, sink_ty sink);

  bool operator<(const TaintHitInfo &other) const;
  bool operator==(const TaintHitInfo &other) const;
};

using TaintHitsMap = std::map<TaintHitInfo, rule_ty>;

struct TaintAnnotation final {
  TaintHitsMap hits;

  std::unordered_map<std::string, source_ty> sources;
  std::unordered_map<std::string, sink_ty> sinks;
  std::vector<std::string> rules;

  explicit TaintAnnotation(const std::string &path);
};

} // namespace klee

#endif // KLEE_TAINT_ANNOTATION_H
