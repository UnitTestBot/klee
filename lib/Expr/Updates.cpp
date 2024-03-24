//===-- Updates.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Expr.h"

#include <cassert>
#include <cwchar>

using namespace klee;

///

UpdateNode::UpdateNode(const ref<UpdateNode> &next, const SimpleWrite &write)
    : next(next), write(write) {
  computeHash();
  computeHeight();
  size = next ? next->size + 1 : 1;
}

UpdateNode::UpdateNode(const ref<UpdateNode> &next, const RangeWrite &write)
    : next(next), write(write) {
  computeHash();
  computeHeight();
  size = next ? next->size + write.rangeList.getSize() : write.rangeList.getSize();
}

extern "C" void vc_DeleteExpr(void *);

int UpdateNode::compare(const UpdateNode &b) const {
  if (write == b.write) {
    return 0;
  } else {
    return write < b.write ? -1 : 1;
  }
}

bool UpdateNode::equals(const UpdateNode &b) const { return compare(b) == 0; }

unsigned UpdateNode::computeHash() {
  hashValue = isSimple() ? asSimple()->hash() : asRange()->hash(); // Correct?
  if (next)
    hashValue ^= next->hash();
  return hashValue;
}

unsigned UpdateNode::computeHeight() {
  unsigned maxHeight = next ? next->height() : 0;
  if (isSimple()) {
    maxHeight = std::max(maxHeight, asSimple()->index->height());
    maxHeight = std::max(maxHeight, asSimple()->value->height());
  } else {
    maxHeight = std::max(maxHeight, asRange()->rangeList.height());
  }
  heightValue = maxHeight;
  return heightValue;
}

///

UpdateList::UpdateList(const Array *_root, const ref<UpdateNode> &_head)
    : root(_root), head(_head) {}

unsigned UpdateList::getSize() const { return head ? head->getSize() : 0; }

void UpdateList::extend(const SimpleWrite &write) {
  if (root) {
    assert(root->getDomain() == write.index->getWidth());
    assert(root->getRange() == write.value->getWidth());
  }

  head = new UpdateNode(head, write);
}

void UpdateList::extend(const RangeWrite &write) {
  isSimple = false;
  head = new UpdateNode(head, write);
}

void UpdateList::extend(const Write &write) {
  if (auto simple = std::get_if<SimpleWrite>(&write)) {
    extend(*simple);
  } else if (auto range = std::get_if<RangeWrite>(&write)) {
    extend(*range);
  } else {
    assert(0);
  }
}

ReadRanges UpdateList::flatten() const {
  ReadRanges result;
  std::vector<Write> writes;

  auto un = head.get();

  for (; un; un = un->next.get()) {
    if (un->isSimple()) {
      writes.push_back(un->write);
    } else {
      auto write = un->asRange();
      auto sublists = write->rangeList.flatten();

      for (auto &sublist : sublists) {
        sublist.first = ExprLambda::merge(sublist.first, write->guard);
        auto &list = std::get<UpdateList>(sublist.second);
        for (auto it = writes.rbegin(); it != writes.rend(); ++it) {
          list.extend(*it);
        }
        result.push_back(sublist);
      }
    }
  }

  auto newList = UpdateList(root, nullptr);
  for (auto it = writes.rbegin(); it != writes.rend(); ++it) {
    newList.extend(*it);
  }

  result.push_back({ExprLambda::constantTrue(), newList});

  return result;
}

int UpdateList::compare(const UpdateList &b) const {
  if (root->source != b.root->source)
    return root->source < b.root->source ? -1 : 1;

  // Check the root itself in case we have separate objects with the
  // same name.
  if (root != b.root)
    return root < b.root ? -1 : 1;

  if (getSize() < b.getSize())
    return -1;
  else if (getSize() > b.getSize())
    return 1;

  // XXX build comparison into update, make fast
  const auto *an = head.get(), *bn = b.head.get();
  for (; an && bn; an = an->next.get(), bn = bn->next.get()) {
    if (an == bn) { // exploit shared list structure
      return 0;
    } else {
      if (int res = an->compare(*bn))
        return res;
    }
  }
  assert(!an && !bn);
  return 0;
}

unsigned UpdateList::hash() const {
  unsigned res = 0;
  res = (res * Expr::MAGIC_HASH_CONSTANT) + root->source->hash();
  if (head)
    res = (res * Expr::MAGIC_HASH_CONSTANT) + head->hash();
  return res;
}

unsigned UpdateList::height() const { return head ? head->height() : 0; }
