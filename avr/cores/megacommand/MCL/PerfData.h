/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PERFDATATRACK_H__
#define PERFDATATRACK_H__
#include "WProgram.h"
#include "PerfData.h"

#define NUM_PERF_PARAMS 8
#define PERF_SETTINGS NUM_NUM_PERF_PARAMS
class PerfParam {
public:
  uint8_t dest;
  uint8_t param;
  uint8_t min;
  uint8_t max;
};

#define LEARN_MIN 1

class PerfData {
public:
  PerfParam params[NUM_PERF_PARAMS];
  PerfParam src_param;

  PerfData() { init(); }

  uint8_t add_param(uint8_t dest, uint8_t param, uint8_t learn, uint8_t value) {
    uint8_t match = 255;
    uint8_t empty = 255;

    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      //Find first empty
      if (params[a].dest == 0) { empty = min(a,empty); }
      if (params[a].dest == dest + 1 && params[a].param == param) {
        //Update existing, if matches
        match = a;
      }
    }

    uint8_t b = match;
    if (b == 255) { b = empty; }
    if (b == 255) { return 255; }

    params[b].dest = dest + 1;
    params[b].param = param;

    if (learn == LEARN_MIN) {
      params[b].min = value;
    }
    else {
      params[b].max = value;
    }
    return b;
  }

  void *data() const { return (void *)&params; }
  void init() {
    for (uint8_t a = 0; a < NUM_PERF_PARAMS; a++) {
      params[a].dest = 0;
      params[a].param = 0;
      params[a].min = 0;
      params[a].max = 127;
    }
  }
};

#endif /* PERFDATATRACK_H__ */
