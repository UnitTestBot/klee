// -*- C++ -*-
#ifndef KLEE_TICKER_H
#define KLEE_TICKER_H

#include <cassert>
#include <vector>

class Ticker {
  std::vector<unsigned> ticks;
  unsigned index = 0;
  unsigned counter = 0;

public:
  Ticker(std::vector<unsigned> ticks) : ticks(ticks) {
    bool atLeastOneNonZero = false;
    for (auto i : ticks) {
      if (i > 0) {
        atLeastOneNonZero = true;
      }
    }
    assert(atLeastOneNonZero);
    while (ticks[index] == 0) {
      index += 1;
    }
  }

  unsigned getCurrent() {
    unsigned current = index;
    counter += 1;
    if (counter == ticks[index]) {
      moveToNext();
    }
    return current;
  }

  void moveToNext() {
    assert(ticks[index] != 0);

    if (counter != 0) {
      index = (index + 1) % ticks.size();
      counter = 0;
    }

    while (ticks[index] == 0) {
      index = (index + 1) % ticks.size();
    }
  }

  const std::vector<unsigned> &getTicks() { return ticks; }
};

#endif
