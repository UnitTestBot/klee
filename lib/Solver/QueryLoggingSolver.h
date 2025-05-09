//===-- QueryLoggingSolver.h
//---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_QUERYLOGGINGSOLVER_H
#define KLEE_QUERYLOGGINGSOLVER_H

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/System/Time.h"

#include "llvm/Support/raw_ostream.h"

#include <memory>

using namespace klee;

/// This abstract class represents a solver that is capable of logging
/// queries to a file.
/// Derived classes might specialize this one by providing different formats
/// for the query output.
class QueryLoggingSolver : public SolverImpl {

protected:
  std::unique_ptr<Solver> solver;
  std::unique_ptr<llvm::raw_ostream> os;
  // @brief Buffer used by logBuffer
  std::string BufferString;
  // @brief buffer to store logs before flushing to file
  llvm::raw_string_ostream logBuffer;
  unsigned queryCount;
  time::Span minQueryTimeToLog; // we log to file only those queries which take
                                // longer than the specified time
  bool logTimedOutQueries = false;
  time::Point startTime;
  time::Span lastQueryDuration;
  const std::string queryCommentSign; // sign representing commented lines
                                      // in given a query format

  virtual void startQuery(const Query &query, const char *typeName,
                          const Query *falseQuery = 0,
                          const std::vector<const Array *> *objects = 0);

  virtual void finishQuery(bool success);

  /// flushBuffer - flushes the temporary logs buffer. Depending on threshold
  /// settings, contents of the buffer are either discarded or written to a
  /// file.
  void flushBuffer(void);

  virtual void printQuery(const Query &query, const Query *falseQuery = 0,
                          const std::vector<const Array *> *objects = 0) = 0;
  void flushBufferConditionally(bool writeToFile);

public:
  QueryLoggingSolver(std::unique_ptr<Solver> solver, std::string path,
                     const std::string &commentSign, time::Span queryTimeToLog,
                     bool logTimedOut);

  /// implementation of the SolverImpl interface
  bool computeTruth(const Query &query, bool &isValid);
  bool computeValidity(const Query &query, PartialValidity &result);
  bool computeValue(const Query &query, ref<Expr> &result);
  bool computeInitialValues(
      const Query &query, const std::vector<const Array *> &objects,
      std::vector<SparseStorageImpl<unsigned char>> &values, bool &hasSolution);
  bool check(const Query &query, ref<SolverResponse> &result);
  bool computeValidityCore(const Query &query, ValidityCore &validityCore,
                           bool &isValid);
  SolverRunStatus getOperationStatusCode();
  std::string getConstraintLog(const Query &) final;
  void setCoreSolverLimits(time::Span timeout, unsigned memoryLimit);
  void notifyStateTermination(std::uint32_t id);
};

#endif /* KLEE_QUERYLOGGINGSOLVER_H */
