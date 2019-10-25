/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOSEQTRACK_H__
#define LFOSEQTRACK_H__
#include "WProgram.h"
#include "LFO.h"

#define NUM_OF_LFO_PARAMS 2

// LFO is free running as is never reset
#define LFO_MODE_FREE 0
// LFO resets on trig but plays continously.
#define LFO_MODE_TRIG 1
// LFO resets on trig but only plays 1 cycle
#define LFO_MODE_ONE 2

typedef struct seq_lfo_params_t {
  uint8_t dest;
  uint8_t param;
  uint8_t depth;
} seq_lfo_params_t;

class LFOSeqTrack {
public:
  uint8_t wav_table[LFO_LENGTH];
  uint8_t sample_count;
  uint8_t speed = 1;
  uint8_t mode;

  uint8_t length = 16;
  uint8_t step_count;
  uint64_t pattern_mask;

  bool enable = false;

  seq_lfo_params_t params[NUM_OF_LFO_PARAMS];
  LFOSeqTrack() { init(); };

  void init() {
    for (uint8_t a = 0; a < NUM_OF_LFO_PARAMS; a++) {
      params[a].dest = 255;
    }
  }
  ALWAYS_INLINE() void seq();
};

#endif /* LFOSEQTRACK_H__ */
