/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOSEQTRACK_H__
#define LFOSEQTRACK_H__
#include "WProgram.h"
#include "LFO.h"

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
  uint8_t wav_type;
  uint8_t wav_table[NUM_LFO_PARAMS][WAV_LENGTH];
  bool wav_table_state[NUM_LFO_PARAMS];
  uint8_t last_wav_value[NUM_LFO_PARAMS];
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
  ALWAYS_INLINE() uint8_t get_wav_value(uint8_t sample_count, uint8_t param) {
    int8_t offset = params[param].offset;
    int8_t depth = params[param].depth;
    int8_t sample = wav_table[param][sample_count];
    uint16_t val;

    switch (offset_behaviour) {
    case LFO_OFFSET_CENTRE:
      val = offset + (sample - (depth / 2));
      if (val > 127) {
        return 127;
      }
      if (val < 0) {
        return 0;
      } else {
        return (uint8_t)val;
      }
      break;
    case LFO_OFFSET_MAX:
      // val = 127 - sample;
      val = offset - depth + sample;
      if (val > 127) {
        return 127;
      }
      if (val < 0) {
        return 0;
      } else {
        return (uint8_t)val;
      }
      break;
    }
    return offset;
  }

  void update_kit_params();
  void update_params_offset();
  void reset_params_offset();

  bool wav_table_up_to_date(uint8_t n) { return wav_table_state[n]; }

  void check_and_update_params_offset(uint8_t dest, uint8_t param,
                                      uint8_t value);
  void init() {
    for (uint8_t a = 0; a < NUM_LFO_PARAMS; a++) {
      last_wav_value[a] = 255;
      params[a].dest = 255;
    }
  }
  void set_wav_type(uint8_t _wav_type) {
    if (wav_type != _wav_type) {
      wav_type = _wav_type;
      wav_table_state[0] = false;
      wav_table_state[1] = false;
    }
  }
  void set_speed(uint8_t _speed) { speed = _speed; }
  void set_depth(uint8_t param, uint8_t depth) {
    if (params[param].depth != depth) {
      params[param].depth = depth;
      wav_table_state[param] = false;
    }
  }
  void load_wav_table(uint8_t table);
  ALWAYS_INLINE() void seq() {

    if ((MidiClock.mod12_counter == 0) && (mode != LFO_MODE_FREE) &&
        IS_BIT_SET64(pattern_mask, step_count)) {
      sample_count = 0;
    }
    if (enable && (MidiUart.uart_block == 0)) {
      for (uint8_t i = 0; i < NUM_LFO_PARAMS; i++) {
        uint8_t wav_value = get_wav_value(sample_count, i);
        if (last_wav_value[i] != wav_value) {

          if (params[i].dest > 0) {
            // MD CC LFO
            if (params[i].dest <= NUM_MD_TRACKS) {
              MD.setTrackParam_inline(params[i].dest - 1, params[i].param,
                                      wav_value);
            }
            // MD FX LFO
            else {
              MD.sendFXParam(params[i].param, wav_value,
                             MD_FX_ECHO + params[i].dest - NUM_MD_TRACKS - 1);
            }
            last_wav_value[i] = wav_value;
          }
        }
      }
    }

    if (speed == 0) {
      sample_count += 2;
    } else {
      sample_hold += 1;
      if (sample_hold >= speed - 1) {
        sample_hold = 0;
        sample_count += 1;
      }
    }
    if (sample_count > LFO_LENGTH) {
      // Free running LFO should reset, oneshot should hold at last value.
      if (mode == LFO_MODE_ONE) {
        sample_count = LFO_LENGTH - 1;
      } else {
        sample_count = 0;
      }
    }

    if (MidiClock.mod12_counter == 11) {
      if (step_count == length - 1) {
        step_count = 0;
      } else {
        step_count++;
      }
    }
  }
};

#endif /* LFOSEQTRACK_H__ */
