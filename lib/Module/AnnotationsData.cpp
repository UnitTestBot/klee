#include "klee/Module/AnnotationsData.h"

namespace klee {

klee::AnnotationsData::AnnotationsData(const std::string &annotationsFile,
                                       const std::string &taintAnnotationsFile)
    : taintAnnotation(taintAnnotationsFile) {
  annotations = parseAnnotations(annotationsFile);
}

AnnotationsData::~AnnotationsData() = default;

} // namespace klee
