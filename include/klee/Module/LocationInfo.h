////===-- LocationInfo.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_LOCATIONINFO_H
#define KLEE_LOCATIONINFO_H

#include "klee/ADT/Ref.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>

namespace llvm {
class Function;
class GlobalVariable;
class Instruction;
class Module;
} // namespace llvm

namespace klee {
struct PhysicalLocationJson;
struct LocationInfo;
} // namespace klee

namespace klee {

/// @brief Immutable struct representing location in source code.
struct LocationInfo {
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  /// @brief Path to source file for that location.
  const std::string file;

  /// @brief Code line in source file.
  const uint64_t line;

  /// @brief Column number in source file.
  const std::optional<uint64_t> column;

  /// @brief Converts location info to SARIFs representation
  /// of location.
  /// @param location location info in source code.
  /// @return SARIFs representation of location.
  PhysicalLocationJson serialize() const;

  static ref<LocationInfo> create(std::string file, uint64_t line,
                                  std::optional<uint64_t> column) {
    LocationInfo *locationInfo = new LocationInfo(file, line, column);
    return createCachedLocationInfo(locationInfo);
  }

  bool operator==(const LocationInfo &rhs) const {
    return file == rhs.file && line == rhs.line && column == rhs.column;
  }

  bool equals(const LocationInfo &b) const { return *this == b; }

  ~LocationInfo() {
    if (isCached) {
      toBeCleared = true;
      cachedLocationInfo.cache.erase(this);
    }
  }

private:
  LocationInfo(std::string file, uint64_t line, std::optional<uint64_t> column)
      : file(file), line(line), column(column) {}

  struct LocationInfoHash {
    std::size_t operator()(LocationInfo *const s) const noexcept {
      std::size_t r = 0;
      std::size_t h1 = std::hash<std::string>{}(s->file);
      std::size_t h2 = std::hash<uint64_t>{}(s->line);
      std::size_t h3 = std::hash<std::optional<uint64_t>>{}(s->column);
      r ^= h1 + 0x9e3779b9 + (r << 6) + (r >> 2);
      r ^= h2 + 0x9e3779b9 + (r << 6) + (r >> 2);
      r ^= h3 + 0x9e3779b9 + (r << 6) + (r >> 2);
      return r;
    }
  };

  struct LocationInfoCmp {
    bool operator()(LocationInfo *const a, LocationInfo *const b) const {
      return *a == *b;
    }
  };

  using CacheType =
      std::unordered_set<LocationInfo *, LocationInfoHash, LocationInfoCmp>;

  struct LocationInfoCacheSet {
    CacheType cache;
    ~LocationInfoCacheSet() {
      while (cache.size() != 0) {
        ref<LocationInfo> tmp = *cache.begin();
        tmp->isCached = false;
        cache.erase(cache.begin());
      }
    }
  };

  static LocationInfoCacheSet cachedLocationInfo;
  bool isCached = false;
  bool toBeCleared = false;

  static ref<LocationInfo>
  createCachedLocationInfo(ref<LocationInfo> locationInfo);
};

ref<LocationInfo> getLocationInfo(const llvm::Function *func);
ref<LocationInfo> getLocationInfo(const llvm::Instruction *inst);
ref<LocationInfo> getLocationInfo(const llvm::GlobalVariable *global);

} // namespace klee

#endif /* KLEE_LOCATIONINFO_H */
