#ifndef KLEE_CONCRETIZATIONMANAGER_H
#define KLEE_CONCRETIZATIONMANAGER_H

#include "klee/ADT/MapOfSets.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include <unordered_map>

namespace klee {
struct Query;

class ConcretizationManager {
private:
  MapOfSets<ref<Expr>, Assignment> concretizations;

public:
  Assignment get(const ConstraintSet &set);
  void add(const ConstraintSet &oldCS, const ConstraintSet &newCS,
           const Assignment &assign);
  void add(const Query &q, const Assignment &assign);
};

}; // end klee namespace

#endif /* KLEE_CONCRETIZATIONMANAGER_H */
