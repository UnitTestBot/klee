// -*- C++ -*-
#ifndef KLEE_TICKER_H
#define KLEE_TICKER_H

#include <vector>

class Ticker {
  std::vector<unsigned> ticks;
  unsigned index = 0;
  unsigned counter = 0;

public:
  Ticker(std::vector<unsigned> ticks) : ticks(ticks) {}

  unsigned getCurrent() {
    unsigned current = index;
    counter += 1;
    if (counter == ticks[index]) {
      index = (index + 1) % ticks.size();
      counter = 0;
    }
    return current;
  }

  void moveToNext() {
    if (counter != 0) {
      index = (index + 1) % ticks.size();
      counter = 0;
    }
  }
};

#endif
