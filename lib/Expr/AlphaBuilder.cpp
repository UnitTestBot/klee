//===-- AlphaBuilder.cpp ---------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/AlphaBuilder.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/SourceBuilder.h"

#include <vector>

namespace klee {

const Array *AlphaBuilder::visitArray(const Array *arr) {
  if (!reverse && alphaArrayMap.find(arr) == alphaArrayMap.end()) {
    ref<SymbolicSource> source = arr->source;
    ref<Expr> size = visit(arr->getSize());
    if (ref<MockDeterministicSource> mockSource =
            dyn_cast_or_null<MockDeterministicSource>(source)) {
      std::vector<ref<Expr>> args;
      for (const auto &it : mockSource->args) {
        args.push_back(visit(it));
      }
      source = SourceBuilder::mockDeterministic(mockSource->km,
                                                mockSource->function, args);
      alphaArrayMap[arr] =
          Array::create(size, source, arr->getDomain(), arr->getRange());
      reverseAlphaArrayMap[alphaArrayMap[arr]] = arr;
    } else if (!arr->isConstantArray()) {
      source = SourceBuilder::alpha(index);
      index++;
      alphaArrayMap[arr] =
          Array::create(size, source, arr->getDomain(), arr->getRange());
      reverseAlphaArrayMap[alphaArrayMap[arr]] = arr;
    } else if (size != arr->getSize()) {
      alphaArrayMap[arr] =
          Array::create(size, source, arr->getDomain(), arr->getRange());
      reverseAlphaArrayMap[alphaArrayMap[arr]] = arr;
    } else {
      alphaArrayMap[arr] = arr;
      reverseAlphaArrayMap[arr] = arr;
    }
  }
  if (reverse) {
    return reverseAlphaArrayMap[arr];
  } else {
    return alphaArrayMap[arr];
  }
}

UpdateList AlphaBuilder::visitUpdateList(UpdateList u) {
  const Array *root = visitArray(u.root);
  std::vector<ref<UpdateNode>> updates;

  for (auto un = u.head; un; un = un->next) {
    updates.push_back(un);
  }

  updates.push_back(nullptr);

  for (int i = updates.size() - 2; i >= 0; i--) {
    ref<Expr> index = visit(updates[i]->index);
    ref<Expr> value = visit(updates[i]->value);
    updates[i] = new UpdateNode(updates[i + 1], index, value);
  }
  return UpdateList(root, updates[0]);
}

ExprVisitor::Action AlphaBuilder::visitRead(const ReadExpr &re) {
  ref<Expr> v = visit(re.index);
  UpdateList u = visitUpdateList(re.updates);
  ref<Expr> e = ReadExpr::create(u, v);
  return Action::changeTo(e);
}

AlphaBuilder::AlphaBuilder() {}

constraints_ty AlphaBuilder::visitConstraints(const constraints_ty &cs) {
  constraints_ty result;
  for (const auto &arg : cs) {
    ref<Expr> v = visit(arg);
    reverseExprMap[v] = arg;
    reverseExprMap[Expr::createIsZero(v)] = Expr::createIsZero(arg);
    result.insert(v);
  }
  return result;
}

ref<Expr> AlphaBuilder::build(ref<Expr> v) {
  ref<Expr> e = visit(v);
  reverseExprMap[e] = v;
  reverseExprMap[Expr::createIsZero(e)] = Expr::createIsZero(v);
  return e;
}
ref<Expr> AlphaBuilder::reverseBuild(ref<Expr> v) {
  reverse = true;
  ref<Expr> e = visit(v);
  reverse = false;
  return e;
}

} // namespace klee
