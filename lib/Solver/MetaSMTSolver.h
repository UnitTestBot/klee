//===-- MetaSMTSolver.h
//---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_METASMTSOLVER_H
#define KLEE_METASMTSOLVER_H

#include "klee/Solver/Solver.h"

#include <memory>

namespace klee {

template <typename SolverContext> class MetaSMTSolver : public Solver {
public:
  MetaSMTSolver(bool useForked, bool optimizeDivides);
  virtual ~MetaSMTSolver();

  std::string getConstraintLog(const Query &) final;
  virtual void setCoreSolverLimits(time::Span timeout, unsigned memoryLimit);
};

/// createMetaSMTSolver - Create a solver using the metaSMT backend set by
/// the option MetaSMTBackend.
std::unique_ptr<Solver> createMetaSMTSolver();
} // namespace klee

#endif /* KLEE_METASMTSOLVER_H */
