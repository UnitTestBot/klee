#ifndef SMITHRILSOLVER_H_
#define SMITHRILSOLVER_H_

#include "klee/Solver/Solver.h"

namespace klee {

/// SmithrilCompleteSolver - A complete solver based on Smithril
class SmithrilSolver : public Solver {
public:
  /// SmithrilSolver - Construct a new SmithrilSolver.
  SmithrilSolver();
  std::string getConstraintLog(const Query &) final;
};

class SmithrilTreeSolver : public Solver {
public:
  SmithrilTreeSolver(unsigned maxSolvers);
};

} // namespace klee

#endif
