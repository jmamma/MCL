/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOSEQTRACK_H__
#define LFOSEQTRACK_H__
#include "WProgram.h"

#define NUM_LFO_PARAMS 2

// LFO is free running as is never reset
#define LFO_MODE_FREE 0
// LFO resets on trig but plays continously.
#define LFO_MODE_TRIG 1
// LFO resets on trig but only plays 1 cycle
#define LFO_MODE_ONE 2

// LFO will symmetrically oscillate around offset value
#define LFO_OFFSET_CENTRE 0

// LFO maximum value will be equal to the offset.
#define LFO_OFFSET_MAX 1

#define WAV_LENGTH 96

class LFOSeqParam {
public:
  uint8_t dest;
  uint8_t param;
  uint8_t depth;
  uint8_t offset;

  uint8_t get_param_offset(uint8_t dest, uint8_t param);
  void reset_param(uint8_t dest, uint8_t param, uint8_t value);
  void reset_param_offset();
  void update_offset();
  void update_kit();
};

class LFOSeqTrack {
public:
  uint8_t track_number;
  uint8_t wav_table[2][WAV_LENGTH];
  uint8_t sample_count;
  uint8_t sample_hold = 0;

  uint8_t speed = 0;
  uint8_t mode;
  uint8_t offset_behaviour;

  uint8_t length = 16;
  uint8_t step_count;
  uint64_t pattern_mask;

  bool enable = false;

  LFOSeqParam params[NUM_LFO_PARAMS];
  LFOSeqTrack() { init(); };
  ALWAYS_INLINE() uint8_t get_wav_value(uint8_t sample_count, uint8_t param);
  void update_kit_params();
  void update_params_offset();
  void reset_params_offset();
  void check_and_update_params_offset(uint8_t dest, uint8_t value);
  void init() {
    for (uint8_t a = 0; a < NUM_LFO_PARAMS; a++) {
      params[a].dest = 255;
    }
  }
  ALWAYS_INLINE() void seq();
};

#endif /* LFOSEQTRACK_H__ */
