//===-- Memory.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Memory.h"

#include "ExecutionState.h"
#include "MemoryManager.h"
#include "klee/Core/Context.h"

#include "klee/ADT/BitArray.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KType.h"
#include "klee/Solver/Solver.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <cassert>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace {
// cl::opt<bool>
//     UseConstantArrays("use-constant-arrays",
//                       cl::desc("Use constant arrays instead of updates when "
//                                "possible (default=true)\n"),
//                       cl::init(true), cl::cat(SolvingCat));
}

/***/

IDType MemoryObject::counter = 1;
int MemoryObject::time = 0;

MemoryObject::~MemoryObject() {
  if (parent)
    parent->markFreed(this);
}

void MemoryObject::getAllocInfo(std::string &result) const {
  llvm::raw_string_ostream info(result);

  info << "MO" << id << "[" << size << "]";

  if (allocSite) {
    info << " allocated at ";
    if (const Instruction *i = dyn_cast<Instruction>(allocSite)) {
      info << i->getParent()->getParent()->getName() << "():";
      info << *i;
    } else if (const GlobalValue *gv = dyn_cast<GlobalValue>(allocSite)) {
      info << "global:" << gv->getName();
    } else {
      info << "value:" << *allocSite;
    }
  } else {
    info << " (no allocation info)";
  }

  info.flush();
}

/***/

ObjectState::ObjectState(const MemoryObject *mo, KType *dt)
    : copyOnWriteOwner(0), object(mo), concreteStore(0), concreteMask(true),
      knownSymbolics(nullptr), unflushedMask(false), updates(nullptr, nullptr),
      lastUpdate(nullptr), dynamicType(dt), readOnly(false) {
  // if (!UseConstantArrays) {
  //   static unsigned id = 0;
  //   const Array *array = getArrayCache()->CreateArray(
  //       mo->getSizeExpr(), SourceBuilder::makeSymbolic("tmp_arr", ++id));
  //   updates = UpdateList(array, 0);
  // }
}

ObjectState::ObjectState(const Array *array, KType *dt)
    : copyOnWriteOwner(0), object(nullptr), concreteStore(0),
      concreteMask(false), knownSymbolics(nullptr), unflushedMask(false),
      updates(array, nullptr), lastUpdate(nullptr), dynamicType(dt),
      readOnly(false) {}

ObjectState::ObjectState(const MemoryObject *mo, const Array *array, KType *dt)
    : copyOnWriteOwner(0), object(mo), concreteStore(0), concreteMask(false),
      knownSymbolics(nullptr), unflushedMask(false), updates(array, nullptr),
      lastUpdate(nullptr), dynamicType(dt), readOnly(false) {}

ObjectState::ObjectState(const ObjectState &os)
    : copyOnWriteOwner(0), object(os.object), concreteStore(os.concreteStore),
      concreteMask(os.concreteMask), knownSymbolics(os.knownSymbolics),
      unflushedMask(os.unflushedMask), updates(os.updates),
      wasZeroInitialized(os.wasZeroInitialized), lastUpdate(os.lastUpdate),
      dynamicType(os.dynamicType), readOnly(os.readOnly) {}

ArrayCache *ObjectState::getArrayCache() const {
  assert(object && "object was NULL");
  return object->parent->getArrayCache();
}

/***/

const UpdateList &ObjectState::getUpdates() const {
  // Constant arrays are created lazily.
  if (!updates.root) {
    // Collect the list of writes, with the oldest writes first.

    // FIXME: We should be able to do this more efficiently, we just need to be
    // careful to get the interaction with the cache right. In particular we
    // should avoid creating UpdateNode instances we never use.
    unsigned NumWrites = updates.head ? updates.head->getSize() : 0;
    std::vector<std::pair<ref<Expr>, ref<Expr>>> Writes(NumWrites);
    const auto *un = updates.head.get();
    for (unsigned i = NumWrites; i != 0; un = un->next.get()) {
      --i;
      Writes[i] = std::make_pair(un->index, un->value);
    }

    /* For objects of symbolic size we will leave last constant
    sizes for every index and create constant array (in terms of
    Z3 solver) filled with zeros. This part is required for reads
    from unitialzed memory. */
    SparseStorage<ref<ConstantExpr>> Contents(
        ConstantExpr::create(concreteStore.defaultV(), Expr::Int8));
    // std::vector<ref<ConstantExpr>> Contents(size);

    // Initialize to zeros.
    // for (unsigned i = 0, e = size; i != e; ++i)
    //   Contents[i] = ConstantExpr::create(0, Expr::Int8);

    // Pull off as many concrete writes as we can.
    unsigned Begin = 0, End = Writes.size();
    for (; Begin != End; ++Begin) {
      // Push concrete writes into the constant array.
      ConstantExpr *Index = dyn_cast<ConstantExpr>(Writes[Begin].first);
      if (!Index)
        break;

      ConstantExpr *Value = dyn_cast<ConstantExpr>(Writes[Begin].second);
      if (!Value)
        break;

      Contents.store(Index->getZExtValue(), Value);
    }

    static unsigned id = 0;
    std::string arrayName = "const_arr" + llvm::utostr(++id);
    const Array *array = nullptr;

    if (object->hasSymbolicSize()) {
      /* Extend updates with last written non-zero constant values.
      ConstantValues must be empty in constant array. */
      array = getArrayCache()->CreateArray(
          object->getSizeExpr(),
          SourceBuilder::symbolicSizeConstant(concreteStore.defaultV()));
      updates = UpdateList(array, 0);
      for (auto entry : Contents.storage()) {
        updates.extend(ConstantExpr::create(entry.first, Expr::Int32), entry.second);
      }
    } else {
      std::vector<ref<ConstantExpr>> FixedSizeContents(object->size);
      for (size_t i = 0; i < object->size; i++) {
        FixedSizeContents[i] = Contents.load(i);
      }
      array = getArrayCache()->CreateArray(
          object->getSizeExpr(), SourceBuilder::constant(FixedSizeContents));
      updates = UpdateList(array, 0);
    }

    // Apply the remaining (non-constant) writes.
    for (; Begin != End; ++Begin)
      updates.extend(Writes[Begin].first, Writes[Begin].second);
  }

  return updates;
}

void ObjectState::flushToConcreteStore(TimingSolver *solver,
                                       const ExecutionState &state) const {
  for (auto i : knownSymbolics.storage()) {
    ref<ConstantExpr> ce;
    bool success = solver->getValue(state.constraints.cs(), read8(i.first), ce,
                                    state.queryMetaData);
    if (!success) {
      klee_warning("Solver timed out when getting a value for external call, "
                   "byte %p+%lu will have random value",
                   (void *)object->address, i.first);
    }
    assert(ce->getWidth() == Expr::Int8);
    uint8_t value = ce->getZExtValue(8);
    concreteStore.store(i.first, value);
  }
}

void ObjectState::makeConcrete() {
  concreteMask.reset(true);
  knownSymbolics.reset(nullptr);
}

void ObjectState::initializeToZero() {
  makeConcrete();
  wasZeroInitialized = true;
  concreteStore.reset(0);
}

void ObjectState::initializeToRandom() {
  makeConcrete();
  wasZeroInitialized = false;
  concreteStore.reset(0xAB);
}

/*
Cache Invariants
--
isByteKnownSymbolic(i) => !isByteConcrete(i)
isByteConcrete(i) => !isByteKnownSymbolic(i)
isByteUnflushed(i) => (isByteConcrete(i) || isByteKnownSymbolic(i))
 */

void ObjectState::flushForRead() const {
  for (const auto &unflushed : unflushedMask.storage()) {
    if (!unflushed.second) {
      continue;
    }
    auto offset = unflushed.first;
    if (isByteConcrete(offset)) {
      updates.extend(
          ConstantExpr::create(offset, Expr::Int32),
          ConstantExpr::create(concreteStore.load(offset), Expr::Int8));
    } else {
      assert(isByteKnownSymbolic(offset) && "invalid bit set in unflushedMask");
      updates.extend(ConstantExpr::create(offset, Expr::Int32),
                     knownSymbolics.load(offset));
    }
  }
  unflushedMask.reset(false);
}

void ObjectState::flushForWrite() {
  flushForRead();
  concreteMask.reset(false);
  knownSymbolics.reset(nullptr);
}

bool ObjectState::isByteConcrete(unsigned offset) const {
  return concreteMask.load(offset);
}

bool ObjectState::isByteUnflushed(unsigned offset) const {
  return unflushedMask.load(offset);
}

bool ObjectState::isByteKnownSymbolic(unsigned offset) const {
  return knownSymbolics.load(offset).get();
}

void ObjectState::markByteConcrete(unsigned offset) {
  concreteMask.store(offset, true);
}

void ObjectState::markByteSymbolic(unsigned offset) {
  concreteMask.store(offset, false);
}

void ObjectState::markByteUnflushed(unsigned offset) {
  unflushedMask.store(offset, true);
}

void ObjectState::markByteFlushed(unsigned offset) {
  unflushedMask.store(offset, false);
}

void ObjectState::setKnownSymbolic(unsigned offset,
                                   Expr *value /* can be null */) {
  knownSymbolics.store(offset, value);
}

/***/

ref<Expr> ObjectState::read8(unsigned offset) const {
  if (isByteConcrete(offset)) {
    return ConstantExpr::create(concreteStore.load(offset), Expr::Int8);
  } else if (isByteKnownSymbolic(offset)) {
    return knownSymbolics.load(offset);
  } else {
    assert(!isByteUnflushed(offset) && "unflushed byte without cache value");

    return ReadExpr::create(getUpdates(),
                            ConstantExpr::create(offset, Expr::Int32));
  }
}

ref<Expr> ObjectState::read8(ref<Expr> offset) const {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic read8");
  flushForRead();

  if (object && object->size > 4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(
        nullptr,
        "Symbolic memory access will send the following array of %d bytes to "
        "the constraint solver -- large symbolic arrays may cause significant "
        "performance issues: %s",
        object->size, allocInfo.c_str());
  }

  return ReadExpr::create(getUpdates(), ZExtExpr::create(offset, Expr::Int32));
}

void ObjectState::write8(unsigned offset, uint8_t value) {
  // assert(read_only == false && "writing to read-only object!");
  concreteStore.store(offset, value);
  setKnownSymbolic(offset, 0);

  markByteConcrete(offset);
  markByteUnflushed(offset);
}

void ObjectState::write8(unsigned offset, ref<Expr> value) {
  // can happen when ExtractExpr special cases
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    write8(offset, (uint8_t)CE->getZExtValue(8));
  } else {
    setKnownSymbolic(offset, value.get());

    markByteSymbolic(offset);
    markByteUnflushed(offset);
  }
}

void ObjectState::write8(ref<Expr> offset, ref<Expr> value) {
  assert(!isa<ConstantExpr>(offset) &&
         "constant offset passed to symbolic write8");
  flushForWrite();

  if (object && object->size > 4096) {
    std::string allocInfo;
    object->getAllocInfo(allocInfo);
    klee_warning_once(
        nullptr,
        "Symbolic memory access will send the following array of %d bytes to "
        "the constraint solver -- large symbolic arrays may cause significant "
        "performance issues: %s",
        object->size, allocInfo.c_str());
  }

  updates.extend(ZExtExpr::create(offset, Expr::Int32), value);
}

/***/

ref<Expr> ObjectState::read(ref<Expr> offset, Expr::Width width) const {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for reads at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset))
    return read(CE->getZExtValue(32), width);

  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  if (lastUpdate && lastUpdate->index == offset &&
      lastUpdate->value->getWidth() == width)
    return lastUpdate->value;

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte =
        read8(AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)));
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

ref<Expr> ObjectState::read(unsigned offset, Expr::Width width) const {
  // Treat bool specially, it is the only non-byte sized write we allow.
  if (width == Expr::Bool)
    return ExtractExpr::create(read8(offset), 0, Expr::Bool);

  // Otherwise, follow the slow general case.
  unsigned NumBytes = width / 8;
  assert(width == NumBytes * 8 && "Invalid width for read size!");
  ref<Expr> Res(0);
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    ref<Expr> Byte = read8(offset + idx);
    Res = i ? ConcatExpr::create(Byte, Res) : Byte;
  }

  return Res;
}

void ObjectState::write(ref<Expr> offset, ref<Expr> value) {
  // Truncate offset to 32-bits.
  offset = ZExtExpr::create(offset, Expr::Int32);

  // Check for writes at constant offsets.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(offset)) {
    write(CE->getZExtValue(32), value);
    return;
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  unsigned NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(AddExpr::create(offset, ConstantExpr::create(idx, Expr::Int32)),
           ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
  lastUpdate = new UpdateNode(nullptr, offset, value);
}

void ObjectState::write(unsigned offset, ref<Expr> value) {
  // Check for writes of constant values.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    Expr::Width w = CE->getWidth();
    if (w <= 64 && klee::bits64::isPowerOfTwo(w)) {
      uint64_t val = CE->getZExtValue();
      switch (w) {
      default:
        assert(0 && "Invalid write size!");
      case Expr::Bool:
      case Expr::Int8:
        write8(offset, val);
        return;
      case Expr::Int16:
        write16(offset, val);
        return;
      case Expr::Int32:
        write32(offset, val);
        return;
      case Expr::Int64:
        write64(offset, val);
        return;
      }
    }
  }

  // Treat bool specially, it is the only non-byte sized write we allow.
  Expr::Width w = value->getWidth();
  if (w == Expr::Bool) {
    write8(offset, ZExtExpr::create(value, Expr::Int8));
    return;
  }

  // Otherwise, follow the slow general case.
  unsigned NumBytes = w / 8;
  assert(w == NumBytes * 8 && "Invalid write size!");
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, ExtractExpr::create(value, 8 * i, Expr::Int8));
  }
}

void ObjectState::write16(unsigned offset, uint16_t value) {
  unsigned NumBytes = 2;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::write32(unsigned offset, uint32_t value) {
  unsigned NumBytes = 4;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::write64(unsigned offset, uint64_t value) {
  unsigned NumBytes = 8;
  for (unsigned i = 0; i != NumBytes; ++i) {
    unsigned idx = Context::get().isLittleEndian() ? i : (NumBytes - i - 1);
    write8(offset + idx, (uint8_t)(value >> (8 * i)));
  }
}

void ObjectState::print() const {
  llvm::errs() << "-- ObjectState --\n";
  llvm::errs() << "\tMemoryObject ID: " << object->id << "\n";
  llvm::errs() << "\tRoot Object: " << updates.root << "\n";
  llvm::errs() << "\tSize: " << object->size << "\n";

  llvm::errs() << "\tBytes:\n";
  for (unsigned i = 0; i < object->size; i++) {
    llvm::errs() << "\t\t[" << i << "]"
                 << " concrete? " << isByteConcrete(i) << " known-sym? "
                 << isByteKnownSymbolic(i) << " unflushed? "
                 << isByteUnflushed(i) << " = ";
    ref<Expr> e = read8(i);
    llvm::errs() << e << "\n";
  }

  llvm::errs() << "\tUpdates:\n";
  for (const auto *un = updates.head.get(); un; un = un->next.get()) {
    llvm::errs() << "\t\t[" << un->index << "] = " << un->value << "\n";
  }
}

KType *ObjectState::getDynamicType() const { return dynamicType; }

bool ObjectState::isAccessableFrom(KType *accessingType) const {
  return !UseTypeBasedAliasAnalysis ||
         dynamicType->isAccessableFrom(accessingType);
}
