//===-- ExprTest.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/ArrayExprOptimizer.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/SourceBuilder.h"

#include "llvm/Support/CommandLine.h"

using namespace klee;
namespace klee {
extern llvm::cl::opt<ArrayOptimizationType> OptimizeArray;
}

namespace {

ref<Expr> getConstant(int value, Expr::Width width) {
  int64_t ext = value;
  uint64_t trunc = ext & (((uint64_t)-1LL) >> (64 - width));
  return ConstantExpr::create(trunc, width);
}

TEST(ArrayExprTest, HashCollisions) {
  klee::OptimizeArray = ALL;
  SparseStorageImpl<ref<ConstantExpr>> constVals(
      ConstantExpr::create(5, Expr::Int8));
  const Array *array = Array::create(
      ConstantExpr::create(256, sizeof(uint64_t) * CHAR_BIT),
      SourceBuilder::constant(constVals.clone()), Expr::Int32, Expr::Int8);
  const Array *symArray =
      Array::create(ConstantExpr::create(4, sizeof(uint64_t) * CHAR_BIT),
                    SourceBuilder::makeSymbolic("symIdx", 0));
  ref<Expr> symIdx = Expr::createTempRead(symArray, Expr::Int32);
  UpdateList ul(array, 0);
  ul.extend(getConstant(3, Expr::Int32), getConstant(11, Expr::Int8));
  ref<Expr> firstRead = ReadExpr::create(ul, symIdx);
  ul.extend(getConstant(6, Expr::Int32), getConstant(42, Expr::Int8));
  ul.extend(getConstant(6, Expr::Int32), getConstant(42, Expr::Int8));
  ref<Expr> updatedRead = ReadExpr::create(ul, symIdx);

  // This test requires hash collision and should be updated if the hash
  // function changes
  ASSERT_NE(updatedRead, firstRead);
  ASSERT_EQ(updatedRead->hash(), firstRead->hash());

  SparseStorageImpl<unsigned char> value({6, 0, 0, 0});
  std::vector<SparseStorageImpl<unsigned char>> values = {value};
  std::vector<const Array *> assignmentArrays = {symArray};
  auto a = std::make_unique<Assignment>(assignmentArrays, values);

  EXPECT_NE(a->evaluate(updatedRead), a->evaluate(firstRead));
  EXPECT_EQ(a->evaluate(updatedRead), getConstant(42, Expr::Int8));
  EXPECT_EQ(a->evaluate(firstRead), getConstant(5, Expr::Int8));

  ExprOptimizer opt;
  auto oFirstRead = opt.optimizeExpr(firstRead, true);
  auto oUpdatedRead = opt.optimizeExpr(updatedRead, true);
  EXPECT_NE(oFirstRead, firstRead);
  EXPECT_NE(updatedRead, oUpdatedRead);

  EXPECT_NE(a->evaluate(oUpdatedRead), a->evaluate(oFirstRead));
  EXPECT_EQ(a->evaluate(oUpdatedRead), getConstant(42, Expr::Int8));
  EXPECT_EQ(a->evaluate(oFirstRead), getConstant(5, Expr::Int8));
}
} // namespace
