//===-- ArrayExprHash.h --------------------------------------------*- C++
//-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETHASH_H
#define KLEE_TARGETHASH_H

#include "Target.h"
#include "TargetForest.h"

#include <map>
#include <unordered_map>

namespace klee {

struct TargetHash {
  unsigned operator()(const Target *t) const {
    if (t) {
      return t->hash();
    } else {
      return 0;
    }
  }
};

struct TargetCmp {
  bool operator()(const Target *a, const Target *b) const {
    return a == b;
  }
};

struct EquivTargetCmp {
    bool operator()(const Target *a, const Target *b) const {
    if (a == NULL || b == NULL)
      return false;
    return a->compare(*b) == 0;
  }
};

struct RefTargetHash {
  unsigned operator()(const ref<Target> &t) const {
    return t->hash();
  }
};

struct RefTargetCmp {
  bool operator()(const ref<Target> &a, const ref<Target> &b) const {
    return a.get() == b.get();
  }
};

struct RefTargetLess {
  bool operator()(const ref<Target> &a, const ref<Target> &b) const {
    return a.get() < b.get();
  }
};

} // namespace klee
#endif /* KLEE_TARGETHASH_H */
