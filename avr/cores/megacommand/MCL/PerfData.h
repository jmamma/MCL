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

class PerfData {
public:
  PerfParam params[NUM_PERF_PARAMS];

  PerfData() { init(); }

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
