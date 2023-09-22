#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <vector>

namespace llvm {
class raw_ostream;
};

namespace klee {

template <typename ValueType> class SparseStorage {
private:
  std::map<size_t, ValueType> internalStorage;
  ValueType defaultValue;

  bool contains(size_t key) const { return internalStorage.count(key) != 0; }

public:
  SparseStorage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {}

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      internalStorage[idx] = values[idx];
    }
  }

  void store(size_t idx, const ValueType &value) {
      internalStorage[idx] = value;
  }

  template <typename InputIterator>
  void store(size_t idx, InputIterator iteratorBegin,
             InputIterator iteratorEnd) {
    for (; iteratorBegin != iteratorEnd; ++iteratorBegin, ++idx) {
      store(idx, *iteratorBegin);
    }
  }

  ValueType load(size_t idx) const {
    return contains(idx) ? internalStorage.at(idx) : defaultValue;
  }

  size_t sizeOfSetRange() const {
    return internalStorage.empty() ? 0 : internalStorage.rbegin()->first + 1;
  }

  bool operator==(const SparseStorage<ValueType> &another) const {
    return defaultValue == another.defaultValue &&
           internalStorage == another.internalStorage;
  }

  bool operator!=(const SparseStorage<ValueType> &another) const {
    return !(*this == another);
  }

  bool operator<(const SparseStorage &another) const {
    return internalStorage < another.internalStorage;
  }

  bool operator>(const SparseStorage &another) const {
    return internalStorage > another.internalStorage;
  }

  const std::map<size_t, ValueType> &storage() const {
    return internalStorage;
  };
  const ValueType &defaultV() const { return defaultValue; };

  void reset() {
    internalStorage.clear();
  }

  void reset(ValueType newDefault) {
    defaultValue = newDefault;
    internalStorage.clear();
  }

  // Get values in range [0, n) as vector
  std::vector<ValueType> rangeAsVector(size_t n) const {
    std::vector<ValueType> range(n);
    for (size_t i = 0; i < n; i++) {
      range[i] = load(i);
    }
    return range;
  }

  // Specialized for unsigned char in .cpp
  void print(llvm::raw_ostream &os) const;
};

template <typename U>
SparseStorage<unsigned char> sparseBytesFromValue(const U &value) {
  const unsigned char *valueUnsignedCharIterator =
      reinterpret_cast<const unsigned char *>(&value);
  SparseStorage<unsigned char> result(sizeof(value));
  result.store(0, valueUnsignedCharIterator,
               valueUnsignedCharIterator + sizeof(value));
  return result;
}

} // namespace klee

#endif
