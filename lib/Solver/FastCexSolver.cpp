//===-- FastCexSolver.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cex-solver"
#include "klee/Solver/Solver.h"

#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprEvaluator.h"
#include "klee/Expr/ExprRangeEvaluator.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Solver/IncompleteSolver.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/APInt.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <cassert>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

using namespace klee;
using namespace llvm;

namespace {
enum class FastCexSolverType { EQUALITY, ALL };

cl::opt<FastCexSolverType> FastCexFor(
    "fast-cex-for",
    cl::desc(
        "Specifiy a query predicate to filter queries for FastCexSolver using"),
    cl::values(clEnumValN(FastCexSolverType::EQUALITY, "equality",
                          "Query with only equality expressions"),
               clEnumValN(FastCexSolverType::ALL, "all", "All queries")),
    cl::init(FastCexSolverType::EQUALITY), cl::cat(SolvingCat));
} // namespace

// Hacker's Delight, pgs 58-63
static llvm::APInt minOR(llvm::APInt a, llvm::APInt b, llvm::APInt c,
                         llvm::APInt d) {
  assert(a.getBitWidth() == c.getBitWidth());

  llvm::APInt m =
      llvm::APInt::getOneBitSet(a.getBitWidth(), a.getBitWidth() - 1);
  while (m.getBoolValue()) {
    if ((a.reverseBits() & c & m).getBoolValue()) {
      llvm::APInt temp = (a | m) & -m;
      if (temp.ule(b)) {
        a = temp;
        break;
      }
    } else if ((a & c.reverseBits() & m).getBoolValue()) {
      llvm::APInt temp = (c | m) & -m;
      if (temp.ule(d)) {
        c = temp;
        break;
      }
    }
    m = m.lshr(1);
  }

  return a | c;
}
static llvm::APInt maxOR(llvm::APInt a, llvm::APInt b, llvm::APInt c,
                         llvm::APInt d) {
  assert(a.getBitWidth() == c.getBitWidth());

  llvm::APInt m =
      llvm::APInt::getOneBitSet(a.getBitWidth(), a.getBitWidth() - 1);

  while (m.getBoolValue()) {
    if ((b & d & m).getBoolValue()) {
      llvm::APInt temp = (b - m) | (m - 1);
      if (temp.uge(a)) {
        b = temp;
        break;
      }
      temp = (d - m) | (m - 1);
      if (temp.uge(c)) {
        d = temp;
        break;
      }
    }
    m = m.lshr(1);
  }

  return b | d;
}

static llvm::APInt minAND(llvm::APInt a, llvm::APInt b, llvm::APInt c,
                          llvm::APInt d) {
  assert(a.getBitWidth() == c.getBitWidth());

  llvm::APInt m =
      llvm::APInt::getOneBitSet(a.getBitWidth(), a.getBitWidth() - 1);

  while (m.getBoolValue()) {
    if ((~a & ~c & m).getBoolValue()) {
      llvm::APInt temp = (a | m) & -m;
      if (temp.ule(b)) {
        a = temp;
        break;
      }
      temp = (c | m) & -m;
      if (temp.ule(d)) {
        c = temp;
        break;
      }
    }
    m = m.lshr(1);
  }

  return a & c;
}
static llvm::APInt maxAND(llvm::APInt a, llvm::APInt b, llvm::APInt c,
                          llvm::APInt d) {
  assert(a.getBitWidth() == c.getBitWidth());

  llvm::APInt m =
      llvm::APInt::getOneBitSet(a.getBitWidth(), a.getBitWidth() - 1);

  while (m.getBoolValue()) {
    if ((b & ~d & m).getBoolValue()) {
      llvm::APInt temp = (b & ~m) | (m - 1);
      if (temp.uge(a)) {
        b = temp;
        break;
      }
    } else if ((~b & d & m).getBoolValue()) {
      llvm::APInt temp = (d & ~m) | (m - 1);
      if (temp.uge(c)) {
        d = temp;
        break;
      }
    }
    m = m.lshr(1);
  }

  return b & d;
}

///

class ValueRange {
private:
  llvm::APInt m_min, m_max;
  unsigned width;

public:
  ValueRange() noexcept : width(m_min.getBitWidth()) {}
  ValueRange(const ref<ConstantExpr> &ce) : width(ce->getWidth()) {
    m_min = m_max = ce->getAPValue();
  }
  explicit ValueRange(const llvm::APInt &value) noexcept
      : m_min(value), m_max(value), width(value.getBitWidth()) {}
  ValueRange(const llvm::APInt &_min, const llvm::APInt &_max) noexcept
      : m_min(_min), m_max(_max), width(m_min.getBitWidth()) {
    assert(m_min.getBitWidth() == m_max.getBitWidth());
  }
  ValueRange(const ValueRange &other) noexcept = default;
  ValueRange &operator=(const ValueRange &other) noexcept = default;
  ValueRange(ValueRange &&other) noexcept = default;
  ValueRange &operator=(ValueRange &&other) noexcept = default;

  void print(llvm::raw_ostream &os) const {
    if (isFixed()) {
      os << m_min;
    } else {
      os << "[" << m_min << "," << m_max << "]";
    }
  }

  unsigned bitWidth() const { return width; }

  bool isEmpty() const noexcept { return m_min.ugt(m_max); }
  bool contains(const llvm::APInt &value) const {
    return this->intersects(ValueRange(value));
  }
  bool intersects(const ValueRange &b) const {
    return !this->set_intersection(b).isEmpty();
  }

  bool isFullRange(unsigned bits) const noexcept {
    return m_min == 0 && m_max == llvm::APInt::getAllOnesValue(bits);
  }

  ValueRange set_intersection(const ValueRange &b) const {
    return ValueRange(llvm::APIntOps::umax(m_min, b.m_min),
                      llvm::APIntOps::umin(m_max, b.m_max));
  }
  ValueRange set_union(const ValueRange &b) const {
    return ValueRange(llvm::APIntOps::umin(m_min, b.m_min),
                      llvm::APIntOps::umax(m_max, b.m_max));
  }
  ValueRange set_difference(const ValueRange &b) const {
    if (b.isEmpty() || b.m_min.ugt(m_max) ||
        b.m_max.ult(m_min)) { // no intersection
      return *this;
    } else if (b.m_min.ule(m_min) && b.m_max.uge(m_max)) { // empty
      return ValueRange(llvm::APInt::getOneBitSet(width, 0),
                        llvm::APInt::getNullValue(width));
    } else if (b.m_min.ule(m_min)) { // one range out
      // cannot overflow because b.m_max < m_max
      return ValueRange(b.m_max + 1, m_max);
    } else if (b.m_max.uge(m_max)) {
      // cannot overflow because b.min > m_min
      return ValueRange(m_min, b.m_min - 1);
    } else {
      // two ranges, take bottom
      return ValueRange(m_min, b.m_min - 1);
    }
  }
  ValueRange binaryAnd(const ValueRange &b) const {
    // XXX
    assert(!isEmpty() && !b.isEmpty() && "XXX");
    if (isFixed() && b.isFixed()) {
      return ValueRange(m_min & b.m_min);
    } else {
      return ValueRange(minAND(m_min, m_max, b.m_min, b.m_max),
                        maxAND(m_min, m_max, b.m_min, b.m_max));
    }
  }
  ValueRange binaryAnd(const llvm::APInt &b) const {
    return binaryAnd(ValueRange(b));
  }
  ValueRange binaryOr(ValueRange b) const {
    // XXX
    assert(!isEmpty() && !b.isEmpty() && "XXX");
    if (isFixed() && b.isFixed()) {
      return ValueRange(m_min | b.m_min);
    } else {
      return ValueRange(minOR(m_min, m_max, b.m_min, b.m_max),
                        maxOR(m_min, m_max, b.m_min, b.m_max));
    }
  }
  ValueRange binaryOr(const llvm::APInt &b) const {
    return binaryOr(ValueRange(b));
  }
  ValueRange binaryXor(ValueRange b) const {
    if (isFixed() && b.isFixed()) {
      return ValueRange(m_min ^ b.m_min);
    } else {
      llvm::APInt t = m_max | b.m_max;
      if (!t.isPowerOf2()) {
        t = llvm::APInt::getOneBitSet(t.getBitWidth(),
                                      t.getBitWidth() - t.countLeadingZeros());
      }
      return ValueRange(llvm::APInt::getNullValue(t.getBitWidth()),
                        (t << 1) - 1);
    }
  }

  ValueRange binaryShiftLeft(unsigned bits) const {
    return ValueRange(m_min << bits, m_max << bits);
  }
  ValueRange binaryShiftRight(unsigned bits) const {
    return ValueRange(m_min.lshr(bits), m_max.lshr(bits));
  }

  ValueRange concat(ValueRange b, unsigned bits) const {
    ValueRange newRange =
        ValueRange(m_min.zext(bitWidth() + bits), m_max.zext(bitWidth() + bits))
            .binaryShiftLeft(bits);
    b.m_min = b.m_min.zext(bitWidth() + bits);
    b.m_max = b.m_max.zext(bitWidth() + bits);
    return newRange.binaryOr(b);
  }
  ValueRange extract(std::uint64_t lowBit, std::uint64_t maxBit) const {
    assert(!isEmpty());
    ValueRange newRange =
        binaryShiftRight(width - maxBit)
            .binaryAnd(llvm::APInt::getLowBitsSet(width, maxBit - lowBit));
    newRange.width = maxBit - lowBit;
    newRange.m_min = newRange.m_min.trunc(newRange.width);
    newRange.m_max = newRange.m_max.trunc(newRange.width);
    assert(!newRange.isEmpty());
    return newRange;
  }

  ValueRange add(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange sub(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange mul(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange udiv(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange sdiv(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange urem(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange srem(const ValueRange &b, unsigned width) const {
    return ValueRange(llvm::APInt::getNullValue(width),
                      llvm::APInt::getAllOnesValue(width));
  }
  ValueRange zextOrTrunc(unsigned newWidth) const {
    ValueRange zextOrTruncRange;
    zextOrTruncRange.m_min = m_min.zextOrTrunc(newWidth);
    zextOrTruncRange.m_max = m_max.zextOrTrunc(newWidth);
    zextOrTruncRange.width = newWidth;
    return zextOrTruncRange;
  }
  ValueRange sextOrTrunc(unsigned newWidth) const {
    ValueRange sextOrTruncRange;
    sextOrTruncRange.m_min = m_min.sextOrTrunc(newWidth);
    sextOrTruncRange.m_max = m_max.sextOrTrunc(newWidth);
    sextOrTruncRange.width = newWidth;
    return sextOrTruncRange;
  }

  // use min() to get value if true (XXX should we add a method to
  // make code clearer?)
  bool isFixed() const noexcept { return m_min == m_max; }

  bool operator==(const ValueRange &b) const noexcept {
    return m_min == b.m_min && m_max == b.m_max;
  }
  bool operator!=(const ValueRange &b) const noexcept { return !(*this == b); }

  bool mustEqual(const llvm::APInt &b) const noexcept {
    return m_min == m_max && m_min == b;
  }
  bool mayEqual(const llvm::APInt &b) const noexcept {
    return m_min.ule(b) && m_max.uge(b);
  }

  bool mustEqual(const ValueRange &b) const noexcept {
    return isFixed() && b.isFixed() && m_min == b.m_min;
  }
  bool mayEqual(const ValueRange &b) const { return this->intersects(b); }

  llvm::APInt min() const noexcept {
    assert(!isEmpty() && "cannot get minimum of empty range");
    return m_min;
  }

  llvm::APInt max() const noexcept {
    assert(!isEmpty() && "cannot get maximum of empty range");
    return m_max;
  }

  llvm::APInt minSigned(unsigned bits) const {
    // if max allows sign bit to be set then it can be smallest value,
    // otherwise since the range is not empty, min cannot have a sign
    // bit

    llvm::APInt smallest = llvm::APInt::getSignedMinValue(bits);

    if (m_max.uge(smallest)) {
      return m_max.sext(bits);
    } else {
      return m_min;
    }
  }

  // Works like a sext instrution: if bits is less then
  // current width, then truncate expression; otherwise
  // extend it to bits.
  llvm::APInt maxSigned(unsigned bits) const {
    llvm::APInt smallest = llvm::APInt::getSignedMinValue(bits);

    // if max and min have sign bit then max is max, otherwise if only
    // max has sign bit then max is largest signed integer, otherwise
    // max is max

    if (m_min.ult(smallest) && m_max.uge(smallest)) {
      return smallest - 1;
    } else {
      // width are not equal here; if this width is shorter, then
      // we will return sign extended max, otherwise we need to find
      // signed max value of first n bits

      return m_max.sext(bits);
    }
  }
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ValueRange &vr) {
  vr.print(os);
  return os;
}

// XXX waste of space, rather have ByteValueRange
typedef ValueRange CexValueData;

class CexObjectData {
  /// possibleContents - An array of "possible" values for the object.
  ///
  /// The possible values is an inexact approximation for the set of values for
  /// each array location.
  SparseStorage<CexValueData> possibleContents;

  /// exactContents - An array of exact values for the object.
  ///
  /// The exact values are a conservative approximation for the set of values
  /// for each array location.
  SparseStorage<CexValueData> exactContents;

  CexObjectData(const CexObjectData &);  // DO NOT IMPLEMENT
  void operator=(const CexObjectData &); // DO NOT IMPLEMENT

public:
  CexObjectData(uint64_t size)
      : possibleContents(ValueRange(llvm::APInt::getNullValue(CHAR_BIT),
                                    llvm::APInt::getAllOnesValue(CHAR_BIT))),
        exactContents(ValueRange(llvm::APInt::getNullValue(CHAR_BIT),
                                 llvm::APInt::getAllOnesValue(CHAR_BIT))) {}

  const CexValueData getPossibleValues(size_t index) const {
    return possibleContents.load(index);
  }
  void setPossibleValues(size_t index, const CexValueData &values) {
    possibleContents.store(index, values);
  }
  void setPossibleValue(size_t index, const llvm::APInt &value) {
    possibleContents.store(index, CexValueData(value));
  }

  const CexValueData getExactValues(size_t index) const {
    return exactContents.load(index);
  }
  void setExactValues(size_t index, CexValueData values) {
    exactContents.store(index, values);
  }

  /// getPossibleValue - Return some possible value.
  llvm::APInt getPossibleValue(size_t index) const {
    CexValueData cvd = possibleContents.load(index);
    return cvd.min() + (cvd.max() - cvd.min()).lshr(1);
  }
};

class CexRangeEvaluator : public ExprRangeEvaluator<ValueRange> {
public:
  std::map<const Array *, CexObjectData *> &objects;
  CexRangeEvaluator(std::map<const Array *, CexObjectData *> &_objects)
      : objects(_objects) {}

  ValueRange getInitialReadRange(const Array &array, ValueRange index) {
    // Check for a concrete read of a constant array.
    if (array.isConstantArray() && index.isFixed()) {
      if (ref<ConstantSource> constantSource =
              dyn_cast<ConstantSource>(array.source)) {
        if (auto value = constantSource->constantValues.load(
                index.min().getZExtValue())) {
          return ValueRange(value->getAPValue());
        }
      }
    }
    return ValueRange(llvm::APInt::getNullValue(CHAR_BIT),
                      llvm::APInt::getAllOnesValue(CHAR_BIT));
  }
};

class CexPossibleEvaluator : public ExprEvaluator {
protected:
  ref<Expr> getInitialValue(const Array &array, unsigned index) {
    // If the index is out of range, we cannot assign it a value, since that
    // value cannot be part of the assignment.
    ref<ConstantExpr> constantArraySize =
        dyn_cast<ConstantExpr>(visit(array.size));
    if (!constantArraySize) {
      klee_error("FIXME: CexPossibleEvaluator: Arrays of symbolic sizes are "
                 "unsupported in FastCex\n");
      std::abort();
    }

    if (!constantArraySize || index >= constantArraySize->getZExtValue()) {
      return ReadExpr::create(UpdateList(&array, 0),
                              ConstantExpr::alloc(index, array.getDomain()));
    }

    std::map<const Array *, CexObjectData *>::iterator it =
        objects.find(&array);
    return ConstantExpr::alloc(
        (it == objects.end()
             ? 127
             : it->second->getPossibleValue(index).getZExtValue()),
        array.getRange());
  }

public:
  std::map<const Array *, CexObjectData *> &objects;
  CexPossibleEvaluator(std::map<const Array *, CexObjectData *> &_objects)
      : objects(_objects) {}
};

class CexExactEvaluator : public ExprEvaluator {
protected:
  ref<Expr> getInitialValue(const Array &array, unsigned index) {
    // If the index is out of range, we cannot assign it a value, since that
    // value cannot be part of the assignment.
    ref<ConstantExpr> constantArraySize =
        dyn_cast<ConstantExpr>(visit(array.size));
    if (!constantArraySize) {
      return ReadExpr::create(UpdateList(&array, 0),
                              ConstantExpr::alloc(index, array.getDomain()));
    }

    if (!constantArraySize || index >= constantArraySize->getZExtValue()) {
      return ReadExpr::create(UpdateList(&array, 0),
                              ConstantExpr::alloc(index, array.getDomain()));
    }

    std::map<const Array *, CexObjectData *>::iterator it =
        objects.find(&array);
    if (it == objects.end())
      return ReadExpr::create(UpdateList(&array, 0),
                              ConstantExpr::alloc(index, array.getDomain()));

    CexValueData cvd = it->second->getExactValues(index);
    if (!cvd.isFixed())
      return ReadExpr::create(UpdateList(&array, 0),
                              ConstantExpr::alloc(index, array.getDomain()));

    return ConstantExpr::create(cvd.min().getZExtValue(), array.getRange());
  }

public:
  std::map<const Array *, CexObjectData *> &objects;
  CexExactEvaluator(std::map<const Array *, CexObjectData *> &_objects)
      : objects(_objects) {}
};

class CexData {
public:
  std::map<const Array *, CexObjectData *> objects;

  CexData(const CexData &);        // DO NOT IMPLEMENT
  void operator=(const CexData &); // DO NOT IMPLEMENT

public:
  CexData() {}
  ~CexData() {
    for (std::map<const Array *, CexObjectData *>::iterator
             it = objects.begin(),
             ie = objects.end();
         it != ie; ++it)
      delete it->second;
  }

  CexObjectData &getObjectData(const Array *A) {
    CexObjectData *&Entry = objects[A];

    ref<ConstantExpr> constantArraySize =
        dyn_cast<ConstantExpr>(evaluatePossible(A->size));
    if (!constantArraySize) {
      klee_error("FIXME: CexData: Arrays of symbolic sizes are unsupported in "
                 "FastCex\n");
      std::abort();
    }

    if (!Entry)
      Entry = new CexObjectData(constantArraySize->getZExtValue());

    return *Entry;
  }

  void propagatePossibleValue(ref<Expr> e, const llvm::APInt &value) {
    propagatePossibleValues(e, CexValueData(value));
  }

  void propogateExactValue(ref<Expr> e, const llvm::APInt &value) {
    propagateExactValues(e, CexValueData(value));
  }

  void propagatePossibleValues(ref<Expr> e, CexValueData range) {
    assert(range.bitWidth() == e->getWidth());

    switch (e->getKind()) {
    case Expr::Constant: {
      ref<ConstantExpr> CE = cast<ConstantExpr>(e);
      assert(range.intersects(ValueRange(CE->getAPValue())) &&
             "Constant is out of range for propagation.");
      // rather a pity if the constant isn't in the range, but how can
      // we use this?
      break;
    }

      // Special

    case Expr::NotOptimized:
      break;

    case Expr::Read: {
      ReadExpr *re = cast<ReadExpr>(e);
      const Array *array = re->updates.root;
      CexObjectData &cod = getObjectData(array);

      // FIXME: This is imprecise, we need to look through the existing writes
      // to see if this is an initial read or not.
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
        if (ref<ConstantExpr> constantArraySize =
                dyn_cast<ConstantExpr>(evaluatePossible(array->size))) {
          uint64_t index = CE->getZExtValue();

          if (index < constantArraySize->getZExtValue()) {
            // If the range is fixed, just set that; even if it conflicts with
            // the previous range it should be a better guess.
            if (range.isFixed()) {
              cod.setPossibleValue(index, range.min());
            } else {
              CexValueData cvd = cod.getPossibleValues(index);
              CexValueData tmp = cvd.set_intersection(range);

              if (!tmp.isEmpty())
                cod.setPossibleValues(index, tmp);
            }
          }
        } else {
          // XXX        fatal("XXX not implemented");
        }
      } else {
        // XXX        fatal("XXX not implemented");
      }
      break;
    }

    case Expr::Select: {
      SelectExpr *se = cast<SelectExpr>(e);
      ValueRange cond = evalRangeForExpr(se->cond);
      if (cond.isFixed()) {
        if (cond.min().getBoolValue()) {
          propagatePossibleValues(se->trueExpr, range);
        } else {
          propagatePossibleValues(se->falseExpr, range);
        }
      }
      break;
    }

    case Expr::Concat: {
      ConcatExpr *ce = cast<ConcatExpr>(e);
      Expr::Width LSBWidth = ce->getLeft()->getWidth();
      Expr::Width MSBWidth = ce->getRight()->getWidth();
      propagatePossibleValues(ce->getLeft(), range.extract(0, LSBWidth));
      propagatePossibleValues(ce->getRight(),
                              range.extract(LSBWidth, LSBWidth + MSBWidth));
      break;
    }

    case Expr::Extract: {
      // XXX
      break;
    }

      // Casting

      // Simply intersect the output range with the range of all possible
      // outputs and then truncate to the desired number of bits.

      // For ZExt this simplifies to just intersection with the possible input
      // range.
    case Expr::ZExt: {
      CastExpr *ce = cast<CastExpr>(e);
      unsigned inBits = ce->src->getWidth();
      unsigned outBits = ce->getWidth();

      // Intersect with range of same bitness and truncate
      // result to inBits (as llvm::APInt can not be compared
      // if they have different width).
      ValueRange input = range
                             .set_intersection(ValueRange(
                                 llvm::APInt::getNullValue(outBits),
                                 llvm::APInt::getLowBitsSet(outBits, inBits)))
                             .zextOrTrunc(inBits);
      propagatePossibleValues(ce->src, input);
      break;
    }
      // For SExt instead of doing the intersection we just take the output
      // range minus the impossible values. This is nicer since it is a single
      // interval.
    case Expr::SExt: {
      CastExpr *ce = cast<CastExpr>(e);
      unsigned inBits = ce->src->getWidth();
      unsigned outBits = ce->width;

      ValueRange input =
          range
              .set_difference(ValueRange(
                  llvm::APInt::getOneBitSet(outBits, inBits - 1),
                  (llvm::APInt::getAllOnesValue(outBits) -
                   llvm::APInt::getLowBitsSet(outBits, inBits - 1) - 1)))
              .zextOrTrunc(inBits);

      propagatePossibleValues(ce->src, input);
      break;
    }

      // Binary

    case Expr::Add: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
        // FIXME: Why do we ever propogate empty ranges? It doesn't make
        // sense.
        if (range.isEmpty())
          break;

        // C_0 + X \in [MIN, MAX) ==> X \in [MIN - C_0, MAX - C_0)
        CexValueData nrange(range.min() - CE->getAPValue(),
                            range.max() - CE->getAPValue());
        if (!nrange.isEmpty()) {
          propagatePossibleValues(be->right, nrange);
        }
      }
      break;
    }

    case Expr::And: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (be->getWidth() == Expr::Bool) {
        if (range.isFixed()) {
          ValueRange left = evalRangeForExpr(be->left);
          ValueRange right = evalRangeForExpr(be->right);

          if (!range.min()) {
            if (left.mustEqual(llvm::APInt::getNullValue(be->getWidth())) ||
                right.mustEqual(llvm::APInt::getNullValue(be->getWidth()))) {
              // all is well
            } else {
              // XXX heuristic, which order

              propagatePossibleValue(
                  be->left, llvm::APInt::getNullValue(be->left->getWidth()));
              left = evalRangeForExpr(be->left);

              // see if that worked
              if (!left.mustEqual(llvm::APInt(be->left->getWidth(), 1))) {
                propagatePossibleValue(
                    be->right, llvm::APInt::getNullValue(be->left->getWidth()));
              }
            }
          } else {
            llvm::APInt leftAPIntOne = llvm::APInt(be->left->getWidth(), 1);
            if (!left.mustEqual(leftAPIntOne)) {
              propagatePossibleValue(be->left, leftAPIntOne);
            }
            llvm::APInt rightAPIntOne = llvm::APInt(be->right->getWidth(), 1);
            if (!right.mustEqual(rightAPIntOne)) {
              propagatePossibleValue(be->right, rightAPIntOne);
            }
          }
        }
      } else {
        // XXX
      }
      break;
    }

    case Expr::Or: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (be->getWidth() == Expr::Bool) {
        if (range.isFixed()) {
          ValueRange left = evalRangeForExpr(be->left);
          ValueRange right = evalRangeForExpr(be->right);

          llvm::APInt zeroAPInt =
              llvm::APInt::getNullValue(be->left->getWidth());
          llvm::APInt oneAPInt = llvm::APInt(be->left->getWidth(), 1);

          if (range.min().getBoolValue()) {
            if (left.mustEqual(oneAPInt) || right.mustEqual(oneAPInt)) {
              // all is well
            } else {
              // XXX heuristic, which order?

              // force left to value we need
              propagatePossibleValue(be->left, oneAPInt);
              left = evalRangeForExpr(be->left);

              // see if that worked
              if (!left.mustEqual(oneAPInt))
                propagatePossibleValue(be->right, oneAPInt);
            }
          } else {
            if (!left.mustEqual(zeroAPInt))
              propagatePossibleValue(be->left, zeroAPInt);
            if (!right.mustEqual(zeroAPInt))
              propagatePossibleValue(be->right, zeroAPInt);
          }
        }
      } else {
        // XXX
      }
      break;
    }

    case Expr::Xor:
      break;

      // Comparison

    case Expr::Eq: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (range.isFixed()) {
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
          llvm::APInt value = CE->getAPValue();
          if (range.min().getBoolValue()) {
            propagatePossibleValue(be->right, value);
          } else {
            CexValueData range;
            if (value == 0) {
              range =
                  CexValueData(llvm::APInt(CE->getWidth(), 1),
                               llvm::APInt::getAllOnesValue(CE->getWidth()));
            } else {
              // FIXME: heuristic / lossy, could be better to pick larger
              // range?

              // FIXME: choose both
              range = CexValueData(llvm::APInt::getNullValue(CE->getWidth()),
                                   value - 1);
              // range = CexValueData(
              //    value + 1, llvm::APInt::getAllOnesValue(CE->getWidth()));
            }
            propagatePossibleValues(be->right, range);
          }
        }
      }
      break;
    }

    case Expr::Not: {
      if (e->getWidth() == Expr::Bool && range.isFixed()) {
        propagatePossibleValue(
            e->getKid(0), llvm::APInt(e->getKid(0)->getWidth(), !range.min()));
      }
      break;
    }

    case Expr::Ult: {
      BinaryExpr *be = cast<BinaryExpr>(e);

      // XXX heuristic / lossy, what order if conflict

      if (range.isFixed()) {
        ValueRange left = evalRangeForExpr(be->left);
        ValueRange right = evalRangeForExpr(be->right);

        llvm::APInt maxValue =
            llvm::APInt::getAllOnesValue(be->right->getWidth());

        // XXX should deal with overflow (can lead to empty range)

        if (left.isFixed()) {
          if (!range.min().isNullValue()) {
            propagatePossibleValues(be->right,
                                    CexValueData(left.min() + 1, maxValue));
          } else {
            propagatePossibleValues(
                be->right,
                CexValueData(llvm::APInt::getNullValue(be->right->getWidth()),
                             left.min()));
          }
        } else if (right.isFixed()) {
          if (!range.min().isNullValue()) {
            propagatePossibleValues(
                be->left,
                CexValueData(llvm::APInt::getNullValue(be->right->getWidth()),
                             right.min() - 1));
          } else {
            propagatePossibleValues(be->left,
                                    CexValueData(right.min(), maxValue));
          }
        } else {
          // XXX ???
        }
      }
      break;
    }
    case Expr::Ule: {
      BinaryExpr *be = cast<BinaryExpr>(e);

      // XXX heuristic / lossy, what order if conflict

      if (range.isFixed()) {
        ValueRange left = evalRangeForExpr(be->left);
        ValueRange right = evalRangeForExpr(be->right);

        // XXX should deal with overflow (can lead to empty range)

        llvm::APInt maxValue =
            llvm::APInt::getAllOnesValue(be->right->getWidth());
        if (left.isFixed()) {
          if (range.min().getBoolValue()) {
            propagatePossibleValues(be->right,
                                    CexValueData(left.min(), maxValue));
          } else {
            propagatePossibleValues(
                be->right,
                CexValueData(llvm::APInt::getNullValue(be->right->getWidth()),
                             left.min() - 1));
          }
        } else if (right.isFixed()) {
          if (range.min().getBoolValue()) {
            propagatePossibleValues(
                be->left,
                CexValueData(llvm::APInt::getNullValue(be->right->getWidth()),
                             right.min()));
          } else {
            propagatePossibleValues(be->left,
                                    CexValueData(right.min() + 1, maxValue));
          }
        } else {
          // XXX ???
          // TODO: we can try to order it!
        }
      }
      break;
    }

    case Expr::Ne:
    case Expr::Ugt:
    case Expr::Uge:
    case Expr::Sgt:
    case Expr::Sge:
      assert(0 && "invalid expressions (uncanonicalized");

    default:
      break;
    }
  }

  void propagateExactValues(ref<Expr> e, CexValueData range) {
    switch (e->getKind()) {
    case Expr::Constant: {
      // FIXME: Assert that range contains this constant.
      break;
    }

      // Special

    case Expr::NotOptimized:
      break;

    case Expr::Read: {
      ReadExpr *re = cast<ReadExpr>(e);
      const Array *array = re->updates.root;
      CexObjectData &cod = getObjectData(array);
      CexValueData index = evalRangeForExpr(re->index);

      for (const auto *un = re->updates.head.get(); un; un = un->next.get()) {
        CexValueData ui = evalRangeForExpr(un->index);

        // If these indices can't alias, continue propagation
        if (!ui.mayEqual(index))
          continue;

        // Otherwise if we know they alias, propagate into the write value.
        if (ui.mustEqual(index) || re->index == un->index)
          propagateExactValues(un->value, range);
        return;
      }

      // We reached the initial array write, update the exact range if possible.
      if (index.isFixed()) {
        if (ref<ConstantSource> constantSource =
                dyn_cast<ConstantSource>(array->source)) {
          // Verify the range.
          if (!isa<ConstantExpr>(array->size)) {
            assert(0 && "Unimplemented");
          }
          propagateExactValues(
              constantSource->constantValues.load(index.min().getZExtValue()),
              range);
        } else {
          CexValueData cvd = cod.getExactValues(index.min().getZExtValue());
          if (range.min().ugt(cvd.min())) {
            assert(range.min().ule(cvd.max()));
            cvd = CexValueData(range.min(), cvd.max());
          }
          if (range.max().ult(cvd.max())) {
            assert(range.max().uge(cvd.min()));
            cvd = CexValueData(cvd.min(), range.max());
          }
          cod.setExactValues(index.min().getZExtValue(), cvd);
        }
      }
      break;
    }

    case Expr::Select: {
      break;
    }

    case Expr::Concat: {
      break;
    }

    case Expr::Extract: {
      break;
    }

      // Casting

    case Expr::ZExt: {
      break;
    }

    case Expr::SExt: {
      break;
    }

      // Binary

    case Expr::And: {
      break;
    }

    case Expr::Or: {
      break;
    }

    case Expr::Xor: {
      break;
    }

      // Comparison

    case Expr::Eq: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (range.isFixed()) {
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
          if (range.min().getBoolValue()) {
            // If the equality is true, then propogate the value.
            propogateExactValue(be->right, CE->getAPValue());
          } else {
            // If the equality is false and the comparison is of booleans,
            // then we can infer the value to propogate.
            if (be->right->getWidth() == Expr::Bool) {
              propogateExactValue(
                  be->right,
                  llvm::APInt(Expr::Bool, !CE->getAPValue().getBoolValue()));
            }
          }
        }
      }
      break;
    }

    // If a boolean not, and the result is known, propagate it
    case Expr::Not: {
      if (e->getWidth() == Expr::Bool && range.isFixed()) {
        llvm::APInt propValue =
            llvm::APInt(e->getWidth(), !range.min().getBoolValue());
        propogateExactValue(e->getKid(0), propValue);
      }
      break;
    }

    case Expr::Ult: {
      break;
    }

    case Expr::Ule: {
      break;
    }

    case Expr::Ne:
    case Expr::Ugt:
    case Expr::Uge:
    case Expr::Sgt:
    case Expr::Sge:
      assert(0 && "invalid expressions (uncanonicalized");

    default:
      break;
    }
  }

  ValueRange evalRangeForExpr(const ref<Expr> &e) {
    CexRangeEvaluator ce(objects);
    return ce.evaluate(e);
  }

  /// evaluate - Try to evaluate the given expression using a consistent fixed
  /// value for the current set of possible ranges.
  ref<Expr> evaluatePossible(ref<Expr> e) {
    return CexPossibleEvaluator(objects).visit(e);
  }

  ref<Expr> evaluateExact(ref<Expr> e) {
    return CexExactEvaluator(objects).visit(e);
  }

  void dump() {
    llvm::errs() << "-- propagated values --\n";
    for (std::map<const Array *, CexObjectData *>::iterator
             it = objects.begin(),
             ie = objects.end();
         it != ie; ++it) {
      const Array *A = it->first;
      ref<ConstantExpr> arrayConstantSize = dyn_cast<ConstantExpr>(A->size);
      if (!arrayConstantSize) {
        klee_warning("Cannot dump %s as it has symbolic size\n",
                     A->getIdentifier().c_str());
      }

      CexObjectData *COD = it->second;

      llvm::errs() << A->getIdentifier() << "\n";
      llvm::errs() << "possible: [";
      for (unsigned i = 0; i < arrayConstantSize->getZExtValue(); ++i) {
        if (i)
          llvm::errs() << ", ";
        llvm::errs() << COD->getPossibleValues(i);
      }
      llvm::errs() << "]\n";
      llvm::errs() << "exact   : [";
      for (unsigned i = 0; i < arrayConstantSize->getZExtValue(); ++i) {
        if (i)
          llvm::errs() << ", ";
        llvm::errs() << COD->getExactValues(i);
      }
      llvm::errs() << "]\n";
    }
  }
};

/* *** */

class FastCexSolver : public IncompleteSolver {
public:
  FastCexSolver();
  ~FastCexSolver();

  PartialValidity computeTruth(const Query &);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<SparseStorage<unsigned char>> &values,
                            bool &hasSolution);
};

FastCexSolver::FastCexSolver() {}

FastCexSolver::~FastCexSolver() {}

/// propagateValues - propagate value ranges for the given query and return the
/// propagation results.
///
/// \param query - The query to propagate values for.
///
/// \param cd - The initial object values resulting from the propagation.
///
/// \param checkExpr - Include the query expression in the constraints to
/// propagate.
///
/// \param isValid - If the propagation succeeds (returns true), whether the
/// constraints were proven valid or invalid.
///
/// \return - True if the propogation was able to prove validity or invalidity.
static bool propagateValues(const Query &query, CexData &cd, bool checkExpr,
                            bool &isValid) {
  for (const auto &constraint : query.constraints.cs()) {
    cd.propagatePossibleValue(constraint,
                              llvm::APInt(constraint->getWidth(), 1));
    cd.propogateExactValue(constraint, llvm::APInt(constraint->getWidth(), 1));
  }
  if (checkExpr) {
    cd.propagatePossibleValue(
        query.expr, llvm::APInt::getNullValue(query.expr->getWidth()));
    cd.propogateExactValue(query.expr,
                           llvm::APInt::getNullValue(query.expr->getWidth()));
  }

  // Check the result.
  bool hasSatisfyingAssignment = true;
  if (checkExpr) {
    if (!cd.evaluatePossible(query.expr)->isFalse())
      hasSatisfyingAssignment = false;

    // If the query is known to be true, then we have proved validity.
    if (cd.evaluateExact(query.expr)->isTrue()) {
      isValid = true;
      return true;
    }
  }

  for (const auto &constraint : query.constraints.cs()) {
    if (hasSatisfyingAssignment && !cd.evaluatePossible(constraint)->isTrue())
      hasSatisfyingAssignment = false;

    // If this constraint is known to be false, then we can prove anything, so
    // the query is valid.
    if (cd.evaluateExact(constraint)->isFalse()) {
      isValid = true;
      return true;
    }
  }

  if (hasSatisfyingAssignment) {
    isValid = false;
    return true;
  }

  return false;
}

PartialValidity FastCexSolver::computeTruth(const Query &query) {
  CexData cd;

  bool isValid;
  bool success = propagateValues(query, cd, true, isValid);

  if (!success)
    return PValidity::None;

  return isValid ? PValidity::MustBeTrue : PValidity::MayBeFalse;
}

bool FastCexSolver::computeValue(const Query &query, ref<Expr> &result) {
  CexData cd;

  bool isValid;
  bool success = propagateValues(query, cd, false, isValid);

  // Check if propagation wasn't able to determine anything.
  if (!success)
    return false;

  // FIXME: We don't have a way to communicate valid constraints back.
  if (isValid)
    return false;

  // Propogation found a satisfying assignment, evaluate the expression.
  ref<Expr> value = cd.evaluatePossible(query.expr);

  if (isa<ConstantExpr>(value)) {
    // FIXME: We should be able to make sure this never fails?
    result = value;
    return true;
  } else {
    return false;
  }
}

bool FastCexSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<SparseStorage<unsigned char>> &values, bool &hasSolution) {
  CexData cd;

  bool isValid;
  bool success = propagateValues(query, cd, true, isValid);

  // Check if propagation wasn't able to determine anything.
  if (!success)
    return false;

  hasSolution = !isValid;
  if (!hasSolution)
    return true;

  // propagation found a satisfying assignment, compute the initial values.
  for (unsigned i = 0; i != objects.size(); ++i) {
    const Array *array = objects[i];
    assert(array);
    SparseStorage<unsigned char> data(0);
    ref<ConstantExpr> arrayConstantSize =
        dyn_cast<ConstantExpr>(cd.evaluatePossible(array->size));
    assert(arrayConstantSize &&
           "Array of symbolic size had not receive value for size!");

    for (unsigned i = 0; i < arrayConstantSize->getZExtValue(); i++) {
      ref<Expr> read = ReadExpr::create(
          UpdateList(array, 0), ConstantExpr::create(i, array->getDomain()));
      ref<Expr> value = cd.evaluatePossible(read);

      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
        data.store(i, ((unsigned char)CE->getZExtValue(8)));
      } else {
        // FIXME: When does this happen?
        return false;
      }
    }

    values.push_back(data);
  }

  return true;
}

class OnlyEqualityWithConstantQueryPredicate {
public:
  explicit OnlyEqualityWithConstantQueryPredicate() {}

  bool operator()(const Query &query) const {
    for (auto constraint : query.constraints.cs()) {
      if (const EqExpr *ee = dyn_cast<EqExpr>(constraint)) {
        if (!isa<ConstantExpr>(ee->left)) {
          return false;
        }
      } else {
        return false;
      }
    }
    if (ref<EqExpr> ee = dyn_cast<EqExpr>(query.negateExpr().expr)) {
      if (!isa<ConstantExpr>(ee->left)) {
        return false;
      }
    } else {
      return false;
    }
    return true;
  }
};

class TrueQueryPredicate {
public:
  explicit TrueQueryPredicate() {}

  bool operator()(const Query &query) const { return true; }
};

std::unique_ptr<Solver> klee::createFastCexSolver(std::unique_ptr<Solver> s) {
  if (FastCexFor == FastCexSolverType::EQUALITY) {
    return std::make_unique<Solver>(std::make_unique<StagedSolverImpl>(
        std::make_unique<FastCexSolver>(), std::move(s),
        OnlyEqualityWithConstantQueryPredicate()));
  } else {
    return std::make_unique<Solver>(std::make_unique<StagedSolverImpl>(
        std::make_unique<FastCexSolver>(), std::move(s), TrueQueryPredicate()));
  }
}
