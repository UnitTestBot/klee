#include "klee/ADT/SeedFromFile.h"
#include "klee/ADT/KTest.h"

#include <fstream>

using namespace klee;

SeedFromFile::SeedFromFile(std::string _path)
    : ktest(kTest_fromFile((_path + "ktest").c_str())) {
  std::ifstream seedInfoStream(_path + "seedinfo");
  if (seedInfoStream.good()) {
    seedInfoStream >> maxInstructions;
  }
}
