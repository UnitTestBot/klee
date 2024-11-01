//===-- Z3Builder.cpp ------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#ifdef ENABLE_Z3
#include "Z3Builder.h"

#include "klee/ADT/Bits.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Module/KModule.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverStats.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/iterator_range.h"

using namespace klee;

namespace klee {

// Declared here rather than `Z3Builder.h` so they can be called in gdb.
template <> void Z3NodeHandle<Z3_sort>::dump() const {
  llvm::errs() << "Z3SortHandle:\n"
               << ::Z3_sort_to_string(context, node) << "\n";
}
template <> unsigned Z3NodeHandle<Z3_sort>::hash() {
  return Z3_get_ast_hash(context, as_ast());
}
template <> void Z3NodeHandle<Z3_ast>::dump() const {
  llvm::errs() << "Z3ASTHandle:\n"
               << ::Z3_ast_to_string(context, as_ast()) << "\n";
}
template <> unsigned Z3NodeHandle<Z3_ast>::hash() {
  return Z3_get_ast_hash(context, as_ast());
}

void custom_z3_error_handler(Z3_context ctx, Z3_error_code ec) {
  ::Z3_string errorMsg =
#ifdef HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT
      // Z3 > 4.4.1
      Z3_get_error_msg(ctx, ec);
#else
      // Z3 4.4.1
      Z3_get_error_msg(ec);
#endif
  // FIXME: This is kind of a hack. The value comes from the enum
  // Z3_CANCELED_MSG but this isn't currently exposed by Z3's C API
  if (strcmp(errorMsg, "canceled") == 0) {
    // Solver timeout is not a fatal error
    return;
  }
  llvm::errs() << "Error: Incorrect use of Z3. [" << ec << "] " << errorMsg
               << "\n";
  // NOTE: The current implementation of `Z3_close_log()` can be safely
  // called even if the log isn't open.
  Z3_close_log();
  abort();
}

Z3ArrayExprHash::~Z3ArrayExprHash() {}

void Z3ArrayExprHash::clear() {
  _update_node_hash.clear();
  _array_hash.clear();
}

void Z3ArrayExprHash::clearUpdates() { _update_node_hash.clear(); }

Z3Builder::Z3Builder(bool autoClearConstructCache,
                     const char *z3LogInteractionFileArg)
    : autoClearConstructCache(autoClearConstructCache),
      z3LogInteractionFile("") {
  if (z3LogInteractionFileArg)
    this->z3LogInteractionFile = std::string(z3LogInteractionFileArg);
  if (z3LogInteractionFile.length() > 0) {
    klee_message("Logging Z3 API interaction to \"%s\"",
                 z3LogInteractionFile.c_str());
    assert(!Z3HashConfig::Z3InteractionLogOpen &&
           "interaction log should not already be open");
    Z3_open_log(z3LogInteractionFile.c_str());
    Z3HashConfig::Z3InteractionLogOpen = true;
  }
  // FIXME: Should probably let the client pass in a Z3_config instead
  Z3_config cfg = Z3_mk_config();
  // It is very important that we ask Z3 to let us manage memory so that
  // we are able to cache expressions and sorts.
  ctx = Z3_mk_context_rc(cfg);
  // Make sure we handle any errors reported by Z3.
  Z3_set_error_handler(ctx, custom_z3_error_handler);
  // When emitting Z3 expressions make them SMT-LIBv2 compliant
  Z3_set_ast_print_mode(ctx, Z3_PRINT_SMTLIB2_COMPLIANT);
  Z3_del_config(cfg);
}

Z3Builder::~Z3Builder() {
  // Clear caches so exprs/sorts gets freed before the destroying context
  // they aren associated with.
  clearConstructCache();
  _arr_hash.clear();
  constant_array_assertions.clear();
  Z3_del_context(ctx);
  if (z3LogInteractionFile.length() > 0) {
    Z3_close_log();
    Z3HashConfig::Z3InteractionLogOpen = false;
  }
}

Z3SortHandle Z3Builder::getBoolSort() {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_bool_sort(ctx), ctx);
}

Z3SortHandle Z3Builder::getBvSort(unsigned width) {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_bv_sort(ctx, width), ctx);
}

Z3SortHandle Z3Builder::getArraySort(Z3SortHandle domainSort,
                                     Z3SortHandle rangeSort) {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_array_sort(ctx, domainSort, rangeSort), ctx);
}

Z3ASTHandle Z3Builder::buildFreshBoolConst() {
  Z3SortHandle boolSort = getBoolSort();
  return Z3ASTHandle(Z3_mk_fresh_const(ctx, "freshName", boolSort), ctx);
}

Z3ASTHandle Z3Builder::buildArray(const char *name, unsigned indexWidth,
                                  unsigned valueWidth) {
  Z3SortHandle domainSort = getBvSort(indexWidth);
  Z3SortHandle rangeSort = getBvSort(valueWidth);
  Z3SortHandle t = getArraySort(domainSort, rangeSort);
  Z3_symbol s = Z3_mk_string_symbol(ctx, const_cast<char *>(name));
  return Z3ASTHandle(Z3_mk_const(ctx, s, t), ctx);
}

Z3ASTHandle Z3Builder::buildConstantArray(const char *, unsigned indexWidth,
                                          unsigned valueWidth, unsigned value) {
  Z3SortHandle domainSort = getBvSort(indexWidth);
  Z3ASTHandle defaultValue = bvZExtConst(valueWidth, value);
  return Z3ASTHandle(Z3_mk_const_array(ctx, domainSort, defaultValue), ctx);
}

Z3ASTHandle Z3Builder::getTrue() { return Z3ASTHandle(Z3_mk_true(ctx), ctx); }

Z3ASTHandle Z3Builder::getFalse() { return Z3ASTHandle(Z3_mk_false(ctx), ctx); }

Z3ASTHandle Z3Builder::bvOne(unsigned width) { return bvZExtConst(width, 1); }

Z3ASTHandle Z3Builder::bvZero(unsigned width) { return bvZExtConst(width, 0); }

Z3ASTHandle Z3Builder::bvMinusOne(unsigned width) {
  return bvSExtConst(width, (int64_t)-1);
}

Z3ASTHandle Z3Builder::bvConst32(unsigned width, uint32_t value) {
  Z3SortHandle t = getBvSort(width);
  return Z3ASTHandle(Z3_mk_unsigned_int(ctx, value, t), ctx);
}

Z3ASTHandle Z3Builder::bvConst64(unsigned width, uint64_t value) {
  Z3SortHandle t = getBvSort(width);
  return Z3ASTHandle(Z3_mk_unsigned_int64(ctx, value, t), ctx);
}

Z3ASTHandle Z3Builder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3ASTHandle expr = Z3ASTHandle(bvConst64(64, value), ctx);
  Z3ASTHandle zero = Z3ASTHandle(bvConst64(64, 0), ctx);
  for (width -= 64; width > 64; width -= 64)
    expr = Z3ASTHandle(Z3_mk_concat(ctx, zero, expr), ctx);
  return Z3ASTHandle(Z3_mk_concat(ctx, bvConst64(width, 0), expr), ctx);
}

Z3ASTHandle Z3Builder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3SortHandle t = getBvSort(width - 64);
  if (value >> 63) {
    Z3ASTHandle r = Z3ASTHandle(Z3_mk_int64(ctx, -1, t), ctx);
    return Z3ASTHandle(Z3_mk_concat(ctx, r, bvConst64(64, value)), ctx);
  }

  Z3ASTHandle r = Z3ASTHandle(Z3_mk_int64(ctx, 0, t), ctx);
  return Z3ASTHandle(Z3_mk_concat(ctx, r, bvConst64(64, value)), ctx);
}

Z3ASTHandle Z3Builder::bvBoolExtract(Z3ASTHandle expr, int bit) {
  return Z3ASTHandle(Z3_mk_eq(ctx, bvExtract(expr, bit, bit), bvOne(1)), ctx);
}

Z3ASTHandle Z3Builder::notExpr(Z3ASTHandle expr) {
  return Z3ASTHandle(Z3_mk_not(ctx, expr), ctx);
}

Z3ASTHandle Z3Builder::andExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  ::Z3_ast args[2] = {lhs, rhs};
  return Z3ASTHandle(Z3_mk_and(ctx, 2, args), ctx);
}

Z3ASTHandle Z3Builder::orExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  ::Z3_ast args[2] = {lhs, rhs};
  return Z3ASTHandle(Z3_mk_or(ctx, 2, args), ctx);
}

Z3ASTHandle Z3Builder::iffExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  Z3SortHandle lhsSort = Z3SortHandle(Z3_get_sort(ctx, lhs), ctx);
  Z3SortHandle rhsSort = Z3SortHandle(Z3_get_sort(ctx, rhs), ctx);
  assert(Z3_get_sort_kind(ctx, lhsSort) == Z3_get_sort_kind(ctx, rhsSort) &&
         "lhs and rhs sorts must match");
  assert(Z3_get_sort_kind(ctx, lhsSort) == Z3_BOOL_SORT &&
         "args must have BOOL sort");
  return Z3ASTHandle(Z3_mk_iff(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3Builder::writeExpr(Z3ASTHandle array, Z3ASTHandle index,
                                 Z3ASTHandle value) {
  return Z3ASTHandle(Z3_mk_store(ctx, array, index, value), ctx);
}

Z3ASTHandle Z3Builder::readExpr(Z3ASTHandle array, Z3ASTHandle index) {
  return Z3ASTHandle(Z3_mk_select(ctx, array, index), ctx);
}

unsigned Z3Builder::getBVLength(Z3ASTHandle expr) {
  return Z3_get_bv_sort_size(ctx, Z3SortHandle(Z3_get_sort(ctx, expr), ctx));
}

Z3ASTHandle Z3Builder::getInitialArray(const Array *root) {

  assert(root);
  Z3ASTHandle array_expr;
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
    } else if (ref<MockDeterministicSource> mockDeterministicSource =
                   dyn_cast<MockDeterministicSource>(root->source)) {
      size_t num_args = mockDeterministicSource->args.size();
      std::vector<Z3_ast> args(num_args);
      std::vector<Z3_sort> argsSort(num_args);
      for (size_t i = 0; i < num_args; i++) {
        ref<Expr> kid = mockDeterministicSource->args[i];
        int kidWidth = kid->getWidth();
        Z3ASTHandle argsHandle = construct(kid, &kidWidth);
        args[i] = argsHandle;
        Z3SortHandle z3SortHandle =
            Z3SortHandle(Z3_get_sort(ctx, args[i]), ctx);
        argsSort[i] = z3SortHandle;
      }

      Z3SortHandle domainSort = getBvSort(root->getDomain());
      Z3SortHandle rangeSort = getBvSort(root->getRange());
      Z3SortHandle retValSort = getArraySort(domainSort, rangeSort);

      Z3FuncDeclHandle func;
      func = Z3FuncDeclHandle(
          Z3_mk_func_decl(
              ctx,
              Z3_mk_string_symbol(
                  ctx,
                  mockDeterministicSource->function.getName().str().c_str()),
              num_args, argsSort.data(), retValSort),
          ctx);
      array_expr =
          Z3ASTHandle(Z3_mk_app(ctx, func, num_args, args.data()), ctx);
    } else {
      array_expr =
          buildArray(unique_name.c_str(), root->getDomain(), root->getRange());
    }

    if (source) {
      if (auto constSize = dyn_cast<ConstantExpr>(root->size)) {
        std::vector<Z3ASTHandle> array_assertions;
        for (size_t i = 0; i < constSize->getZExtValue(); i++) {
          auto value = source->constantValues->load(i);
          // construct(= (select i root) root->value[i]) to be asserted in
          // Z3Solver.cpp
          int width_out;
          Z3ASTHandle array_value = construct(value, &width_out);
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
          Z3ASTHandle array_value = construct(value, &width_out);
          assert(width_out == (int)root->getRange() &&
                 "Value doesn't match root range");
          array_expr = writeExpr(
              array_expr, bvConst32(root->getDomain(), index), array_value);
        }
      }
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }

  return (array_expr);
}

Z3_sort_kind Z3Builder::getSortKind(const Z3ASTHandle &e) {
  auto sort = Z3_get_sort(ctx, e);
  return Z3_get_sort_kind(ctx, sort);
}

  Z3ASTHandle Z3Builder::castToBitVector(const Z3ASTHandle &e) {
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  Z3_sort_kind kind = Z3_get_sort_kind(ctx, currentSort);
  switch (kind) {
  case Z3_BOOL_SORT:
    return Z3ASTHandle(Z3_mk_ite(ctx, e, bvOne(1), bvZero(1)), ctx);
  case Z3_BV_SORT:
    // Already a bitvector
    return e;
  case Z3_FLOATING_POINT_SORT: {
    // Note this picks a single representation for NaN which means
    // `castToBitVector(castToFloat(e))` might not equal `e`.
    unsigned exponentBits = Z3_fpa_get_ebits(ctx, currentSort);
    unsigned significandBits =
        Z3_fpa_get_sbits(ctx, currentSort); // Includes implicit bit
    unsigned floatWidth = exponentBits + significandBits;
    switch (floatWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return Z3ASTHandle(Z3_mk_fpa_to_ieee_bv(ctx, e), ctx);
    case 79: {
      // This is Expr::Fl80 (64 bit exponent, 15 bit significand) but due to
      // the "implicit" bit actually being implicit in x87 fp80 the sum of
      // the exponent and significand bitwidth is 79 not 80.

      // Get Z3's IEEE representation
      Z3ASTHandle ieeeBits = Z3ASTHandle(Z3_mk_fpa_to_ieee_bv(ctx, e), ctx);

      // Construct the x87 fp80 bit representation
      Z3ASTHandle signBit = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/78, /*low=*/78, ieeeBits), ctx);
      Z3ASTHandle exponentBits = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/77, /*low=*/63, ieeeBits), ctx);
      Z3ASTHandle significandIntegerBit =
          getx87FP80ExplicitSignificandIntegerBit(e);
      Z3ASTHandle significandFractionBits = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/62, /*low=*/0, ieeeBits), ctx);

      Z3ASTHandle x87FP80Bits =
          Z3ASTHandle(Z3_mk_concat(ctx, signBit, exponentBits), ctx);
      x87FP80Bits = Z3ASTHandle(
          Z3_mk_concat(ctx, x87FP80Bits, significandIntegerBit), ctx);
      x87FP80Bits = Z3ASTHandle(
          Z3_mk_concat(ctx, x87FP80Bits, significandFractionBits), ctx);
#ifndef NDEBUG
      Z3SortHandle x87FP80BitsSort =
          Z3SortHandle(Z3_get_sort(ctx, x87FP80Bits), ctx);
      assert(Z3_get_sort_kind(ctx, x87FP80BitsSort) == Z3_BV_SORT);
      assert(Z3_get_bv_sort_size(ctx, x87FP80BitsSort) == 80);
#endif
      return x87FP80Bits;
    }
    default:
      llvm_unreachable("Unhandled width when casting float to bitvector");
    }
  }
  default:
    llvm_unreachable("Sort cannot be cast to float");
  }
}

Z3ASTHandle Z3Builder::castToBool(const Z3ASTHandle &e) {
  auto kind = getSortKind(e);
  if (kind == Z3_sort_kind::Z3_BOOL_SORT) {
    return e;
  } else {
    return Z3ASTHandle(Z3_mk_eq(ctx, e, bvOne(1)), ctx);
  }
}


Z3ASTHandle Z3Builder::getInitialRead(const Array *root, unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

Z3ASTHandle Z3Builder::getArrayForUpdate(const Array *root,
                                         const UpdateNode *un) {
  // Iterate over the update nodes, until we find a cached version of the node,
  // or no more update nodes remain
  Z3ASTHandle un_expr;
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

  // Create and cache solver expressions based on the update nodes starting from
  // the oldest
  for (const auto &un :
       llvm::make_range(update_nodes.crbegin(), update_nodes.crend())) {
    un_expr =
        writeExpr(un_expr, construct(un->index, 0), construct(un->value, 0));

    _arr_hash.hashUpdateNodeExpr(un, un_expr);
  }

  return un_expr;
}

Z3ASTHandle Z3Builder::construct(ref<Expr> e, int *width_out) {
  // TODO: We could potentially use Z3_simplify() here
  // to store simpler expressions.
  if (!Z3HashConfig::UseConstructHashZ3 || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap<std::pair<Z3ASTHandle, unsigned>>::iterator it =
        constructed.find(e);
    if (it != constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out)
        width_out = &width;
      Z3ASTHandle res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}

} // namespace klee
#endif // ENABLE_Z3
