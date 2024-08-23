#ifndef KLEE_SEED_FROM_FILE_H
#include <string>

class KTest;

namespace klee {

class SeedFromFile {
public:
  KTest *ktest;
  unsigned maxInstructions;

  SeedFromFile() {}
  SeedFromFile(std::string _path);
};

} // namespace klee
#define KLEE_SEED_FROM_FILE_H
#endif