//===-- SmithrilBuilder.cpp ---------------------------------*- C++ -*-====//
//-*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"

#ifdef ENABLE_SMITHRIL

#include "SmithrilBuilder.h"
#include "SmithrilHashConfig.h"
#include "klee/ADT/Bits.h"

#include "klee/Expr/Expr.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverStats.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringExtras.h"

using namespace smithril;

namespace klee {

SmithrilArrayExprHash::~SmithrilArrayExprHash() {}

void SmithrilArrayExprHash::clear() {
  _update_node_hash.clear();
  _array_hash.clear();
}

void SmithrilArrayExprHash::clearUpdates() { _update_node_hash.clear(); }

SmithrilBuilder::SmithrilBuilder(bool autoClearConstructCache)
    : autoClearConstructCache(autoClearConstructCache) {
  ctx = smithril_new_context();
}

SmithrilBuilder::~SmithrilBuilder() {
  _arr_hash.clearUpdates();
  clearSideConstraints();
}

SmithrilSort SmithrilBuilder::getBoolSort() {
  // FIXME: cache these
  return smithril_mk_bool_sort(ctx);
}

SmithrilSort SmithrilBuilder::getBvSort(unsigned width) {
  // FIXME: cache these
  return smithril_mk_bv_sort(ctx, width);
}

SmithrilSort SmithrilBuilder::getArraySort(SmithrilSort domainSort,
                                           SmithrilSort rangeSort) {
  // FIXME: cache these
  return smithril_mk_array_sort(ctx, domainSort, rangeSort);
}

SmithrilTerm SmithrilBuilder::buildFreshBoolConst() {
  return smithril_mk_fresh_smt_symbol(ctx, getBoolSort());
}

SmithrilTerm SmithrilBuilder::buildArray(const char *name, unsigned indexWidth,
                                         unsigned valueWidth) {
  SmithrilSort domainSort = getBvSort(indexWidth);
  SmithrilSort rangeSort = getBvSort(valueWidth);
  SmithrilSort t = getArraySort(domainSort, rangeSort);
  return smithril_mk_smt_symbol(ctx, name,
                                t); // mk_const(t, std::string(name));
}

SmithrilTerm SmithrilBuilder::buildConstantArray(const char *,
                                                 unsigned indexWidth,
                                                 unsigned valueWidth,
                                                 unsigned value) {
  SmithrilSort domainSort = getBvSort(indexWidth);
  SmithrilSort rangeSort = getBvSort(valueWidth);
  return smithril_mk_smt_const_symbol(ctx, bvConst32(valueWidth, value),
                                      getArraySort(domainSort, rangeSort));
}

SmithrilTerm SmithrilBuilder::getTrue() {
  return smithril_mk_smt_bool(ctx, true);
}

SmithrilTerm SmithrilBuilder::getFalse() {
  return smithril_mk_smt_bool(ctx, false);
}

SmithrilTerm SmithrilBuilder::bvOne(unsigned width) {
  return bvZExtConst(width, 1);
}
SmithrilTerm SmithrilBuilder::bvZero(unsigned width) {
  return bvZExtConst(width, 0);
}
SmithrilTerm SmithrilBuilder::bvMinusOne(unsigned width) {
  return bvZExtConst(width, (uint64_t)-1);
}
SmithrilTerm SmithrilBuilder::bvConst32(unsigned width, uint32_t value) {
  if (width < 32) {
    value &= ((1 << width) - 1);
  }
  return smithril_mk_bv_value_uint64(ctx, getBvSort(width), value);
}
SmithrilTerm SmithrilBuilder::bvConst64(unsigned width, uint64_t value) {
  if (width < 64) {
    value &= ((uint64_t(1) << width) - 1);
  }
  return smithril_mk_bv_value_uint64(ctx, getBvSort(width), value);
}
SmithrilTerm SmithrilBuilder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64) {
    return bvConst64(width, value);
  }
  SmithrilTerm expr = bvConst64(64, value);
  SmithrilTerm zero = bvConst64(64, 0);
  for (width -= 64; width > 64; width -= 64) {
    expr = smithril_mk_concat(ctx, zero, expr);
  }
  return smithril_mk_concat(ctx, bvConst64(width, 0), expr);
}

SmithrilTerm SmithrilBuilder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64) {
    return bvConst64(width, value);
  }

  if (value >> 63) {
    return smithril_mk_concat(ctx, bvMinusOne(width - 64),
                              bvConst64(64, value));
  }
  return smithril_mk_concat(ctx, bvZero(width - 64), bvConst64(64, value));
}

SmithrilTerm SmithrilBuilder::bvBoolExtract(SmithrilTerm expr, int bit) {
  return smithril_mk_eq(ctx, bvExtract(expr, bit, bit), bvOne(1));
}

SmithrilTerm SmithrilBuilder::bvExtract(SmithrilTerm expr, unsigned top,
                                        unsigned bottom) {
  return smithril_mk_extract(ctx, top, bottom, castToBitVector(expr));
}

SmithrilTerm SmithrilBuilder::eqExpr(SmithrilTerm a, SmithrilTerm b) {
  // Handle implicit bitvector/float coercision
  SmithrilSort aSort = smithril_get_sort(ctx, a);
  SmithrilSort bSort = smithril_get_sort(ctx, b);

  if (smithril_get_sort_kind(aSort) == SortKind::Bv &&
      smithril_get_sort_kind(bSort) == SortKind::Fp) {
    // Coerce `b` to be a bitvector
    b = castToBitVector(b);
  }

  if (smithril_get_sort_kind(aSort) == SortKind::Fp &&
      smithril_get_sort_kind(bSort) == SortKind::Bv) {
    // Coerce `a` to be a bitvector
    a = castToBitVector(a);
  }
  return smithril_mk_eq(ctx, a, b);
}

// logical right shift
SmithrilTerm SmithrilBuilder::bvRightShift(SmithrilTerm expr, unsigned shift) {
  SmithrilTerm exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return smithril_mk_bvlshr(ctx, exprAsBv, bvConst32(width, shift));
  }
}

// logical left shift
SmithrilTerm SmithrilBuilder::bvLeftShift(SmithrilTerm expr, unsigned shift) {
  SmithrilTerm exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return smithril_mk_bvshl(ctx, exprAsBv, bvConst32(width, shift));
  }
}

// left shift by a variable amount on an expression of the specified width
SmithrilTerm SmithrilBuilder::bvVarLeftShift(SmithrilTerm expr,
                                             SmithrilTerm shift) {
  SmithrilTerm exprAsBv = castToBitVector(expr);
  SmithrilTerm shiftAsBv = castToBitVector(shift);

  unsigned width = getBVLength(exprAsBv);
  SmithrilTerm res = smithril_mk_bvshl(ctx, exprAsBv, shiftAsBv);

  // If overshifting, shift to zero
  SmithrilTerm ex =
      bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified
// width
SmithrilTerm SmithrilBuilder::bvVarRightShift(SmithrilTerm expr,
                                              SmithrilTerm shift) {
  SmithrilTerm exprAsBv = castToBitVector(expr);
  SmithrilTerm shiftAsBv = castToBitVector(shift);

  unsigned width = getBVLength(exprAsBv);
  SmithrilTerm res = smithril_mk_bvlshr(ctx, exprAsBv, shiftAsBv);

  // If overshifting, shift to zero
  SmithrilTerm ex =
      bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the
// specified width
SmithrilTerm SmithrilBuilder::bvVarArithRightShift(SmithrilTerm expr,
                                                   SmithrilTerm shift) {
  SmithrilTerm exprAsBv = castToBitVector(expr);
  SmithrilTerm shiftAsBv = castToBitVector(shift);

  unsigned width = getBVLength(exprAsBv);

  SmithrilTerm res = smithril_mk_bvashr(ctx, exprAsBv, shiftAsBv);

  // If overshifting, shift to zero
  SmithrilTerm ex =
      bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

SmithrilTerm SmithrilBuilder::notExpr(SmithrilTerm expr) {
  return smithril_mk_not(ctx, expr);
}
SmithrilTerm SmithrilBuilder::andExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_and(ctx, lhs, rhs);
}
SmithrilTerm SmithrilBuilder::orExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_or(ctx, lhs, rhs);
}
SmithrilTerm SmithrilBuilder::iffExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_iff(ctx, lhs, rhs);
}

SmithrilTerm SmithrilBuilder::bvNotExpr(SmithrilTerm expr) {
  return smithril_mk_bvnot(ctx, castToBitVector(expr));
}

SmithrilTerm SmithrilBuilder::bvAndExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvand(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::bvOrExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvor(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::bvXorExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvxor(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::bvSignExtend(SmithrilTerm src, unsigned width) {
  SmithrilTerm srcAsBv = castToBitVector(src);
  SmithrilSort srcAsBvSort = smithril_get_sort(ctx, srcAsBv);
  unsigned src_width = smithril_get_bv_sort_size(srcAsBvSort);
  assert(src_width <= width && "attempted to extend longer data");
  return smithril_mk_sign_extend(ctx, width - src_width, srcAsBv);
}

SmithrilTerm SmithrilBuilder::writeExpr(SmithrilTerm array, SmithrilTerm index,
                                        SmithrilTerm value) {
  return smithril_mk_store(ctx, array, index, value);
}

SmithrilTerm SmithrilBuilder::readExpr(SmithrilTerm array, SmithrilTerm index) {
  return smithril_mk_select(ctx, array, index);
}

unsigned SmithrilBuilder::getBVLength(SmithrilTerm expr) {
  SmithrilSort exprSort = smithril_get_sort(ctx, expr);
  return smithril_get_bv_sort_size(exprSort);
}

SmithrilTerm SmithrilBuilder::iteExpr(SmithrilTerm condition,
                                      SmithrilTerm whenTrue,
                                      SmithrilTerm whenFalse) {
  // Handle implicit bitvector/float coercision
  SmithrilSort whenTrueSort = smithril_get_sort(ctx, whenTrue);
  SmithrilSort whenFalseSort = smithril_get_sort(ctx, whenFalse);

  if (smithril_get_sort_kind(whenTrueSort) == SortKind::Bv &&
      smithril_get_sort_kind(whenFalseSort) == SortKind::Fp) {
    // Coerce `whenFalse` to be a bitvector
    whenFalse = castToBitVector(whenFalse);
  }

  if (smithril_get_sort_kind(whenTrueSort) == SortKind::Fp &&
      smithril_get_sort_kind(whenFalseSort) == SortKind::Bv) {
    // Coerce `whenTrue` to be a bitvector
    whenTrue = castToBitVector(whenTrue);
  }
  return smithril_mk_ite(ctx, condition, whenTrue, whenFalse);
}

SmithrilTerm SmithrilBuilder::bvLtExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvult(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::bvLeExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvule(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::sbvLtExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvslt(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::sbvLeExpr(SmithrilTerm lhs, SmithrilTerm rhs) {
  return smithril_mk_bvsle(ctx, castToBitVector(lhs), castToBitVector(rhs));
}

SmithrilTerm SmithrilBuilder::constructAShrByConstant(SmithrilTerm expr,
                                                      unsigned shift,
                                                      SmithrilTerm isSigned) {
  SmithrilTerm exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return exprAsBv;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    // FIXME: Is this really the best way to interact with Smithril?
    SmithrilTerm signed_term = smithril_mk_concat(
        ctx, bvMinusOne(shift), bvExtract(exprAsBv, width - 1, shift));
    SmithrilTerm unsigned_term = bvRightShift(exprAsBv, shift);

    return smithril_mk_ite(ctx, isSigned, signed_term, unsigned_term);
  }
}

SmithrilTerm SmithrilBuilder::getInitialArray(const Array *root) {
  assert(root);
  SmithrilTerm array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);

  if (!hashed) {
    // Unique arrays by name, so we make sure the name is unique by
    // using the size of the array hash as a counter.
    std::string unique_id = llvm::utostr(_arr_hash._array_hash.size());
    std::string unique_name = root->getIdentifier() + unique_id;

    auto source = dyn_cast<ConstantSource>(root->source);
    auto value = (source ? source->constantValues->defaultV() : nullptr);
    if (source) {
      assert(value);
    }

    if (source && !isa<ConstantExpr>(root->size)) {
      array_expr = buildConstantArray(unique_name.c_str(), root->getDomain(),
                                      root->getRange(), value->getZExtValue(8));
    } else {
      array_expr =
          buildArray(unique_name.c_str(), root->getDomain(), root->getRange());
    }

    if (source) {
      if (auto constSize = dyn_cast<ConstantExpr>(root->size)) {
        std::vector<SmithrilTerm> array_assertions;
        for (size_t i = 0; i < constSize->getZExtValue(); i++) {
          auto value = source->constantValues->load(i);
          // construct(= (select i root) root->value[i]) to be asserted in
          // SmithrilSolver.cpp
          int width_out;
          SmithrilTerm array_value = construct(value, &width_out);
          assert(width_out == (int)root->getRange() &&
                 "Value doesn't match root range");
          array_assertions.push_back(
              eqExpr(readExpr(array_expr, bvConst32(root->getDomain(), i)),
                     array_value));
        }
        constant_array_assertions[root] = std::move(array_assertions);
      } else {
        for (const auto &[index, value] : source->constantValues->storage()) {
          int width_out;
          SmithrilTerm array_value = construct(value, &width_out);
          assert(width_out == (int)root->getRange() &&
                 "Value doesn't match root range");
          array_expr = writeExpr(
              array_expr, bvConst32(root->getDomain(), index), array_value);
        }
      }
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }

  return array_expr;
}

SmithrilTerm SmithrilBuilder::getInitialRead(const Array *root,
                                             unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

SmithrilTerm SmithrilBuilder::getArrayForUpdate(const Array *root,
                                                const UpdateNode *un) {
  // Iterate over the update nodes, until we find a cached version of the
  // node, or no more update nodes remain
  SmithrilTerm un_expr;
  std::vector<const UpdateNode *> update_nodes;
  for (; un && !_arr_hash.lookupUpdateNodeExpr(un, un_expr);
       un = un->next.get()) {
    update_nodes.push_back(un);
  }
  if (!un) {
    un_expr = getInitialArray(root);
  }
  // `un_expr` now holds an expression for the array - either from cache or by
  // virtue of being the initial array expression

  // Create and cache solver expressions based on the update nodes starting
  // from the oldest
  for (const auto &un :
       llvm::make_range(update_nodes.crbegin(), update_nodes.crend())) {
    un_expr =
        writeExpr(un_expr, construct(un->index, 0), construct(un->value, 0));

    _arr_hash.hashUpdateNodeExpr(un, un_expr);
  }

  return un_expr;
}

SmithrilTerm SmithrilBuilder::construct(ref<Expr> e, int *width_out) {
  if (!SmithrilHashConfig::UseConstructHashSmithril || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap<std::pair<SmithrilTerm, unsigned>>::iterator it =
        constructed.find(e);
    if (it != constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out)
        width_out = &width;
      SmithrilTerm res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}

void SmithrilBuilder::FPCastWidthAssert([[maybe_unused]] int *width_out,
                                        [[maybe_unused]] char const *msg) {
  assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
             &(llvm::APFloat::Bogus()) &&
         msg);
}

/** if *width_out!=1 then result is a bitvector,
otherwise it is a bool */
SmithrilTerm SmithrilBuilder::constructActual(ref<Expr> e, int *width_out) {

  int width;
  if (!width_out)
    width_out = &width;
  ++stats::queryConstructs;
  switch (e->getKind()) {
  case Expr::Pointer:
  case Expr::ConstantPointer: {
    assert(0 && "unreachable");
  }

  case Expr::Constant: {
    ConstantExpr *CE = cast<ConstantExpr>(e);
    *width_out = CE->getWidth();

    // Coerce to bool if necessary.
    if (*width_out == 1)
      return CE->isTrue() ? getTrue() : getFalse();

    SmithrilTerm Res;
    if (*width_out <= 32) {
      // Fast path.
      Res = bvConst32(*width_out, CE->getZExtValue(32));
    } else if (*width_out <= 64) {
      // Fast path.
      Res = bvConst64(*width_out, CE->getZExtValue());
    } else {
      ref<ConstantExpr> Tmp = CE;
      Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
      while (Tmp->getWidth() > 64) {
        Tmp = Tmp->Extract(64, Tmp->getWidth() - 64);
        unsigned Width = std::min(64U, Tmp->getWidth());
        Res = smithril_mk_concat(
            ctx, bvConst64(Width, Tmp->Extract(0, Width)->getZExtValue()), Res);
      }
    }
    // Coerce to float if necesary
    if (CE->isFloat()) {
      Res = castToFloat(Res);
    }
    return Res;
  }

    // Special
  case Expr::NotOptimized: {
    NotOptimizedExpr *noe = cast<NotOptimizedExpr>(e);
    return construct(noe->src, width_out);
  }

  case Expr::Read: {
    ReadExpr *re = cast<ReadExpr>(e);
    assert(re && re->updates.root);
    *width_out = re->updates.root->getRange();
    return readExpr(getArrayForUpdate(re->updates.root, re->updates.head.get()),
                    construct(re->index, 0));
  }

  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    SmithrilTerm cond = construct(se->cond, 0);
    SmithrilTerm tExpr = construct(se->trueExpr, width_out);
    SmithrilTerm fExpr = construct(se->falseExpr, width_out);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    SmithrilTerm res = construct(ce->getKid(numKids - 1), 0);
    for (int i = numKids - 2; i >= 0; i--) {
      res = smithril_mk_concat(ctx, construct(ce->getKid(i), 0), res);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    SmithrilTerm src = construct(ee->expr, width_out);
    *width_out = ee->getWidth();
    if (*width_out == 1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return bvExtract(src, ee->offset + *width_out - 1, ee->offset);
    }
  }

    // Casting

  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    SmithrilTerm src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      assert(*width_out > srcWidth && "Invalid width_out");
      return smithril_mk_concat(ctx, bvZero(*width_out - srcWidth),
                                castToBitVector(src));
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    SmithrilTerm src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvMinusOne(*width_out), bvZero(*width_out));
    } else {
      return bvSignExtend(src, *width_out);
    }
  }

  case Expr::FPExt: {
    int srcWidth;
    FPExtExpr *ce = cast<FPExtExpr>(e);
    SmithrilTerm src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPExt width");
    assert(*width_out >= srcWidth && "Invalid FPExt");
    // Just use any arounding mode here as we are extending
    auto out_widths = getFloatSortFromBitWidth(*width_out);

    return smithril_mk_fp_to_fp_from_fp(
        ctx, getRoundingModeSort(llvm::APFloat::rmNearestTiesToEven), src,
        out_widths.first, out_widths.second);
  }

  case Expr::FPTrunc: {
    int srcWidth;
    FPTruncExpr *ce = cast<FPTruncExpr>(e);
    SmithrilTerm src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPTrunc width");
    assert(*width_out <= srcWidth && "Invalid FPTrunc");

    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return smithril_mk_fp_to_fp_from_fp(
        ctx, getRoundingModeSort(llvm::APFloat::rmNearestTiesToEven), src,
        out_widths.first, out_widths.second);
  }

  case Expr::FPToUI: {
    int srcWidth;
    FPToUIExpr *ce = cast<FPToUIExpr>(e);
    SmithrilTerm src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPToUI width");
    return smithril_mk_fp_to_ubv(ctx, getRoundingModeSort(ce->roundingMode),
                                 src, ce->getWidth());
  }

  case Expr::FPToSI: {
    int srcWidth;
    FPToSIExpr *ce = cast<FPToSIExpr>(e);
    SmithrilTerm src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid FPToSI width");
    return smithril_mk_fp_to_sbv(ctx, getRoundingModeSort(ce->roundingMode),
                                 src, ce->getWidth());
  }

  case Expr::UIToFP: {
    int srcWidth;
    UIToFPExpr *ce = cast<UIToFPExpr>(e);
    SmithrilTerm src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid UIToFP width");

    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return smithril_mk_fp_to_fp_from_ubv(
        ctx, getRoundingModeSort(ce->roundingMode), src, out_widths.first,
        out_widths.second);
  }

  case Expr::SIToFP: {
    int srcWidth;
    SIToFPExpr *ce = cast<SIToFPExpr>(e);
    SmithrilTerm src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    FPCastWidthAssert(width_out, "Invalid SIToFP width");

    auto out_widths = getFloatSortFromBitWidth(*width_out);
    return smithril_mk_fp_to_fp_from_sbv(
        ctx, getRoundingModeSort(ce->roundingMode), src, out_widths.first,
        out_widths.second);
  }

    // Arithmetic
  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    SmithrilTerm left = castToBitVector(construct(ae->left, width_out));
    SmithrilTerm right = castToBitVector(construct(ae->right, width_out));
    assert(*width_out != 1 && "uncanonicalized add");
    SmithrilTerm result = smithril_mk_bvadd(ctx, left, right);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    SmithrilTerm left = castToBitVector(construct(se->left, width_out));
    SmithrilTerm right = castToBitVector(construct(se->right, width_out));
    assert(*width_out != 1 && "uncanonicalized sub");
    SmithrilTerm result = smithril_mk_bvsub(ctx, left, right);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    SmithrilTerm right = castToBitVector(construct(me->right, width_out));
    assert(*width_out != 1 && "uncanonicalized mul");
    SmithrilTerm left = castToBitVector(construct(me->left, width_out));
    SmithrilTerm result = smithril_mk_bvmul(ctx, left, right);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    SmithrilTerm left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized udiv");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();
        if (bits64::isPowerOfTwo(divisor))
          return bvRightShift(left, bits64::indexOfSingleBit(divisor));
      }
    }

    SmithrilTerm right = castToBitVector(construct(de->right, width_out));
    SmithrilTerm result = smithril_mk_bvudiv(ctx, left, right);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    SmithrilTerm left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized sdiv");
    SmithrilTerm right = castToBitVector(construct(de->right, width_out));
    SmithrilTerm result = smithril_mk_bvsdiv(ctx, left, right);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    SmithrilTerm left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized urem");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          int bits = bits64::indexOfSingleBit(divisor);
          assert(bits >= 0 && "bit index cannot be negative");
          assert(bits64::indexOfSingleBit(divisor) < INT32_MAX);

          // special case for modding by 1 or else we bvExtract -1:0
          if (bits == 0) {
            return bvZero(*width_out);
          } else {
            assert(*width_out > bits && "invalid width_out");
            return smithril_mk_concat(ctx, bvZero(*width_out - bits),
                                      bvExtract(left, bits - 1, 0));
          }
        }
      }
    }

    SmithrilTerm right = castToBitVector(construct(de->right, width_out));
    SmithrilTerm result = smithril_mk_bvurem(ctx, left, right);

    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    SmithrilTerm left = castToBitVector(construct(de->left, width_out));
    SmithrilTerm right = castToBitVector(construct(de->right, width_out));
    assert(*width_out != 1 && "uncanonicalized srem");
    SmithrilTerm result = smithril_mk_bvsrem(ctx, left, right);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

    // Bitwise
  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    SmithrilTerm expr = construct(ne->expr, width_out);
    if (*width_out == 1) {
      return notExpr(expr);
    } else {
      return bvNotExpr(expr);
    }
  }

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    SmithrilTerm left = construct(ae->left, width_out);
    SmithrilTerm right = construct(ae->right, width_out);
    if (*width_out == 1) {
      return andExpr(left, right);
    } else {
      return bvAndExpr(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    SmithrilTerm left = construct(oe->left, width_out);
    SmithrilTerm right = construct(oe->right, width_out);
    if (*width_out == 1) {
      return orExpr(left, right);
    } else {
      return bvOrExpr(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    SmithrilTerm left = construct(xe->left, width_out);
    SmithrilTerm right = construct(xe->right, width_out);

    if (*width_out == 1) {
      // XXX check for most efficient?
      return iteExpr(left, SmithrilTerm(notExpr(right)), right);
    } else {
      return bvXorExpr(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    SmithrilTerm left = construct(se->left, width_out);
    assert(*width_out != 1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      SmithrilTerm amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift(left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    SmithrilTerm left = construct(lse->left, width_out);
    assert(*width_out != 1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      SmithrilTerm amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift(left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    SmithrilTerm left = castToBitVector(construct(ase->left, width_out));
    assert(*width_out != 1 && "uncanonicalized ashr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned)CE->getLimitedValue();
      SmithrilTerm signedBool = bvBoolExtract(left, *width_out - 1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      SmithrilTerm amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift(left, amount);
    }
  }

    // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    SmithrilTerm left = construct(ee->left, width_out);
    SmithrilTerm right = construct(ee->right, width_out);
    if (*width_out == 1) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left)) {
        if (CE->isTrue())
          return right;
        return notExpr(right);
      } else {
        return iffExpr(left, right);
      }
    } else {
      *width_out = 1;
      return eqExpr(left, right);
    }
  }

  case Expr::Ult: {
    UltExpr *ue = cast<UltExpr>(e);
    SmithrilTerm left = construct(ue->left, width_out);
    SmithrilTerm right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ult");
    *width_out = 1;
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    SmithrilTerm left = construct(ue->left, width_out);
    SmithrilTerm right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ule");
    *width_out = 1;
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    SmithrilTerm left = construct(se->left, width_out);
    SmithrilTerm right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized slt");
    *width_out = 1;
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    SmithrilTerm left = construct(se->left, width_out);
    SmithrilTerm right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized sle");
    *width_out = 1;
    return sbvLeExpr(left, right);
  }

  case Expr::FOEq: {
    FOEqExpr *fcmp = cast<FOEqExpr>(e);
    SmithrilTerm left = castToFloat(construct(fcmp->left, width_out));
    SmithrilTerm right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return smithril_mk_fp_eq(ctx, left, right);
  }

  case Expr::FOLt: {
    FOLtExpr *fcmp = cast<FOLtExpr>(e);
    SmithrilTerm left = castToFloat(construct(fcmp->left, width_out));
    SmithrilTerm right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return smithril_mk_fp_lt(ctx, left, right);
  }

  case Expr::FOLe: {
    FOLeExpr *fcmp = cast<FOLeExpr>(e);
    SmithrilTerm left = castToFloat(construct(fcmp->left, width_out));
    SmithrilTerm right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return smithril_mk_fp_leq(ctx, left,
                              right); // mixa117 RoundingMode does not matter
  }

  case Expr::FOGt: {
    FOGtExpr *fcmp = cast<FOGtExpr>(e);
    SmithrilTerm left = castToFloat(construct(fcmp->left, width_out));
    SmithrilTerm right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return smithril_mk_fp_gt(ctx, left,
                             right); // mixa117 RoundingMode does not matter
  }

  case Expr::FOGe: {
    FOGeExpr *fcmp = cast<FOGeExpr>(e);
    SmithrilTerm left = castToFloat(construct(fcmp->left, width_out));
    SmithrilTerm right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return smithril_mk_fp_geq(ctx, left,
                              right); // mixa117 RoundingMode does not matter
  }

  case Expr::IsNaN: {
    IsNaNExpr *ine = cast<IsNaNExpr>(e);
    SmithrilTerm arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return smithril_fp_is_nan(ctx, arg);
  }

  case Expr::IsInfinite: {
    IsInfiniteExpr *iie = cast<IsInfiniteExpr>(e);
    SmithrilTerm arg = castToFloat(construct(iie->expr, width_out));
    *width_out = 1;
    return smithril_fp_is_inf(ctx, arg);
  }

  case Expr::IsNormal: {
    IsNormalExpr *ine = cast<IsNormalExpr>(e);
    SmithrilTerm arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return smithril_fp_is_normal(ctx, arg);
  }

  case Expr::IsSubnormal: {
    IsSubnormalExpr *ise = cast<IsSubnormalExpr>(e);
    SmithrilTerm arg = castToFloat(construct(ise->expr, width_out));
    *width_out = 1;
    return smithril_fp_is_subnormal(ctx, arg);
  }

  case Expr::FAdd: {
    FAddExpr *fadd = cast<FAddExpr>(e);
    SmithrilTerm left = castToFloat(construct(fadd->left, width_out));
    SmithrilTerm right = castToFloat(construct(fadd->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FAdd");
    return smithril_mk_fp_add(ctx, getRoundingModeSort(fadd->roundingMode),
                              left, right);
  }

  case Expr::FSub: {
    FSubExpr *fsub = cast<FSubExpr>(e);
    SmithrilTerm left = castToFloat(construct(fsub->left, width_out));
    SmithrilTerm right = castToFloat(construct(fsub->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FSub");
    return smithril_mk_fp_sub(ctx, getRoundingModeSort(fsub->roundingMode),
                              left, right);
  }

  case Expr::FMul: {
    FMulExpr *fmul = cast<FMulExpr>(e);
    SmithrilTerm left = castToFloat(construct(fmul->left, width_out));
    SmithrilTerm right = castToFloat(construct(fmul->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMul");
    return smithril_mk_fp_mul(ctx, getRoundingModeSort(fmul->roundingMode),
                              left, right);
  }

  case Expr::FDiv: {
    FDivExpr *fdiv = cast<FDivExpr>(e);
    SmithrilTerm left = castToFloat(construct(fdiv->left, width_out));
    SmithrilTerm right = castToFloat(construct(fdiv->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FDiv");
    return smithril_mk_fp_div(ctx, getRoundingModeSort(fdiv->roundingMode),
                              left, right);
  }
  case Expr::FRem: {
    FRemExpr *frem = cast<FRemExpr>(e);
    SmithrilTerm left = castToFloat(construct(frem->left, width_out));
    SmithrilTerm right = castToFloat(construct(frem->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FRem");
    return smithril_mk_fp_rem(ctx, left,
                              right); // mixa117 RoundingMode does not matter
  }

  case Expr::FMax: {
    FMaxExpr *fmax = cast<FMaxExpr>(e);
    SmithrilTerm left = castToFloat(construct(fmax->left, width_out));
    SmithrilTerm right = castToFloat(construct(fmax->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMax");
    return smithril_mk_fp_max(ctx, left,
                              right); // mixa117 RoundingMode does not matter
  }

  case Expr::FMin: {
    FMinExpr *fmin = cast<FMinExpr>(e);
    SmithrilTerm left = castToFloat(construct(fmin->left, width_out));
    SmithrilTerm right = castToFloat(construct(fmin->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMin");
    return smithril_mk_fp_min(ctx, left,
                              right); // mixa117 RoundingMode does not matter
  }

  case Expr::FSqrt: {
    FSqrtExpr *fsqrt = cast<FSqrtExpr>(e);
    SmithrilTerm arg = castToFloat(construct(fsqrt->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return smithril_mk_fp_sqrt(ctx, getRoundingModeSort(fsqrt->roundingMode),
                               arg);
  }
  case Expr::FRint: {
    FRintExpr *frint = cast<FRintExpr>(e);
    SmithrilTerm arg = castToFloat(construct(frint->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return smithril_mk_fp_rti(ctx, getRoundingModeSort(frint->roundingMode),
                              arg);
  }

  case Expr::FAbs: {
    FAbsExpr *fabsExpr = cast<FAbsExpr>(e);
    SmithrilTerm arg = castToFloat(construct(fabsExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FAbs");
    return smithril_mk_fp_abs(ctx,
                              arg); // mixa117 RoundingMode does not matter
  }

  case Expr::FNeg: {
    FNegExpr *fnegExpr = cast<FNegExpr>(e);
    SmithrilTerm arg = castToFloat(construct(fnegExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FNeg");
    return smithril_mk_fp_neg(ctx,
                              arg); // mixa117 RoundingMode does not matter
  }

// unused due to canonicalization
#if 0
        case Expr::Ne:
case Expr::Ugt:
case Expr::Uge:
case Expr::Sgt:
case Expr::Sge:
#endif

  default:
    assert(0 && "unhandled Expr type");
    return getTrue();
  }
}

SmithrilTerm SmithrilBuilder::fpToIEEEBV(const SmithrilTerm &fp) {
  SmithrilTerm signBit = smithril_mk_fresh_smt_symbol(ctx, getBvSort(1));
  SmithrilTerm exponentBits = smithril_mk_fresh_smt_symbol(
      ctx, getBvSort(smithril_fp_get_bv_exp_size(fp)));
  SmithrilTerm significandBits = smithril_mk_fresh_smt_symbol(
      ctx, getBvSort(smithril_fp_get_bv_sig_size(fp) - 1));

  SmithrilTerm floatTerm =
      smithril_mk_fp(ctx, signBit, exponentBits, significandBits);
  sideConstraints.push_back(smithril_mk_eq(ctx, fp, floatTerm));
  SmithrilTerm ieeeBits = smithril_mk_concat(ctx, signBit, exponentBits);
  ieeeBits = smithril_mk_concat(ctx, ieeeBits, significandBits);

  return ieeeBits;
}

std::pair<unsigned, unsigned>
SmithrilBuilder::getFloatSortFromBitWidth(unsigned bitWidth) {
  switch (bitWidth) {
  case Expr::Int16: {
    return {5, 11};
  }
  case Expr::Int32: {
    return {8, 24};
  }
  case Expr::Int64: {
    return {11, 53};
  }
  case Expr::Fl80: {
    // Note this is an IEEE754 with a 15 bit exponent
    // and 64 bit significand. This is not the same
    // as x87 fp80 which has a different binary encoding.
    // We can use this Smithril type to get the appropriate
    // amount of precision. We just have to be very
    // careful which casting between floats and bitvectors.
    //
    // Note that the number of significand bits includes the "implicit"
    // bit (which is not implicit for x87 fp80).
    return {15, 64};
  }
  case Expr::Int128: {
    return {15, 113};
  }
  default:
    assert(0 && "bitWidth cannot converted to a IEEE-754 binary-* number by "
                "Smithril");
    std::abort();
  }
}

SmithrilTerm SmithrilBuilder::castToFloat(const SmithrilTerm &e) {
  SmithrilSort currentSort = smithril_get_sort(ctx, e);
  if (smithril_get_sort_kind(currentSort) == SortKind::Fp) {
    // Already a float
    return e;
  } else if (smithril_get_sort_kind(currentSort) == SortKind::Bv) {
    unsigned bitWidth =
        smithril_get_bv_sort_size(currentSort); // mixa117 no such?
    switch (bitWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128: {
      auto out_width = getFloatSortFromBitWidth(bitWidth);
      return smithril_mk_fp_to_fp_from_bv(ctx, e, out_width.first,
                                          out_width.second);
    }
    case Expr::Fl80: {
      // The bit pattern used by x87 fp80 and what we use in Smithril are
      // different
      //
      // x87 fp80
      //
      // Sign Exponent Significand
      // [1]    [15]   [1] [63]
      //
      // The exponent has bias 16383 and the significand has the integer
      // portion as an explicit bit
      //
      // 79-bit IEEE-754 encoding used in Smithril
      //
      // Sign Exponent [Significand]
      // [1]    [15]       [63]
      //
      // Exponent has bias 16383 (2^(15-1) -1) and the significand has
      // the integer portion as an implicit bit.
      //
      // We need to provide the mapping here and also emit a side constraint
      // to make sure the explicit bit is appropriately constrained so when
      // Smithril generates a model we get the correct bit pattern back.
      //
      // This assumes Smithril's IEEE semantics, x87 fp80 actually
      // has additional semantics due to the explicit bit (See 8.2.2
      // "Unsupported  Double Extended-Precision Floating-Point Encodings and
      // Pseudo-Denormals" in the Intel 64 and IA-32 Architectures Software
      // Developer's Manual) but this encoding means we can't model these
      // unsupported values in Smithril.
      //
      // Note this code must kept in sync with
      // `SmithrilBuilder::castToBitVector()`. Which performs the inverse
      // operation here.
      //
      // TODO: Experiment with creating a constraint that transforms these
      // unsupported bit patterns into a Smithril NaN to approximate the
      // behaviour from those values.

      // Note we try very hard here to avoid calling into our functions
      // here that do implicit casting so we can never recursively call
      // into this function.
      SmithrilTerm signBit = smithril_mk_extract(ctx, 79, 79, e);
      SmithrilTerm exponentBits = smithril_mk_extract(ctx, 78, 64, e);
      SmithrilTerm significandIntegerBit = smithril_mk_extract(ctx, 63, 63, e);
      SmithrilTerm significandFractionBits = smithril_mk_extract(ctx, 62, 0, e);

      SmithrilTerm ieeeBitPatternAsFloat =
          smithril_mk_fp(ctx, signBit, exponentBits, significandFractionBits);

      // Generate side constraint on the significand integer bit. It is not
      // used in `ieeeBitPatternAsFloat` so we need to constrain that bit to
      // have the correct value so that when Smithril gives a model the bit
      // pattern has the right value for x87 fp80.
      //
      // If the number is a denormal or zero then the implicit integer bit
      // is zero otherwise it is one.
      SmithrilTerm significandIntegerBitConstrainedValue =
          getx87FP80ExplicitSignificandIntegerBit(ieeeBitPatternAsFloat);
      SmithrilTerm significandIntegerBitConstraint = smithril_mk_eq(
          ctx, significandIntegerBit, significandIntegerBitConstrainedValue);

      sideConstraints.push_back(significandIntegerBitConstraint);
      return ieeeBitPatternAsFloat;
    }
    default:
      llvm_unreachable("Unhandled width when casting bitvector to float");
    }
  } else {
    llvm_unreachable("Sort cannot be cast to float");
  }
}

SmithrilTerm SmithrilBuilder::castToBitVector(const SmithrilTerm &e) {
  SmithrilSort currentSort = smithril_get_sort(ctx, e);
  if (smithril_get_sort_kind(currentSort) == SortKind::Bool) {
    return smithril_mk_ite(ctx, e, bvOne(1), bvZero(1));
  } else if (smithril_get_sort_kind(currentSort) == SortKind::Bv) {
    // Already a bitvector
    return e;
  } else if (smithril_get_sort_kind(currentSort) == SortKind::Fp) {
    // Note this picks a single representation for NaN which means
    // `castToBitVector(castToFloat(e))` might not equal `e`.
    unsigned exponentBits = smithril_fp_get_bv_exp_size(e);
    unsigned significandBits = smithril_fp_get_bv_sig_size(e);
    unsigned floatWidth = exponentBits + significandBits;

    switch (floatWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return fpToIEEEBV(e);
    case 79: {
      // This is Expr::Fl80 (64 bit exponent, 15 bit significand) but due to
      // the "implicit" bit actually being implicit in x87 fp80 the sum of
      // the exponent and significand bitwidth is 79 not 80.

      // Get Smithril's IEEE representation
      SmithrilTerm ieeeBits = fpToIEEEBV(e);

      // Construct the x87 fp80 bit representation
      SmithrilTerm signBit = smithril_mk_extract(ctx, 78, 78, ieeeBits);
      SmithrilTerm exponentBits = smithril_mk_extract(ctx, 77, 63, ieeeBits);
      SmithrilTerm significandIntegerBit =
          getx87FP80ExplicitSignificandIntegerBit(e);
      SmithrilTerm significandFractionBits =
          smithril_mk_extract(ctx, 62, 0, ieeeBits);
      SmithrilTerm x87FP80Bits = smithril_mk_concat(ctx, signBit, exponentBits);
      x87FP80Bits = smithril_mk_concat(ctx, x87FP80Bits, significandIntegerBit);
      x87FP80Bits =
          smithril_mk_concat(ctx, x87FP80Bits, significandFractionBits);
      // mixa117 3 concats vs 1 concat in bitwuzla?
      return x87FP80Bits;
    }
    default:
      llvm_unreachable("Unhandled width when casting float to bitvector");
    }
  } else {
    llvm_unreachable("Sort cannot be cast to float");
  }
}

RoundingMode
SmithrilBuilder::getRoundingModeSort(llvm::APFloat::roundingMode rm) {
  switch (rm) {
  case llvm::APFloat::rmNearestTiesToEven:
    return RoundingMode::RNE;
  case llvm::APFloat::rmTowardPositive:
    return RoundingMode::RTP;
  case llvm::APFloat::rmTowardNegative:
    return RoundingMode::RTN;
  case llvm::APFloat::rmTowardZero:
    return RoundingMode::RTZ;
  case llvm::APFloat::rmNearestTiesToAway:
    return RoundingMode::RNA;
  default:
    llvm_unreachable("Unhandled rounding mode");
  }
}

SmithrilTerm SmithrilBuilder::getx87FP80ExplicitSignificandIntegerBit(
    const SmithrilTerm &e) {
#ifndef NDEBUG
  // Check the passed in expression is the right type.
  SmithrilSort currentSort = smithril_get_sort(ctx, e);
  assert(smithril_get_sort_kind(currentSort) == Fp);

  unsigned exponentBits = smithril_fp_get_bv_exp_size(e);
  unsigned significandBits = smithril_fp_get_bv_sig_size(e);
  assert(exponentBits == 15);
  llvm::errs() << "significandBits: " << significandBits << "\n";
  assert(significandBits == 64);
#endif
  // If the number is a denormal or zero then the implicit integer bit is zero
  // otherwise it is one.  SmithrilTerm isDenormal =
  SmithrilTerm isDenormal = smithril_fp_is_subnormal(ctx, e);
  SmithrilTerm isZero = smithril_fp_is_zero(ctx, e);

  // FIXME: Cache these constants somewhere
  SmithrilSort oneBitBvSort = getBvSort(/*width=*/1);

  SmithrilTerm oneBvOne = smithril_mk_bv_value_uint64(ctx, oneBitBvSort, 1);
  SmithrilTerm zeroBvOne = smithril_mk_bv_value_uint64(ctx, oneBitBvSort, 0);

  SmithrilTerm significandIntegerBitCondition = orExpr(isDenormal, isZero);

  SmithrilTerm significandIntegerBitConstrainedValue =
      smithril_mk_ite(ctx, significandIntegerBitCondition, zeroBvOne, oneBvOne);

  return significandIntegerBitConstrainedValue;
}
} // namespace klee

#endif // ENABLE_SMITHRIL
