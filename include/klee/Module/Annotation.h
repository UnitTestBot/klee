#ifndef KLEE_ANNOTATION_H
#define KLEE_ANNOTATION_H

#include "map"
#include "set"
#include "string"
#include "vector"

#include "nlohmann/json.hpp"
#include "nonstd/optional.hpp"

#include "klee/Config/config.h"

#include "llvm/IR/Module.h"

using nonstd::nullopt;
using nonstd::optional;
using json = nlohmann::json;

namespace klee {

namespace Statement {
enum class Kind {
  Unknown,

  Deref,
  InitNull,
  // TODO: rename to alloc
  AllocSource,
  Free
};

enum class Property {
  Unknown,

  Deterministic,
  Noreturn,
};

struct Unknown {
protected:
  std::string rawAnnotation;
  std::string rawOffset;
  std::string rawValue;

public:
  std::vector<std::string> offset;

  explicit Unknown(const std::string &str = "Unknown");
  virtual ~Unknown();

  virtual bool operator==(const Unknown &other) const;
  [[nodiscard]] virtual Kind getKind() const;

  [[nodiscard]] const std::vector<std::string> &getOffset() const;
  [[nodiscard]] std::string toString() const;
};

struct Deref final : public Unknown {
  explicit Deref(const std::string &str = "Deref");

  [[nodiscard]] Kind getKind() const override;
};

struct InitNull final : public Unknown {
  explicit InitNull(const std::string &str = "InitNull");

  [[nodiscard]] Kind getKind() const override;
};

struct AllocSource final : public Unknown {
public:
  enum Type {
    Alloc = 1,       // malloc, calloc, realloc
    New = 2,         // operator new
    NewBrackets = 3, // operator new[]
    OpenFile = 4,    // open file (fopen, open)
    MutexLock = 5    // mutex lock (pthread_mutex_lock)
  };

  Type value;

  explicit AllocSource(const std::string &str = "AllocSource::1");

  [[nodiscard]] Kind getKind() const override;
};

struct Free final : public Unknown {
public:
  enum Type {
    Free_ = 1,          // Kind of free function
    Delete = 2,         // operator delete
    DeleteBrackets = 3, // operator delete[]
    CloseFile = 4,      // close file
    MutexUnlock = 5     // mutex unlock (pthread_mutex_unlock)
  };

  Type value;

  explicit Free(const std::string &str = "FreeSource::1");

  [[nodiscard]] Kind getKind() const override;
};

using Ptr = std::shared_ptr<Unknown>;
bool operator==(const Ptr &first, const Ptr &second);
} // namespace Statement

// Annotation format: https://github.com/UnitTestBot/klee/discussions/92
struct Annotation {
  std::string functionName;
  std::vector<Statement::Ptr> returnStatements;
  std::vector<std::vector<Statement::Ptr>> argsStatements;
  std::set<Statement::Property> properties;

  bool operator==(const Annotation &other) const;
};

using AnnotationsMap = std::map<std::string, Annotation>;

AnnotationsMap parseAnnotationsJson(const json &annotationsJson);
AnnotationsMap parseAnnotations(const std::string &path, const llvm::Module *m);
} // namespace klee

#endif // KLEE_ANNOTATION_H
