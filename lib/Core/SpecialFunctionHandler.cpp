//===-- SpecialFunctionHandler.cpp ----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SpecialFunctionHandler.h"

#include "CodeEvent.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "Memory.h"
#include "MemoryManager.h"
#include "Searcher.h"
#include "StatsTracker.h"
#include "TimingSolver.h"

#include "klee/Config/config.h"
#include "klee/Core/Context.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/Casting.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"
#include "klee/klee.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include <array>
#include <cerrno>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace {
cl::opt<bool>
    ReadablePosix("readable-posix-inputs", cl::init(false),
                  cl::desc("Prefer creation of POSIX inputs (command-line "
                           "arguments, files, etc.) with human readable bytes. "
                           "Note: option is expensive when creating lots of "
                           "tests (default=false)"),
                  cl::cat(TestGenCat));

cl::opt<bool>
    SilentKleeAssume("silent-klee-assume", cl::init(false),
                     cl::desc("Silently terminate paths with an infeasible "
                              "condition given to klee_assume rather than "
                              "emitting an error (default=false)"),
                     cl::cat(TerminationCat));

cl::opt<bool> CheckOutOfMemory("check-out-of-memory", cl::init(false),
                               cl::desc("Enable out-of-memory checking during "
                                        "memory allocation (default=false)"),
                               cl::cat(ExecCat));
} // namespace

/// \todo Almost all of the demands in this file should be replaced
/// with terminateState calls.

///

// FIXME: We are more or less committed to requiring an intrinsic
// library these days. We can move some of this stuff there,
// especially things like realloc which have complicated semantics
// w.r.t. forking. Among other things this makes delayed query
// dispatch easier to implement.
static constexpr std::array handlerInfo = {
#define add(name, handler, ret)                                                \
  SpecialFunctionHandler::HandlerInfo {                                        \
    name, &SpecialFunctionHandler::handler, false, ret, false                  \
  }
#define addDNR(name, handler)                                                  \
  SpecialFunctionHandler::HandlerInfo {                                        \
    name, &SpecialFunctionHandler::handler, true, false, false                 \
  }
    addDNR("__assert_rtn", handleAssertFail),
    addDNR("__assert_fail", handleAssertFail),
    addDNR("__assert", handleAssertFail),
    addDNR("_assert", handleAssert),
    addDNR("abort", handleAbort),
    addDNR("_exit", handleExit),
    SpecialFunctionHandler::HandlerInfo{
        "exit", &SpecialFunctionHandler::handleExit, true, false, true},
    addDNR("klee_abort", handleAbort),
    addDNR("klee_silent_exit", handleSilentExit),
    addDNR("klee_report_error", handleReportError),
    add("aligned_alloc", handleMemalign, true),
    add("calloc", handleCalloc, true),
    add("free", handleFree, false),
    add("klee_assume", handleAssume, false),
    add("klee_sleep", handleSleep, false),
    add("klee_check_memory_access", handleCheckMemoryAccess, false),
    add("klee_get_valuef", handleGetValue, true),
    add("klee_get_valued", handleGetValue, true),
    add("klee_get_valuel", handleGetValue, true),
    add("klee_get_valuell", handleGetValue, true),
    add("klee_get_value_i32", handleGetValue, true),
    add("klee_get_value_i64", handleGetValue, true),
    add("klee_define_fixed_object", handleDefineFixedObject, true),
    add("klee_get_obj_size", handleGetObjSize, true),
    add("klee_get_errno", handleGetErrno, true),
#ifndef __APPLE__
    add("__errno_location", handleErrnoLocation, true),
#else
    add("__error", handleErrnoLocation, true),
#endif
    add("klee_is_symbolic", handleIsSymbolic, true),
    add("klee_make_symbolic", handleMakeSymbolic, false),
    add("klee_make_mock", handleMakeMock, false),
    add("klee_mark_global", handleMarkGlobal, false),
    add("klee_prefer_cex", handlePreferCex, false),
    add("klee_posix_prefer_cex", handlePosixPreferCex, false),
    add("klee_print_expr", handlePrintExpr, false),
    add("klee_print_range", handlePrintRange, false),
    add("klee_set_forking", handleSetForking, false),
    add("klee_stack_trace", handleStackTrace, false),
    add("klee_warning", handleWarning, false),
    add("klee_warning_once", handleWarningOnce, false),
    add("klee_dump_constraints", handleDumpConstraints, false),
    add("malloc", handleMalloc, true),
    add("memalign", handleMemalign, true),
    add("realloc", handleRealloc, true),

#ifdef SUPPORT_KLEE_EH_CXX
    add("_klee_eh_Unwind_RaiseException_impl", handleEhUnwindRaiseExceptionImpl,
        false),
    add("klee_eh_typeid_for", handleEhTypeid, true),
#endif

    // operator delete[](void*)
    add("_ZdaPv", handleDeleteArray, false),
    // operator delete(void*)
    add("_ZdlPv", handleDelete, false),

    // operator new[](unsigned int)
    add("_Znaj", handleNewArray, true),
    // operator new(unsigned int)
    add("_Znwj", handleNew, true),

    // FIXME-64: This is wrong for 64-bit long...

    // operator new[](unsigned long)
    add("_Znam", handleNewArray, true),
    // operator new[](unsigned long, std::nothrow_t const&)
    add("_ZnamRKSt9nothrow_t", handleNewNothrowArray, true),
    // operator new(unsigned long)
    add("_Znwm", handleNew, true),

    // float classification instrinsics
    add("klee_is_nan_float", handleIsNaN, true),
    add("klee_is_nan_double", handleIsNaN, true),
    add("klee_is_nan_long_double", handleIsNaN, true),
    add("klee_is_infinite_float", handleIsInfinite, true),
    add("klee_is_infinite_double", handleIsInfinite, true),
    add("klee_is_infinite_long_double", handleIsInfinite, true),
    add("klee_is_normal_float", handleIsNormal, true),
    add("klee_is_normal_double", handleIsNormal, true),
    add("klee_is_normal_long_double", handleIsNormal, true),
    add("klee_is_subnormal_float", handleIsSubnormal, true),
    add("klee_is_subnormal_double", handleIsSubnormal, true),
    add("klee_is_subnormal_long_double", handleIsSubnormal, true),

    // Rounding mode intrinsics
    add("klee_get_rounding_mode", handleGetRoundingMode, true),
    add("klee_set_rounding_mode_internal", handleSetConcreteRoundingMode,
        false),

    // square root
    add("klee_sqrt_float", handleSqrt, true),
    add("klee_sqrt_double", handleSqrt, true),
#if defined(__x86_64__) || defined(__i386__)
    add("klee_sqrt_long_double", handleSqrt, true),
#endif

    // floating point absolute
    add("klee_abs_float", handleFAbs, true),
    add("klee_abs_double", handleFAbs, true),
#if defined(__x86_64__) || defined(__i386__)
    add("klee_abs_long_double", handleFAbs, true),
#endif
    add("klee_rint", handleRint, true),
    add("klee_rintf", handleRint, true),
#if defined(__x86_64__) || defined(__i386__)
    add("klee_rintl", handleRint, true),
#if defined(HAVE_CTYPE_EXTERNALS)
    add("__ctype_b_loc", handleCTypeBLoc, true),
    add("__ctype_tolower_loc", handleCTypeToLowerLoc, true),
    add("__ctype_toupper_loc", handleCTypeToUpperLoc, true),
#endif
#endif
#undef addDNR
#undef add
};

SpecialFunctionHandler::SpecialFunctionHandler(Executor &_executor)
    : executor(_executor) {}

void SpecialFunctionHandler::prepare(
    std::vector<const char *> &preservedFunctions) {
  for (const auto &hi : handlerInfo) {
    Function *f = executor.kmodule->module->getFunction(hi.name);

    // No need to create if the function doesn't exist, since it cannot
    // be called in that case.
    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      preservedFunctions.push_back(hi.name);
      // Make sure NoReturn attribute is set, for optimization and
      // coverage counting.
      if (hi.doesNotReturn)
        f->addFnAttr(Attribute::NoReturn);

      // Change to a declaration since we handle internally (simplifies
      // module and allows deleting dead code).
      if (!f->isDeclaration())
        f->deleteBody();
    }
  }
}

void SpecialFunctionHandler::bind() {
  for (const auto &hi : handlerInfo) {
    Function *f = executor.kmodule->module->getFunction(hi.name);

    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      handlers[f] = std::make_pair(hi.handler, hi.hasReturnValue);

      if (executor.kmodule->functionMap.count(f)) {
        executor.kmodule->functionMap.at(f)->kleeHandled = true;
      }
    }
  }
}

bool SpecialFunctionHandler::handle(ExecutionState &state, Function *f,
                                    KInstruction *target,
                                    std::vector<ref<Expr>> &arguments) {
  handlers_ty::iterator it = handlers.find(f);
  if (it != handlers.end()) {
    Handler h = it->second.first;
    bool hasReturnValue = it->second.second;
    // FIXME: Check this... add test?
    if (!hasReturnValue && !target->inst()->use_empty()) {
      executor.terminateStateOnExecError(
          state, "expected return value from void special function");
    } else {
      (this->*h)(state, target, arguments);
    }
    return true;
  } else {
    return false;
  }
}

/****/

// reads a concrete string from memory
std::string
SpecialFunctionHandler::readStringAtAddress(ExecutionState &state,
                                            ref<PointerExpr> pointer) {
  ObjectPair idStringAddress;
  ref<PointerExpr> uniquePointer = executor.toUnique(state, pointer);
  if (!isa<ConstantPointerExpr>(uniquePointer)) {
    executor.terminateStateOnUserError(
        state, "Symbolic string pointer passed to one of the klee_ functions");
    return "";
  }
  ref<ConstantPointerExpr> pointerConst =
      cast<ConstantPointerExpr>(uniquePointer);
  if (!state.addressSpace.resolveOne(pointerConst, idStringAddress)) {
    executor.terminateStateOnUserError(
        state, "Invalid string pointer passed to one of the klee_ functions");
    return "";
  }
  ObjectPair op = idStringAddress;
  const MemoryObject *mo = op.first;
  const ObjectState *os = op.second;

  // the relativeOffset must be concrete as the address is concrete
  size_t offset = pointerConst->getConstantOffset()->getZExtValue();

  std::ostringstream buf;
  char c = 0;
  ref<ConstantExpr> sizeExpr = dyn_cast<ConstantExpr>(mo->getSizeExpr());
  assert(sizeExpr);
  size_t moSize = sizeExpr->getZExtValue();
  for (size_t i = offset; i < moSize; ++i) {
    ref<Expr> cur = os->read8(i);
    cur = executor.toUnique(state, cur);
    assert(isa<ConstantExpr>(cur) &&
           "hit symbolic char while reading concrete string");
    c = cast<ConstantExpr>(cur)->getZExtValue(8);
    if (c == '\0') {
      // we read the whole string
      break;
    }

    buf << c;
  }

  if (c != '\0') {
    klee_warning_once(0, "String not terminated by \\0 passed to "
                         "one of the klee_ functions");
  }

  return buf.str();
}

/****/

void SpecialFunctionHandler::handleAbort(
    ExecutionState &state, KInstruction *,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 0 && "invalid number of arguments to abort");
  executor.terminateStateOnProgramError(
      state, new ErrorEvent(executor.locationOf(state),
                            StateTerminationType::Abort, "abort failure"));
}

void SpecialFunctionHandler::handleExit(
    ExecutionState &state, KInstruction *,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to exit");
  executor.terminateStateOnExit(state);
}

void SpecialFunctionHandler::handleSilentExit(
    ExecutionState &state, KInstruction *,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to exit");
  executor.terminateStateEarlyUser(state, "");
}

void SpecialFunctionHandler::handleAssert(ExecutionState &state, KInstruction *,
                                          std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 3 && "invalid number of arguments to _assert");
  executor.terminateStateOnProgramError(
      state,
      new ErrorEvent(
          executor.locationOf(state), StateTerminationType::Assert,
          "ASSERTION FAIL: " +
              readStringAtAddress(state, executor.makePointer(arguments[0]))));
}

void SpecialFunctionHandler::handleAssertFail(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 4 &&
         "invalid number of arguments to __assert_fail");
  executor.terminateStateOnProgramError(
      state,
      new ErrorEvent(
          executor.locationOf(state), StateTerminationType::Assert,
          "ASSERTION FAIL: " +
              readStringAtAddress(state, executor.makePointer(arguments[0]))));
}

void SpecialFunctionHandler::handleReportError(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 4 &&
         "invalid number of arguments to klee_report_error");

  // arguments[0,1,2,3] are file, line, message, suffix
  executor.terminateStateOnProgramError(
      state,
      new ErrorEvent(
          executor.locationOf(state), StateTerminationType::ReportError,
          readStringAtAddress(state, executor.makePointer(arguments[2]))),
      "",
      readStringAtAddress(state, executor.makePointer(arguments[3])).c_str());
}

void SpecialFunctionHandler::handleNew(ExecutionState &state,
                                       KInstruction *target,
                                       std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to new");
  executor.executeAlloc(state, arguments[0], false, target, false, nullptr, 0,
                        CheckOutOfMemory);
}

void SpecialFunctionHandler::handleDelete(ExecutionState &state, KInstruction *,
                                          std::vector<ref<Expr>> &arguments) {
  // FIXME: Should check proper pairing with allocation type (malloc/free,
  // new/delete, new[]/delete[]).

  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to delete");
  executor.executeFree(state, executor.makePointer(arguments[0]));
}

void SpecialFunctionHandler::handleNewArray(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to new[]");
  executor.executeAlloc(state, arguments[0], false, target, false, nullptr, 0,
                        CheckOutOfMemory);
}

void SpecialFunctionHandler::handleNewNothrowArray(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 &&
         "invalid number of arguments to new[](unsigned long, std::nothrow_t "
         "const&)");
  executor.executeAlloc(state, arguments[0], false, target, false, nullptr, 0,
                        CheckOutOfMemory);
}

void SpecialFunctionHandler::handleDeleteArray(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to delete[]");
  executor.executeFree(state, executor.makePointer(arguments[0]));
}

void SpecialFunctionHandler::handleMalloc(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to malloc");
  executor.executeAlloc(state, arguments[0], false, target, false, nullptr, 0,
                        CheckOutOfMemory);
}

void SpecialFunctionHandler::handleMemalign(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  if (arguments.size() != 2) {
    executor.terminateStateOnUserError(
        state, "Incorrect number of arguments to memalign(size_t alignment, "
               "size_t size)");
    return;
  }

  std::pair<ref<Expr>, ref<Expr>> alignmentRangeExpr =
      executor.solver->getRange(state.constraints.cs(), arguments[0],
                                state.queryMetaData);
  ref<Expr> alignmentExpr = alignmentRangeExpr.first;
  auto alignmentConstExpr = dyn_cast<ConstantExpr>(alignmentExpr);

  if (!alignmentConstExpr) {
    executor.terminateStateOnUserError(
        state, "Could not determine size of symbolic alignment");
    return;
  }

  uint64_t alignment = alignmentConstExpr->getZExtValue();

  // Warn, if the expression has more than one solution
  if (alignmentRangeExpr.first != alignmentRangeExpr.second) {
    klee_warning_once(
        0, "Symbolic alignment for memalign. Choosing smallest alignment");
  }

  executor.executeAlloc(state, arguments[1], false, target, false, 0,
                        alignment);
}

#ifdef SUPPORT_KLEE_EH_CXX
void SpecialFunctionHandler::handleEhUnwindRaiseExceptionImpl(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to _klee_eh_Unwind_RaiseException_impl");

  ref<ConstantPointerExpr> exceptionObject =
      dyn_cast<ConstantPointerExpr>(executor.makePointer(arguments[0]));
  if (!exceptionObject.get()) {
    executor.terminateStateOnExecError(
        state, "Internal error: Symbolic exception pointer");
    return;
  }

  if (isa_and_nonnull<SearchPhaseUnwindingInformation>(
          state.unwindingInformation.get())) {
    executor.terminateStateOnExecError(
        state,
        "Internal error: Unwinding restarted during an ongoing search phase");
    return;
  }

  state.unwindingInformation =
      std::make_unique<SearchPhaseUnwindingInformation>(exceptionObject,
                                                        state.stack.size() - 1);

  executor.unwindToNextLandingpad(state);
}

void SpecialFunctionHandler::handleEhTypeid(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_eh_typeid_for");

  executor.bindLocal(target, state, executor.getEhTypeidFor(arguments[0]));
}
#endif // SUPPORT_KLEE_EH_CXX

void SpecialFunctionHandler::handleSleep(ExecutionState &, KInstruction *,
                                         std::vector<ref<Expr>> &) {
  nanosleep((const struct timespec[]){{1, 0}}, NULL);
}

void SpecialFunctionHandler::handleAssume(ExecutionState &state, KInstruction *,
                                          std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to klee_assume");

  ref<Expr> e = arguments[0];

  if (e->getWidth() != Expr::Bool)
    e = NeExpr::create(e, ConstantExpr::create(0, e->getWidth()));

  bool res;
  bool success __attribute__((unused)) = executor.solver->mustBeFalse(
      state.constraints.cs(), e, res, state.queryMetaData);
  assert(success && "FIXME: Unhandled solver failure");
  if (res) {
    executor.terminateStateOnUserError(
        state, "invalid klee_assume call (provably false)", !SilentKleeAssume);
  } else {
    executor.addConstraint(state, e);
  }
}

void SpecialFunctionHandler::handleIsSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_is_symbolic");

  executor.bindLocal(
      target, state,
      ConstantExpr::create(!isa<ConstantExpr>(arguments[0]), Expr::Int32));
}

void SpecialFunctionHandler::handlePreferCex(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_prefex_cex");

  ref<Expr> cond = arguments[1];
  if (cond->getWidth() != Expr::Bool)
    cond = NeExpr::create(cond, ConstantExpr::alloc(0, cond->getWidth()));

  state.addCexPreference(cond);
}

void SpecialFunctionHandler::handlePosixPreferCex(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (ReadablePosix)
    return handlePreferCex(state, target, arguments);
}

void SpecialFunctionHandler::handlePrintExpr(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_print_expr");

  std::string msg_str =
      readStringAtAddress(state, executor.makePointer(arguments[0]));
  llvm::errs() << msg_str << ":" << arguments[1] << "\n";
}

void SpecialFunctionHandler::handleSetForking(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_set_forking");
  ref<Expr> value = executor.toUnique(state, arguments[0]);

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    state.forkDisabled = CE->isZero();
  } else {
    executor.terminateStateOnUserError(
        state, "klee_set_forking requires a constant arg");
  }
}

void SpecialFunctionHandler::handleStackTrace(ExecutionState &state,
                                              KInstruction *,
                                              std::vector<ref<Expr>> &) {
  state.dumpStack(outs());
}

void SpecialFunctionHandler::handleWarning(ExecutionState &state,
                                           KInstruction *,
                                           std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_warning");

  std::string msg_str =
      readStringAtAddress(state, executor.makePointer(arguments[0]));
  klee_warning("%s: %s",
               state.stack.callStack().back().kf->function()->getName().data(),
               msg_str.c_str());
}

void SpecialFunctionHandler::handleWarningOnce(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_warning_once");

  std::string msg_str =
      readStringAtAddress(state, executor.makePointer(arguments[0]));
  klee_warning_once(
      0, "%s: %s",
      state.stack.callStack().back().kf->function()->getName().data(),
      msg_str.c_str());
}

void SpecialFunctionHandler::handleDumpConstraints(
    ExecutionState &state, KInstruction *,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {

  assert(arguments.size() == 0 &&
         "invalid number of arguments to klee_warning_once");
  state.constraints.cs().dump();
}

void SpecialFunctionHandler::handlePrintRange(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_print_range");

  std::string msg_str =
      readStringAtAddress(state, executor.makePointer(arguments[0]));
  llvm::errs() << msg_str << ":" << arguments[1];
  if (!isa<ConstantExpr>(arguments[1])) {
    // FIXME: Pull into a unique value method?
    ref<ConstantExpr> value;
    bool success __attribute__((unused)) = executor.solver->getValue(
        state.constraints.cs(), arguments[1], value, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    bool res;
    success = executor.solver->mustBeTrue(state.constraints.cs(),
                                          EqExpr::create(arguments[1], value),
                                          res, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    if (res) {
      llvm::errs() << " == " << value;
    } else {
      llvm::errs() << " ~= " << value;
      std::pair<ref<Expr>, ref<Expr>> res = executor.solver->getRange(
          state.constraints.cs(), arguments[1], state.queryMetaData);
      llvm::errs() << " (in [" << res.first << ", " << res.second << "])";
    }
  }
  llvm::errs() << "\n";
}

void SpecialFunctionHandler::handleGetObjSize(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_get_obj_size");
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "klee_get_obj_size");
  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo = it->first;
    executor.bindLocal(target, *it->second, mo->getSizeExpr());
  }
}

void SpecialFunctionHandler::handleGetErrno(
    ExecutionState &state, KInstruction *target,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 0 &&
         "invalid number of arguments to klee_get_errno");
#ifndef WINDOWS
  int *errno_addr = executor.getErrnoLocation();
#else
  int *errno_addr = nullptr;
#endif

  // Retrieve the memory object of the errno variable
  ObjectPair idErrnoObject;

  bool resolved = state.addressSpace.resolveOne(
      ConstantPointerExpr::create(Expr::createPointer((uint64_t)errno_addr),
                                  Expr::createPointer((uint64_t)errno_addr)),
      idErrnoObject);
  if (!resolved)
    executor.terminateStateOnUserError(state,
                                       "Could not resolve address for errno");
  const ObjectState *os = idErrnoObject.second;
  executor.bindLocal(target, state, os->read(0, Expr::Int32));
}

void SpecialFunctionHandler::handleErrnoLocation(
    ExecutionState &state, KInstruction *target,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {
  // Returns the address of the errno variable
  assert(arguments.size() == 0 &&
         "invalid number of arguments to __errno_location/__error");

#ifndef WINDOWS
  // int *errno_addr = executor.getErrnoLocation(state);
  int *errno_addr = executor.errno_addr;
#else
  int *errno_addr = nullptr;
#endif

  ref<ConstantExpr> errnoExpr = ConstantExpr::create(
      (uint64_t)errno_addr, executor.kmodule->targetData->getTypeSizeInBits(
                                target->inst()->getType()));

  executor.bindLocal(target, state, PointerExpr::create(errnoExpr, errnoExpr));
}
void SpecialFunctionHandler::handleCalloc(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 && "invalid number of arguments to calloc");

  ref<Expr> size = MulExpr::create(arguments[0], arguments[1]);
  executor.executeAlloc(state, size, false, target, true, nullptr, 0,
                        CheckOutOfMemory);
}

void SpecialFunctionHandler::handleRealloc(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 && "invalid number of arguments to realloc");
  ref<Expr> address = arguments[0];
  ref<Expr> size = arguments[1];
  ref<PointerExpr> addressPointer = executor.makePointer(address);

  Executor::StatePair zeroSize = executor.forkInternal(
      state, Expr::createIsZero(size), BranchType::Realloc);

  if (zeroSize.first) { // size == 0
    executor.executeFree(*zeroSize.first,
                         PointerExpr::create(addressPointer->getValue(),
                                             addressPointer->getValue()),
                         target);
  }
  if (zeroSize.second) { // size != 0
    Executor::StatePair zeroPointer = executor.forkInternal(
        *zeroSize.second, Expr::createIsZero(addressPointer->getValue()),
        BranchType::Realloc);

    if (zeroPointer.first) { // address == 0
      executor.executeAlloc(*zeroPointer.first, size, false, target, false,
                            nullptr, 0, CheckOutOfMemory);
    }
    if (zeroPointer.second) { // address != 0
      Executor::ExactResolutionList rl;
      executor.resolveExact(*zeroPointer.second, address, rl, "realloc");

      for (Executor::ExactResolutionList::iterator it = rl.begin(),
                                                   ie = rl.end();
           it != ie; ++it) {
        ref<const ObjectState> os =
            it->second->addressSpace.findOrLazyInitializeObject(it->first)
                .second;
        executor.executeAlloc(*it->second, size, false, target, false, os.get(),
                              0, CheckOutOfMemory);
      }
    }
  }
}

void SpecialFunctionHandler::handleFree(ExecutionState &state, KInstruction *,
                                        std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to free");
  executor.executeFree(state, executor.makePointer(arguments[0]));
}

void SpecialFunctionHandler::handleCheckMemoryAccess(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_check_memory_access");

  ref<PointerExpr> pointer = executor.makePointer(arguments[0]);
  ref<Expr> size = executor.toUnique(state, arguments[1]->getValue());
  ref<PointerExpr> uniquePointer = executor.toUnique(state, pointer);
  if (!isa<ConstantPointerExpr>(uniquePointer) || !isa<ConstantExpr>(size)) {
    executor.terminateStateOnUserError(
        state, "check_memory_access requires constant args");
  } else {
    ObjectPair idObject;

    if (!state.addressSpace.resolveOne(cast<ConstantPointerExpr>(uniquePointer),
                                       idObject)) {
      executor.terminateStateOnProgramError(
          state,
          new ErrorEvent(executor.locationOf(state), StateTerminationType::Ptr,
                         "check_memory_access: memory error"),
          executor.getAddressInfo(state, pointer));
    } else {
      const MemoryObject *mo = idObject.first;
      ref<Expr> chk = mo->getBoundsCheckPointer(
          pointer, cast<ConstantExpr>(size)->getZExtValue());
      if (!chk->isTrue()) {
        executor.terminateStateOnProgramError(
            state,
            new ErrorEvent(
                new AllocEvent(mo->allocSite), executor.locationOf(state),
                StateTerminationType::Ptr, "check_memory_access: memory error"),
            executor.getAddressInfo(state, pointer));
      }
    }
  }
}

void SpecialFunctionHandler::handleGetValue(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_get_value");

  executor.executeGetValue(state, arguments[0], target);
}

void SpecialFunctionHandler::handleDefineFixedObject(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_define_fixed_object");
  assert((isa<ConstantExpr>(arguments[0]) ||
          isa<ConstantPointerExpr>(arguments[0])) &&
         "expect constant address argument to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[1]) &&
         "expect constant size argument to klee_define_fixed_object");

  uint64_t address = isa<ConstantExpr>(arguments[0])
                         ? cast<ConstantExpr>(arguments[0])->getZExtValue()
                         : cast<ConstantPointerExpr>(arguments[0])
                               ->getConstantValue()
                               ->getZExtValue();
  uint64_t size = cast<ConstantExpr>(arguments[1])->getZExtValue();
  MemoryObject *mo =
      executor.memory->allocateFixed(address, size, executor.locationOf(state));
  executor.bindObjectInState(state, mo, false);
  mo->isUserSpecified = true; // XXX hack;

  executor.bindLocal(target, state,
                     PointerExpr::create(mo->getBaseExpr(), mo->getBaseExpr()));
}

void SpecialFunctionHandler::handleMakeSymbolic(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  std::string name;

  if (arguments.size() != 3) {
    executor.terminateStateOnUserError(
        state, "Incorrect number of arguments to klee_make_symbolic(void*, "
               "size_t, char*)");
    return;
  }

  ref<PointerExpr> pointer = PointerExpr::create(arguments[2]);
  name = pointer->getBase()->isZero() && pointer->getValue()->isZero()
             ? ""
             : readStringAtAddress(state, executor.makePointer(arguments[2]));

  if (name.length() == 0) {
    name = "unnamed";
    klee_warning("klee_make_symbolic: renamed empty name to \"unnamed\"");
  }

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "make_symbolic");

  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    RefObjectPair op =
        it->second->addressSpace.findOrLazyInitializeObject(it->first);
    const MemoryObject *mo = op.first;
    mo->setName(name);
    mo->updateTimestamp();

    ref<const ObjectState> old = op.second;
    ExecutionState *s = it->second;

    if (old->readOnly) {
      executor.terminateStateOnUserError(
          *s, "cannot make readonly object symbolic");
      return;
    }

    // FIXME: Type coercion should be done consistently somewhere.
    bool res;
    bool success __attribute__((unused)) = executor.solver->mustBeTrue(
        s->constraints.cs(),
        EqExpr::create(
            ZExtExpr::create(arguments[1], Context::get().getPointerWidth()),
            mo->getSizeExpr()),
        res, s->queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");

    if (res) {
      executor.executeMakeSymbolic(
          *s, mo,
          SourceBuilder::makeSymbolic(name,
                                      executor.updateNameVersion(*s, name)),
          false);
    } else {
      executor.terminateStateOnUserError(
          *s, "Wrong size given to klee_make_symbolic");
    }
  }
}

void SpecialFunctionHandler::handleMakeMock(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  std::string name;

  if (arguments.size() != 3) {
    executor.terminateStateOnUserError(state,
                                       "Incorrect number of arguments to "
                                       "klee_make_mock(void*, size_t, char*)");
    return;
  }

  name = arguments[2]->isZero()
             ? ""
             : readStringAtAddress(state, executor.makePointer(arguments[2]));

  if (name.empty()) {
    executor.terminateStateOnUserError(
        state, "Empty name of function in klee_make_mock");
    return;
  }

  KFunction *kf = target->parent->parent;

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "make_symbolic");

  for (auto &it : rl) {
    ObjectPair op = it.second->addressSpace.findObject(it.first);
    const MemoryObject *mo = op.first;
    mo->setName(name);
    mo->updateTimestamp();

    const ObjectState *old = op.second;
    ExecutionState *s = it.second;

    if (old->readOnly) {
      executor.terminateStateOnUserError(
          *s, "cannot make readonly object symbolic");
      return;
    }

    bool res;
    bool success __attribute__((unused)) = executor.solver->mustBeTrue(
        s->constraints.cs(),
        EqExpr::create(
            ZExtExpr::create(arguments[1], Context::get().getPointerWidth()),
            mo->getSizeExpr()),
        res, s->queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");

    if (res) {
      ref<SymbolicSource> source;
      switch (executor.interpreterOpts.MockStrategy) {
      case MockStrategyKind::Naive:
        source =
            SourceBuilder::mockNaive(executor.kmodule.get(), *kf->function(),
                                     executor.updateNameVersion(state, name));
        break;
      case MockStrategyKind::Deterministic:
        std::vector<ref<Expr>> args(kf->getNumArgs());
        for (size_t i = 0; i < kf->getNumArgs(); i++) {
          args[i] = executor.getArgumentCell(state, kf, i).value;
        }
        source = SourceBuilder::mockDeterministic(executor.kmodule.get(),
                                                  *kf->function(), args);
        break;
      }
      executor.executeMakeSymbolic(state, mo, source, false);
    } else {
      executor.terminateStateOnUserError(*s,
                                         "Wrong size given to klee_make_mock");
    }
  }
}

void SpecialFunctionHandler::handleMarkGlobal(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_mark_global");

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "mark_global");

  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo = it->first;
    assert(!mo->isLocal);
    mo->isGlobal = true;
  }
}

void SpecialFunctionHandler::handleIsNaN(ExecutionState &state,
                                         KInstruction *target,
                                         std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsNaN");
  ref<Expr> result = IsNaNExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleIsInfinite(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsInfinite");
  ref<Expr> result = IsInfiniteExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleIsNormal(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsNormal");
  ref<Expr> result = IsNormalExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleIsSubnormal(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsSubnormal");
  ref<Expr> result = IsSubnormalExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleGetRoundingMode(
    ExecutionState &state, KInstruction *target,
    [[maybe_unused]] std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 0 &&
         "invalid number of arguments to GetRoundingMode");
  unsigned returnValue = 0;
  switch (state.roundingMode) {
  case llvm::APFloat::rmNearestTiesToEven:
    returnValue = KLEE_FP_RNE;
    break;
  case llvm::APFloat::rmNearestTiesToAway:
    returnValue = KLEE_FP_RNA;
    break;
  case llvm::APFloat::rmTowardPositive:
    returnValue = KLEE_FP_RU;
    break;
  case llvm::APFloat::rmTowardNegative:
    returnValue = KLEE_FP_RD;
    break;
  case llvm::APFloat::rmTowardZero:
    returnValue = KLEE_FP_RZ;
    break;
  default:
    klee_warning_once(nullptr, "Unknown llvm::APFloat rounding mode");
    returnValue = KLEE_FP_UNKNOWN;
  }
  // FIXME: The width is fragile. It's dependent on what the compiler
  // choose to be the width of the enum.
  ref<Expr> result = ConstantExpr::create(returnValue, Expr::Int32);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleSetConcreteRoundingMode(
    ExecutionState &state, KInstruction *, std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to SetRoundingMode");
  llvm::APFloat::roundingMode newRoundingMode =
      llvm::APFloat::rmNearestTiesToEven;
  ref<Expr> roundingModeArg = arguments[0];
  if (!isa<ConstantExpr>(roundingModeArg)) {
    executor.terminateStateOnError(state, "argument should be concrete",
                                   StateTerminationType::User);
    return;
  }
  const ConstantExpr *CE = dyn_cast<ConstantExpr>(roundingModeArg);
  switch (CE->getZExtValue()) {
  case KLEE_FP_RNE:
    newRoundingMode = llvm::APFloat::rmNearestTiesToEven;
    break;
  case KLEE_FP_RNA:
    newRoundingMode = llvm::APFloat::rmNearestTiesToAway;
    break;
  case KLEE_FP_RU:
    newRoundingMode = llvm::APFloat::rmTowardPositive;
    break;
  case KLEE_FP_RD:
    newRoundingMode = llvm::APFloat::rmTowardNegative;
    break;
  case KLEE_FP_RZ:
    newRoundingMode = llvm::APFloat::rmTowardZero;
    break;
  default:
    executor.terminateStateOnError(state, "Invalid rounding mode",
                                   StateTerminationType::User);
    return;
  }
  state.roundingMode = newRoundingMode;
}

void SpecialFunctionHandler::handleSqrt(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to sqrt");
  ref<Expr> result = FSqrtExpr::create(arguments[0], state.roundingMode);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleRint(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to rint");
  ref<Expr> result = FRintExpr::create(arguments[0], state.roundingMode);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleFAbs(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to fabs");
  ref<Expr> result = FAbsExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

#ifdef HAVE_CTYPE_EXTERNALS

void SpecialFunctionHandler::handleCTypeBLoc(ExecutionState &state,
                                             KInstruction *target,
                                             std::vector<ref<Expr>> &) {
  ref<ConstantExpr> pointerValue = Expr::createPointer(
      reinterpret_cast<uint64_t>(executor.c_type_b_loc_addr));
  ref<Expr> result = ConstantPointerExpr::create(pointerValue, pointerValue);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleCTypeToLowerLoc(ExecutionState &state,
                                                   KInstruction *target,
                                                   std::vector<ref<Expr>> &) {
  ref<ConstantExpr> pointerValue = Expr::createPointer(
      reinterpret_cast<uint64_t>(executor.c_type_tolower_addr));
  ref<Expr> result = ConstantPointerExpr::create(pointerValue, pointerValue);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleCTypeToUpperLoc(ExecutionState &state,
                                                   KInstruction *target,
                                                   std::vector<ref<Expr>> &) {
  ref<ConstantExpr> pointerValue = Expr::createPointer(
      reinterpret_cast<uint64_t>(executor.c_type_toupper_addr));
  ref<Expr> result = ConstantPointerExpr::create(pointerValue, pointerValue);
  executor.bindLocal(target, state, result);
}

#endif
