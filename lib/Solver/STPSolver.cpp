//===-- STPSolver.cpp -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Statistics/TimerStatIncrementer.h"

#ifdef ENABLE_STP

#include "STPBuilder.h"
#include "STPSolver.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Errno.h"

#include <array>
#include <csignal>
#include <memory>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

llvm::cl::opt<bool> DebugDumpSTPQueries(
    "debug-dump-stp-queries", llvm::cl::init(false),
    llvm::cl::desc("Dump every STP query to stderr (default=false)"),
    llvm::cl::cat(klee::SolvingCat));

llvm::cl::opt<bool> IgnoreSolverFailures(
    "ignore-solver-failures", llvm::cl::init(false),
    llvm::cl::desc("Ignore any STP solver failures (default=false)"),
    llvm::cl::cat(klee::SolvingCat));

enum SAT { MINISAT, SIMPLEMINISAT, CRYPTOMINISAT, RISS };
const std::array<std::string, 4> SATNames{"MiniSat", "simplifying MiniSat",
                                          "CryptoMiniSat", "RISS"};

llvm::cl::opt<SAT> SATSolver(
    "stp-sat-solver",
    llvm::cl::desc(
        "Set the underlying SAT solver for STP (default=cryptominisat)"),
    llvm::cl::values(clEnumValN(SAT::MINISAT, "minisat",
                                SATNames[SAT::MINISAT]),
                     clEnumValN(SAT::SIMPLEMINISAT, "simpleminisat",
                                SATNames[SAT::SIMPLEMINISAT]),
                     clEnumValN(SAT::CRYPTOMINISAT, "cryptominisat",
                                SATNames[SAT::CRYPTOMINISAT]),
                     clEnumValN(SAT::RISS, "riss", SATNames[SAT::RISS])),
    llvm::cl::init(CRYPTOMINISAT), llvm::cl::cat(klee::SolvingCat));
} // namespace

#define vc_bvBoolExtract IAMTHESPAWNOFSATAN

static unsigned char *shared_memory_ptr = nullptr;
static int shared_memory_id = 0;
// Darwin by default has a very small limit on the maximum amount of shared
// memory, which will quickly be exhausted by KLEE running its tests in
// parallel. For now, we work around this by just requesting a smaller size --
// in practice users hitting this limit on counterexample sizes probably already
// are hitting more serious scalability issues.
#ifdef __APPLE__
static const unsigned shared_memory_size = 1 << 16;
#else
static const unsigned shared_memory_size = 1 << 20;
#endif

static void stp_error_handler(const char *err_msg) {
  fprintf(stderr, "error: STP Error: %s\n", err_msg);
  abort();
}

namespace klee {

class STPSolverImpl : public SolverImpl {
private:
  VC vc;
  std::unique_ptr<STPBuilder> builder;
  time::Span timeout;
  bool useForkedSTP;
  SolverRunStatus runStatusCode;

public:
  explicit STPSolverImpl(bool useForkedSTP, bool optimizeDivides = true);
  ~STPSolverImpl() override;

  std::string getConstraintLog(const Query &) final;
  void setCoreSolverLimits(time::Span timeout,
                           [[maybe_unused]] unsigned memoryLimit) override {
    this->timeout = timeout;
  }
  void notifyStateTermination(std::uint32_t) override {}

  bool computeTruth(const Query &, bool &isValid) override;
  bool computeValue(const Query &, ref<Expr> &result) override;
  bool
  computeInitialValues(const Query &, const std::vector<const Array *> &objects,
                       std::vector<SparseStorageImpl<unsigned char>> &values,
                       bool &hasSolution) override;
  SolverRunStatus getOperationStatusCode() override;
};

STPSolverImpl::STPSolverImpl(bool useForkedSTP, bool optimizeDivides)
    : vc(vc_createValidityChecker()),
      builder(new STPBuilder(vc, optimizeDivides)), useForkedSTP(useForkedSTP),
      runStatusCode(SOLVER_RUN_STATUS_FAILURE) {
  assert(vc && "unable to create validity checker");
  assert(builder && "unable to create STPBuilder");

  // In newer versions of STP, a memory management mechanism has been
  // introduced that automatically invalidates certain C interface
  // pointers at vc_Destroy time.  This caused double-free errors
  // due to the ExprHandle destructor also attempting to invalidate
  // the pointers using vc_DeleteExpr.  By setting EXPRDELETE to 0
  // we restore the old behaviour.
  vc_setInterfaceFlags(vc, EXPRDELETE, 0);

  // set SAT solver
  bool SATSolverAvailable = false;
  bool specifiedOnCommandLine = SATSolver.getNumOccurrences() > 0;
  switch (SATSolver) {
  case SAT::MINISAT: {
    SATSolverAvailable = vc_useMinisat(vc);
    break;
  }
  case SAT::SIMPLEMINISAT: {
    SATSolverAvailable = vc_useSimplifyingMinisat(vc);
    break;
  }
  case SAT::CRYPTOMINISAT: {
    SATSolverAvailable = vc_useCryptominisat(vc);
    break;
  }
  case SAT::RISS: {
    SATSolverAvailable = vc_useRiss(vc);
    break;
  }
  default:
    assert(false && "Illegal SAT solver value.");
  }

  // print SMT/SAT status
  const auto expectedSATName = SATNames[SATSolver.getValue()];
  std::string SATName{"unknown"};
  if (vc_isUsingMinisat(vc))
    SATName = SATNames[SAT::MINISAT];
  else if (vc_isUsingSimplifyingMinisat(vc))
    SATName = SATNames[SAT::SIMPLEMINISAT];
  else if (vc_isUsingCryptominisat(vc))
    SATName = SATNames[SAT::CRYPTOMINISAT];
  else if (vc_isUsingRiss(vc))
    SATName = SATNames[SAT::RISS];

  if (!specifiedOnCommandLine || SATSolverAvailable) {
    klee_message("SAT solver: %s", SATName.c_str());
  } else {
    klee_warning("%s not supported by STP", expectedSATName.c_str());
    klee_message("Fallback SAT solver: %s", SATName.c_str());
  }

  make_division_total(vc);

  vc_registerErrorHandler(::stp_error_handler);

  if (useForkedSTP) {
    assert(shared_memory_id == 0 && "shared memory id already allocated");
    shared_memory_id =
        shmget(IPC_PRIVATE, shared_memory_size, IPC_CREAT | 0700);
    if (shared_memory_id < 0)
      llvm::report_fatal_error("unable to allocate shared memory region");
    shared_memory_ptr = (unsigned char *)shmat(shared_memory_id, nullptr, 0);
    if (shared_memory_ptr == (void *)-1)
      llvm::report_fatal_error("unable to attach shared memory region");
    shmctl(shared_memory_id, IPC_RMID, nullptr);
  }
}

STPSolverImpl::~STPSolverImpl() {
  // Detach the memory region.
  shmdt(shared_memory_ptr);
  shared_memory_ptr = nullptr;
  shared_memory_id = 0;

  builder.reset();

  vc_Destroy(vc);
}

/***/

std::string STPSolverImpl::getConstraintLog(const Query &query) {
  vc_push(vc);

  for (const auto &constraint : query.constraints.cs())
    vc_assertFormula(vc, builder->construct(constraint));
  assert(query.expr == ConstantExpr::alloc(0, Expr::Bool) &&
         "Unexpected expression in query!");

  char *buffer;
  unsigned long length;
  vc_printQueryStateToBuffer(vc, builder->getFalse(), &buffer, &length, false);
  vc_pop(vc);

  std::string result(buffer);
  std::free(buffer);

  return result;
}

bool STPSolverImpl::computeTruth(const Query &query, bool &isValid) {
  std::vector<const Array *> objects;
  std::vector<SparseStorageImpl<unsigned char>> values;
  bool hasSolution;

  if (!computeInitialValues(query, objects, values, hasSolution))
    return false;

  isValid = !hasSolution;
  return true;
}

bool STPSolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  std::vector<const Array *> objects;
  std::vector<SparseStorageImpl<unsigned char>> values;
  bool hasSolution;

  // Find the object used in the expression, and compute an assignment
  // for them.
  findSymbolicObjects(query.expr, objects);
  if (!computeInitialValues(query.withFalse(), objects, values, hasSolution))
    return false;
  assert(hasSolution && "state has invalid constraint set");

  // Evaluate the expression with the computed assignment.
  Assignment a(objects, values);
  result = a.evaluate(query.expr);

  return true;
}

static SolverImpl::SolverRunStatus
runAndGetCex(::VC vc, STPBuilder *builder, ::VCExpr q,
             const std::vector<const Array *> &objects,
             std::vector<SparseStorageImpl<unsigned char>> &values,
             bool &hasSolution) {
  // XXX I want to be able to timeout here, safely
  hasSolution = !vc_query(vc, q);

  if (!hasSolution)
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;

  values.reserve(objects.size());
  unsigned i = 0; // FIXME C++17: use reference from emplace_back()
  for (const auto object : objects) {
    uint64_t objectSize = 0;
    if (ref<ConstantExpr> sizeExpr = dyn_cast<ConstantExpr>(object->size)) {
      objectSize = sizeExpr->getZExtValue();
    } else {
      ExprHandle sizeHandle = builder->construct(object->size);
      objectSize = getBVUnsignedLongLong(sizeHandle);
    }

    values.emplace_back(objectSize);

    for (unsigned offset = 0; offset < objectSize; offset++) {
      ExprHandle counter =
          vc_getCounterExample(vc, builder->getInitialRead(object, offset));
      values[i].store(offset,
                      static_cast<unsigned char>(getBVUnsigned(counter)));
    }
    ++i;
  }

  return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
}

static void stpTimeoutHandler(int) { _exit(52); }

static SolverImpl::SolverRunStatus
runAndGetCexForked(::VC vc, STPBuilder *builder, ::VCExpr q,
                   const std::vector<const Array *> &objects,
                   std::vector<SparseStorageImpl<unsigned char>> &values,
                   bool &hasSolution, time::Span timeout) {
  unsigned char *pos = shared_memory_ptr;
  // unsigned sum = 0;
  // for (const auto object : objects)
  //   sum += object->size;
  // if (sum >= shared_memory_size)
  //   llvm::report_fatal_error("not enough shared memory for counterexample");

  fflush(stdout);
  fflush(stderr);

  // We will allocate additional buffer for shared memory
  size_t memory_object_size = objects.size() * sizeof(uint64_t);
  uint64_t *shared_memory_object_sizes = static_cast<uint64_t *>(
      mmap(NULL, memory_object_size, PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS, -1, 0));

  // fork solver
  int pid = fork();
  // - error
  if (pid == -1) {
    klee_warning("fork failed (for STP) - %s",
                 llvm::sys::StrError(errno).c_str());
    if (!IgnoreSolverFailures)
      exit(1);
    munmap(shared_memory_object_sizes, memory_object_size);
    return SolverImpl::SOLVER_RUN_STATUS_FORK_FAILED;
  }
  // - child (solver)
  if (pid == 0) {
    if (timeout) {
      ::alarm(0); /* Turn off alarm so we can safely set signal handler */
      ::signal(SIGALRM, stpTimeoutHandler);
      ::alarm(std::max(1u, static_cast<unsigned>(timeout.toSeconds())));
    }
    int res = vc_query(vc, q);
    if (!res) {
      for (unsigned idx = 0; idx < objects.size(); ++idx) {
        const Array *object = objects[idx];

        /* Receive size for array */
        unsigned int sizeConstant = 0;
        if (ref<ConstantExpr> sizeExpr = dyn_cast<ConstantExpr>(object->size)) {
          sizeConstant = sizeExpr->getZExtValue();
        } else {
          ExprHandle sizeHandle = builder->construct(object->size);
          sizeConstant =
              getBVUnsignedLongLong(vc_getCounterExample(vc, sizeHandle));
        }
        shared_memory_object_sizes[idx] = sizeConstant;

        /* Then fill required bytes */
        for (unsigned offset = 0; offset < shared_memory_object_sizes[idx];
             offset++) {
          ExprHandle counter =
              vc_getCounterExample(vc, builder->getInitialRead(object, offset));
          *pos++ = static_cast<unsigned char>(getBVUnsigned(counter));
        }
      }
    }
    _exit(res);
    // - parent
  } else {
    int status;
    pid_t res;

    do {
      res = waitpid(pid, &status, 0);
    } while (res < 0 && errno == EINTR);

    if (res < 0) {
      klee_warning("waitpid() for STP failed");
      munmap(shared_memory_object_sizes, memory_object_size);
      if (!IgnoreSolverFailures)
        exit(1);
      return SolverImpl::SOLVER_RUN_STATUS_WAITPID_FAILED;
    }

    // From timed_run.py: It appears that linux at least will on
    // "occasion" return a status when the process was terminated by a
    // signal, so test signal first.
    if (WIFSIGNALED(status) || !WIFEXITED(status)) {
      klee_warning("STP did not return successfully.  Most likely you forgot "
                   "to run 'ulimit -s unlimited'");
      munmap(shared_memory_object_sizes, memory_object_size);
      if (!IgnoreSolverFailures) {
        exit(1);
      }
      return SolverImpl::SOLVER_RUN_STATUS_INTERRUPTED;
    }

    int exitcode = WEXITSTATUS(status);

    // solvable
    if (exitcode == 0) {
      hasSolution = true;

      values.reserve(objects.size());
      for (unsigned idx = 0; idx < objects.size(); ++idx) {
        uint64_t objectSize = shared_memory_object_sizes[idx];
        values.emplace_back(0);
        values.back().store(0, pos, pos + objectSize);
        pos += objectSize;
      }

      munmap(shared_memory_object_sizes, memory_object_size);
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }

    munmap(shared_memory_object_sizes, memory_object_size);
    // unsolvable
    if (exitcode == 1) {
      hasSolution = false;
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
    }

    // timeout
    if (exitcode == 52) {
      klee_warning("STP timed out");
      // mark that a timeout occurred
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    }

    // unknown return code
    klee_warning("STP did not return a recognized code");
    if (!IgnoreSolverFailures)
      exit(1);
    return SolverImpl::SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE;
  }
}

bool STPSolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution) {
  runStatusCode = SOLVER_RUN_STATUS_FAILURE;
  TimerStatIncrementer t(stats::queryTime);

  vc_push(vc);

  for (const auto &constraint : query.constraints.cs())
    vc_assertFormula(vc, builder->construct(constraint));

  ++stats::solverQueries;
  ++stats::queryCounterexamples;

  ExprHandle stp_e = builder->construct(query.expr);

  if (DebugDumpSTPQueries) {
    char *buf;
    unsigned long len;
    vc_printQueryStateToBuffer(vc, stp_e, &buf, &len, false);
    klee_warning("STP query:\n%.*s\n", (unsigned)len, buf);
    free(buf);
  }

  bool success;
  if (useForkedSTP) {
    runStatusCode = runAndGetCexForked(vc, builder.get(), stp_e, objects,
                                       values, hasSolution, timeout);
    success = ((SOLVER_RUN_STATUS_SUCCESS_SOLVABLE == runStatusCode) ||
               (SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE == runStatusCode));
  } else {
    runStatusCode =
        runAndGetCex(vc, builder.get(), stp_e, objects, values, hasSolution);
    success = true;
  }

  if (success) {
    if (hasSolution)
      ++stats::queriesInvalid;
    else
      ++stats::queriesValid;
  }

  vc_pop(vc);

  return success;
}

SolverImpl::SolverRunStatus STPSolverImpl::getOperationStatusCode() {
  return runStatusCode;
}

STPSolver::STPSolver(bool useForkedSTP, bool optimizeDivides)
    : Solver(std::make_unique<STPSolverImpl>(useForkedSTP, optimizeDivides)) {}

std::string STPSolver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

void STPSolver::setCoreSolverLimits(time::Span timeout, unsigned memoryLimit) {
  impl->setCoreSolverLimits(timeout, memoryLimit);
}

} // namespace klee
#endif // ENABLE_STP
