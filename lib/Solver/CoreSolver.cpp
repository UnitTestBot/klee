//===-- CoreSolver.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/config.h"

#ifdef ENABLE_BITWUZLA
#include "BitwuzlaSolver.h"
#endif

#ifdef ENABLE_METASMT
#include "MetaSMTSolver.h"
#endif

#ifdef ENABLE_STP
#include "STPSolver.h"
#endif

#ifdef ENABLE_Z3
#include "Z3Solver.h"
#endif

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/Support/ErrorHandling.h"

#include <memory>
#include <string>

namespace klee {

std::unique_ptr<Solver> createCoreSolver(CoreSolverType cst) {
  bool isTreeSolver = (cst == Z3_TREE_SOLVER || cst == BITWUZLA_TREE_SOLVER);
  if (!isTreeSolver && MaxSolversApproxTreeInc > 0)
    klee_warning("--%s option is ignored because --%s is not z3-tree",
                 MaxSolversApproxTreeInc.ArgStr.str().c_str(),
                 CoreSolverToUse.ArgStr.str().c_str());
  switch (cst) {
  case STP_SOLVER:
#ifdef ENABLE_STP
    klee_message("Using STP solver backend");
    return std::make_unique<STPSolver>(UseForkedCoreSolver,
                                       CoreSolverOptimizeDivides);
#else
    klee_message("Not compiled with STP support");
    return NULL;
#endif
  case METASMT_SOLVER:
#ifdef ENABLE_METASMT
    ProduceUnsatCore = false;
    klee_message("Using MetaSMT solver backend");
    if (ProduceUnsatCore) {
      ProduceUnsatCore = false;
      klee_message(
          "Unsat cores are only supported by Z3, disabling unsat cores.");
    }
    return createMetaSMTSolver();
#else
    klee_message("Not compiled with MetaSMT support");
    return NULL;
#endif
  case DUMMY_SOLVER:
    return createDummySolver();
  case Z3_TREE_SOLVER:
  case Z3_SOLVER:
#ifdef ENABLE_Z3
    klee_message("Using Z3 solver backend");
    Z3BuilderType type;
#ifdef ENABLE_FP
    klee_message("Using Z3 bitvector builder");
    type = KLEE_BITVECTOR;
#else
    klee_message("Using Z3 core builder");
    type = KLEE_CORE;
#endif
    if (isTreeSolver) {
      if (MaxSolversApproxTreeInc > 0)
        return std::make_unique<Z3TreeSolver>(type, MaxSolversApproxTreeInc);
      klee_warning("--%s is 0, so falling back to non tree-incremental solver ",
                   MaxSolversApproxTreeInc.ArgStr.str().c_str());
    }
    return std::make_unique<Z3Solver>(type);
#else
    klee_message("Not compiled with Z3 support");
    return NULL;
#endif
  case BITWUZLA_TREE_SOLVER:
  case BITWUZLA_SOLVER:
#ifdef ENABLE_BITWUZLA
    klee_message("Using Bitwuzla solver backend");
    if (isTreeSolver) {
      if (MaxSolversApproxTreeInc > 0)
        return std::make_unique<BitwuzlaTreeSolver>(MaxSolversApproxTreeInc);
      klee_warning("--%s is 0, so falling back to non tree-incremental solver",
                   MaxSolversApproxTreeInc.ArgStr.str().c_str());
    }
    return std::make_unique<BitwuzlaSolver>();
#else
    klee_message("Not compiled with Bitwuzla support");
    return NULL;
#endif
  case NO_SOLVER:
    klee_message("Invalid solver");
    return NULL;
  default:
    llvm_unreachable("Unsupported CoreSolverType");
  }
}
} // namespace klee
