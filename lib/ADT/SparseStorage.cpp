#include "klee/ADT/SparseStorage.h"

#include "llvm/Support/raw_ostream.h"

namespace klee {

template <>
void SparseStorage<unsigned char>::print(llvm::raw_ostream &os) const {
  if (internalStorage.size() * 2 < sizeOfSetRange()) {
    // "Sparse representation"
    os << "[";
    bool firstPrinted = false;
    for (const auto &element : internalStorage) {
      if (firstPrinted) {
        os << ", ";
      }
      os << element.first << ": " << element.second;
      firstPrinted = true;
    }
    os << "] DV: ";
    os << defaultValue;
  } else {
    // "Dense representation"
    os << "[";
    bool firstPrinted = false;
    for (size_t i = 0; i < sizeOfSetRange(); i++) {
      if (firstPrinted) {
        os << ", ";
      }
      os << load(i);
      firstPrinted = true;
    }
    os << "] DV: ";
    os << defaultValue;
  }
}
}
