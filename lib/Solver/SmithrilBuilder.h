//===-- SmithrilBuilder.h --------------------------------------------*- C++
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef SMITHRIL_BUILDER_H
#define SMITHRIL_BUILDER_H

#include "klee/Expr/ArrayExprHash.h"
#include "klee/Expr/ExprHashMap.h"

#include "llvm/ADT/APFloat.h"

namespace smithril {
#include <smithril.h>
}

#include <unordered_map>

namespace klee {

class SmithrilArrayExprHash : public ArrayExprHash<smithril::SmithrilTerm> {
  friend class SmithrilBuilder;

public:
  SmithrilArrayExprHash(){};
  virtual ~SmithrilArrayExprHash();
  void clear();
  void clearUpdates();
};

struct SmithrilTermHash {
  unsigned operator()(const smithril::SmithrilTerm &e) const {
    return (unsigned long)(e._0);
  }
};

struct SmithrilTermCmp {
  bool operator()(const smithril::SmithrilTerm &a,
                  const smithril::SmithrilTerm &b) const {
    return a._0 == b._0;
  }
};
class SmithrilBuilder {
private:
  void FPCastWidthAssert(int *width_out, char const *msg);
  smithril::SmithrilTerm fpToIEEEBV(const smithril::SmithrilTerm &);

protected:
  smithril::SmithrilTerm bvOne(unsigned width);
  smithril::SmithrilTerm bvZero(unsigned width);
  smithril::SmithrilTerm bvMinusOne(unsigned width);
  smithril::SmithrilTerm bvConst32(unsigned width, uint32_t value);
  smithril::SmithrilTerm bvConst64(unsigned width, uint64_t value);
  smithril::SmithrilTerm bvZExtConst(unsigned width, uint64_t value);
  smithril::SmithrilTerm bvSExtConst(unsigned width, uint64_t value);
  smithril::SmithrilTerm bvBoolExtract(smithril::SmithrilTerm expr, int bit);
  smithril::SmithrilTerm bvExtract(smithril::SmithrilTerm expr, unsigned top,
                                   unsigned bottom);
  smithril::SmithrilTerm eqExpr(smithril::SmithrilTerm a,
                                smithril::SmithrilTerm b);

  // logical left and right shift (not arithmetic)
  smithril::SmithrilTerm bvLeftShift(smithril::SmithrilTerm expr,
                                     unsigned shift);
  smithril::SmithrilTerm bvRightShift(smithril::SmithrilTerm expr,
                                      unsigned shift);
  smithril::SmithrilTerm bvVarLeftShift(smithril::SmithrilTerm expr,
                                        smithril::SmithrilTerm shift);
  smithril::SmithrilTerm bvVarRightShift(smithril::SmithrilTerm expr,
                                         smithril::SmithrilTerm shift);
  smithril::SmithrilTerm bvVarArithRightShift(smithril::SmithrilTerm expr,
                                              smithril::SmithrilTerm shift);

  smithril::SmithrilTerm notExpr(smithril::SmithrilTerm expr);
  smithril::SmithrilTerm andExpr(smithril::SmithrilTerm lhs,
                                 smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm orExpr(smithril::SmithrilTerm lhs,
                                smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm iffExpr(smithril::SmithrilTerm lhs,
                                 smithril::SmithrilTerm rhs);

  smithril::SmithrilTerm bvNotExpr(smithril::SmithrilTerm expr);
  smithril::SmithrilTerm bvAndExpr(smithril::SmithrilTerm lhs,
                                   smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm bvOrExpr(smithril::SmithrilTerm lhs,
                                  smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm bvXorExpr(smithril::SmithrilTerm lhs,
                                   smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm bvSignExtend(smithril::SmithrilTerm src,
                                      unsigned width);

  // Array operations
  smithril::SmithrilTerm writeExpr(smithril::SmithrilTerm array,
                                   smithril::SmithrilTerm index,
                                   smithril::SmithrilTerm value);
  smithril::SmithrilTerm readExpr(smithril::SmithrilTerm array,
                                  smithril::SmithrilTerm index);

  // ITE-expression constructor
  smithril::SmithrilTerm iteExpr(smithril::SmithrilTerm condition,
                                 smithril::SmithrilTerm whenTrue,
                                 smithril::SmithrilTerm whenFalse);

  // Bitvector length
  unsigned getBVLength(smithril::SmithrilTerm expr);

  // Bitvector comparison
  smithril::SmithrilTerm bvLtExpr(smithril::SmithrilTerm lhs,
                                  smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm bvLeExpr(smithril::SmithrilTerm lhs,
                                  smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm sbvLtExpr(smithril::SmithrilTerm lhs,
                                   smithril::SmithrilTerm rhs);
  smithril::SmithrilTerm sbvLeExpr(smithril::SmithrilTerm lhs,
                                   smithril::SmithrilTerm rhs);

  smithril::SmithrilTerm
  constructAShrByConstant(smithril::SmithrilTerm expr, unsigned shift,
                          smithril::SmithrilTerm isSigned);

  smithril::SmithrilTerm getInitialArray(const Array *os);
  smithril::SmithrilTerm getArrayForUpdate(const Array *root,
                                           const UpdateNode *un);

  smithril::SmithrilTerm constructActual(ref<Expr> e, int *width_out);
  smithril::SmithrilTerm construct(ref<Expr> e, int *width_out);
  smithril::SmithrilTerm buildArray(const char *name, unsigned indexWidth,
                                    unsigned valueWidth);
  smithril::SmithrilTerm buildConstantArray(const char *name,
                                            unsigned indexWidth,
                                            unsigned valueWidth,
                                            unsigned value);

  smithril::SmithrilSort getBoolSort();
  smithril::SmithrilSort getBvSort(unsigned width);
  smithril::SmithrilSort getArraySort(smithril::SmithrilSort domainSort,
                                      smithril::SmithrilSort rangeSort);

  std::pair<unsigned, unsigned> getFloatSortFromBitWidth(unsigned bitWidth);

  // Float casts
  smithril::SmithrilTerm castToFloat(const smithril::SmithrilTerm &e);
  smithril::SmithrilTerm castToBitVector(const smithril::SmithrilTerm &e);

  smithril::RoundingMode getRoundingModeSort(llvm::APFloat::roundingMode rm);
  smithril::SmithrilTerm
  getx87FP80ExplicitSignificandIntegerBit(const smithril::SmithrilTerm &e);

  ExprHashMap<std::pair<smithril::SmithrilTerm, unsigned>> constructed;
  SmithrilArrayExprHash _arr_hash;
  bool autoClearConstructCache;

public:
  smithril::SmithrilContext ctx;
  std::unordered_map<const Array *, std::vector<smithril::SmithrilTerm>>
      constant_array_assertions;
  // These are additional constraints that are generated during the
  // translation to Smithril's constraint language. Clients should assert
  // these.
  std::vector<smithril::SmithrilTerm> sideConstraints;

  SmithrilBuilder(bool autoClearConstructCache);
  ~SmithrilBuilder();

  smithril::SmithrilTerm getTrue();
  smithril::SmithrilTerm getFalse();
  smithril::SmithrilTerm buildFreshBoolConst();
  smithril::SmithrilTerm getInitialRead(const Array *os, unsigned index);

  smithril::SmithrilTerm construct(ref<Expr> e) {
    smithril::SmithrilTerm res = construct(std::move(e), nullptr);
    if (autoClearConstructCache)
      clearConstructCache();
    return res;
  }
  void clearConstructCache() { constructed.clear(); }
  void clearSideConstraints() { sideConstraints.clear(); }
};
} // namespace klee

#endif
