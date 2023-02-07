//===-- TargetForest.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to represent prefix tree of Targets
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TARGETFOREST_H
#define KLEE_TARGETFOREST_H

#include "Target.h"
#include "TargetHash.h"
#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/KModule.h"
#include "klee/Module/Locations.h"
#include "klee/Core/TargetedExecutionReporter.h"

#include <unordered_map>
#include <vector>

namespace klee {
struct RefTargetHash;
struct RefTargetCmp;
struct TargetsHistoryHash;
struct EquivTargetsHistoryCmp;
struct TargetsHistoryCmp;
struct TargetsVectorHash;
struct TargetsVectorCmp;

class TargetForest {
public:
  using TargetsSet = std::unordered_set<ref<Target>, RefTargetHash, RefTargetCmp>;
private:
  class TargetsVector {
  public:
    explicit TargetsVector(const ref<Target>& target);
    explicit TargetsVector(const TargetsSet* targets);
    std::size_t hash() const { return hashValue; }
    bool operator==(const TargetsVector& other) const {
      return targetsVec == other.targetsVec;
    }
  private:
    std::vector<ref<Target>> targetsVec;
    std::size_t hashValue;

    void sortAndComputeHash();
  };

  struct TargetsVectorHash {
    std::size_t operator()(const TargetsVector &t) const {
      return t.hash();
    }
  };

  struct TargetsVectorCmp {
    bool operator()(const TargetsVector &a,
                    const TargetsVector &b) const {
      return a == b;
    }
  };

  class Layer {
    using InternalLayer = std::unordered_map<ref<Target>, ref<Layer>, RefTargetHash, RefTargetCmp>;
    InternalLayer forest;
    std::unordered_map<TargetsVector, ref<Layer>, TargetsVectorHash, TargetsVectorCmp> targetsABC;

    /// @brief Confidence in % that this layer (i.e., parent target node) can be reached
    confidence::ty confidence;

    Layer(const InternalLayer &forest, confidence::ty confidence) : forest(forest), confidence(confidence) {}
    explicit Layer(const Layer *layer) : Layer(layer->forest, layer->confidence) {}
    explicit Layer(const ref<Layer> layer) : Layer(layer.get()) {}
    void unionWith(Layer *other);
    void block(ref<Target> target);

    confidence::ty getConfidence(confidence::ty parentConfidence) const {
      return confidence::min(parentConfidence, confidence);
    }

    static void pathForestToTargetForest(
      Layer *self,
      PathForest *pathForest,
      std::unordered_map<LocatedEvent *, TargetsSet *> &loc2Targets,
      std::unordered_set<unsigned> &broken_traces);

    void collectHowManyEventsInTracesWereReached(
      std::unordered_map<unsigned, std::pair<unsigned, unsigned>> &trace2eventCount,
      unsigned reached,
      unsigned total) const;

  public:
    using iterator = InternalLayer::const_iterator;

    /// @brief Required by klee::ref-managed objects
    class ReferenceCounter _refCount;

    explicit Layer() : confidence(confidence::MaxConfidence) {}
    Layer(PathForest *pathForest, std::unordered_map<LocatedEvent *, TargetsSet *> &loc2Targets, std::unordered_set<unsigned> &broken_traces);

    iterator find(ref<Target> b) const { return forest.find(b); }
    iterator begin() const { return forest.begin(); }
    iterator end() const { return forest.end(); }
    void insert(ref<Target> loc, ref<Layer> nextLayer) { forest[loc] = nextLayer; }
    bool empty() const { return forest.empty(); }
    bool deepFind(ref<Target> target) const;
    bool deepFindIn(ref<Target> child, ref<Target> target) const;
    size_t size() const { return forest.size(); }
    Layer *replaceChildWith(ref<Target> child, Layer *other) const;
    Layer *removeChild(ref<Target> child) const;
    Layer *addChild(ref<Target> child) const;
    Layer *blockLeafInChild(ref<Target> child, ref<Target> leaf) const;
    Layer *blockLeaf(ref<Target> leaf) const;
    bool allNodesRefCountOne() const;
    void dump(unsigned n) const;
    void addLeafs(std::vector<std::pair<ref<Target>, confidence::ty> > *leafs,
      confidence::ty parentConfidence) const;
    void propagateConfidenceToChildren();
    void subtract_confidences_from(ref<Layer> other, confidence::ty parentConfidence);
    void addTargetWithConfidence(ref<Target> target, confidence::ty confidence);
    ref<Layer> deep_copy();
    Layer *copy();
    void divideConfidenceBy(unsigned factor);
    Layer *divideConfidenceBy(std::multiset<ref<Target> > &reachableStatesOfTarget);
    confidence::ty getConfidence() const { return getConfidence(confidence::MaxConfidence); }
    void collectHowManyEventsInTracesWereReached(std::unordered_map<unsigned, std::pair<unsigned, unsigned>> &trace2eventCount) const {
      collectHowManyEventsInTracesWereReached(trace2eventCount, 0, 0);
    }
  };

  ref<Layer> forest;

  bool allNodesRefCountOne() const;

public:
  class History {
  private:
    typedef std::unordered_set<History *, TargetsHistoryHash, EquivTargetsHistoryCmp> EquivTargetsHistoryHashSet;
    typedef std::unordered_set<History *, TargetsHistoryHash, TargetsHistoryCmp> TargetsHistoryHashSet;

    static EquivTargetsHistoryHashSet cachedHistories;
    static TargetsHistoryHashSet histories;
    unsigned hashValue;


    explicit History(ref<Target> _target, ref<History> _visitedTargets)
        : target(_target), visitedTargets(_visitedTargets) {
      computeHash();
    }

  public:
    const ref<Target> target;
    const ref<History> visitedTargets;

    static ref<History> create(ref<Target> _target, ref<History> _visitedTargets);
    static ref<History> create(ref<Target> _target);
    static ref<History> create();

    ref<History> add(ref<Target> _target) { return History::create(_target, this); }

    unsigned hash() const { return hashValue; }

    int compare(const History &h) const;
    bool equals(const History &h) const;

    void computeHash() {
      unsigned res = 0;
      if (target) {
        res = target->hash() * Expr::MAGIC_HASH_CONSTANT;
      }
      if (visitedTargets) {
        res ^= visitedTargets->hash() * Expr::MAGIC_HASH_CONSTANT;
      }
      hashValue = res;
    }

    void dump() const;

    ~History();

    /// @brief Required by klee::ref-managed objects
    class ReferenceCounter _refCount;
  };

private:
  ref<History> history;
  KFunction *entryFunction;

public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;
  unsigned getDebugReferenceCount() { return forest->_refCount.getCount(); }
  KFunction *getEntryFunction() { return entryFunction; }
  void debugStepToRandomLoc();

  TargetForest(ref<Layer> layer, KFunction *entryFunction) : forest(layer), history(History::create()), entryFunction(entryFunction) {}
  TargetForest() : TargetForest(new Layer(), nullptr) {}
  TargetForest(
    PathForest *pathForest,
    std::unordered_map<LocatedEvent *, TargetsSet *> &loc2Targets,
    std::unordered_set<unsigned> &broken_traces,
    KFunction *entryFunction);

  bool empty() const { return forest.isNull() || forest->empty(); }
  Layer::iterator begin() const { return forest->begin(); }
  Layer::iterator end() const { return forest->end(); }
  bool contains(ref<Target> b) { return forest->find(b) != forest->end(); }
  bool deepContains(ref<Target> b) { return forest->deepFind(b); }

  /// @brief Number of children of this layer (immediate successors)
  size_t successorCount() const { return forest->size(); }

  void stepTo(ref<Target>);
  void add(ref<Target>);
  void remove(ref<Target>);
  void blockIn(ref<Target>, ref<Target>);
  const ref<History> getHistory() { return history; };
  const ref<Layer> getTargets() { return forest; };
  void dump() const;
  std::vector<std::pair<ref<Target>, confidence::ty> > *leafs() const;
  void subtract_confidences_from(TargetForest &other);
  void addTargetWithConfidence(ref<Target> target, confidence::ty confidence) { forest->addTargetWithConfidence(target, confidence); }
  ref<TargetForest> deep_copy();
  void divideConfidenceBy(unsigned factor) { forest->divideConfidenceBy(factor); }
  void divideConfidenceBy(std::multiset<ref<Target> > &reachableStatesOfTarget) {
    forest = forest->divideConfidenceBy(reachableStatesOfTarget);
  }
  void collectHowManyEventsInTracesWereReached(std::unordered_map<unsigned, std::pair<unsigned, unsigned>> &trace2eventCount) const {
    forest->collectHowManyEventsInTracesWereReached(trace2eventCount);
  }
};

struct TargetsHistoryHash {
  unsigned operator()(const TargetForest::History *t) const {
    return t ? t->hash() : 0;
  }
};

struct TargetsHistoryCmp {
  bool operator()(const TargetForest::History *a,
                  const TargetForest::History *b) const {
    return a == b;
  }
};

struct EquivTargetsHistoryCmp {
  bool operator()(const TargetForest::History *a,
                  const TargetForest::History *b) const {
   if (a == NULL || b == NULL)
      return false;
    return a->compare(*b) == 0;
  }
};

struct RefTargetsHistoryHash {
  unsigned operator()(const ref<TargetForest::History> &t) const {
    return t->hash();
  }
};

struct RefTargetsHistoryCmp {
  bool operator()(const ref<TargetForest::History> &a,
                  const ref<TargetForest::History> &b) const {
    return a.get() == b.get();
  }
};

} // End klee namespace

#endif /* KLEE_TARGETFOREST_H */
