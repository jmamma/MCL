/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOSEQTRACK_H__
#define LFOSEQTRACK_H__
#include "platform.h"
#include "MidiUart.h"
#include "SeqTrackModData.h"

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

using LFOSeqParam = SeqLFOParamData;

class LFOSeqTrackData {
public:
  uint64_t pattern_mask;
  LFOSeqParam params[NUM_LFO_PARAMS];

  uint8_t wav_type;
  uint8_t speed;
  uint8_t mode;
  bool enable;
  uint8_t length;

  void init() {
    enable = false;
    wav_type = 0;
    speed = 0;
    mode = 0;
    pattern_mask = 0;
    length = 16;
    for (uint8_t a = 0; a < NUM_LFO_PARAMS; a++) {
      params[a].init();
    }
  }
};

class ATTR_PACKED() LegacyLFOSeqTrackData {
public:
  LFOSeqParam params[NUM_LFO_PARAMS];
  uint8_t wav_type;
  uint8_t wav_table[NUM_LFO_PARAMS][WAV_LENGTH];
  bool wav_table_state[NUM_LFO_PARAMS];
  uint8_t last_wav_value[NUM_LFO_PARAMS];
  uint8_t sample_hold;
  uint8_t speed;
  uint8_t mode;
  uint8_t offset_behaviour;
  uint64_t pattern_mask;
  bool enable;
  uint8_t length;

  void init();
  void load_data(const SeqLFOData &data);
  void store_data(SeqLFOData *data) const;
};

class LFOSeqTrack : public LFOSeqTrackData {
public:
  uint8_t track_number;
  uint8_t device_slot;
  uint8_t step_count;
  uint8_t sample_count;
  uint8_t sample_hold;
  uint8_t last_wav_value[NUM_LFO_PARAMS];

  static uint8_t wav_tables[4][WAV_LENGTH];

  int16_t get_sample(uint8_t n);

  void load_tables();
  void init() {
    LFOSeqTrackData::init();
    track_number = 0;
    device_slot = 1;
    for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
      last_wav_value[i] = 255;
    }
    reset_runtime();
  }
  void reset_runtime();
  void load_data(const SeqLFOData &data);
  void store_data(SeqLFOData *data) const;

  uint8_t get_param_offset(uint8_t dest, uint8_t param_id);
  void reset_params();

  uint8_t get_wav_value(uint8_t sample_count, uint8_t dest, uint8_t param_id);

  void set_wav_type(uint8_t _wav_type) {
      wav_type = _wav_type;
  }
  void set_speed(uint8_t _speed) { speed = _speed; }
  void set_depth(uint8_t param, uint8_t depth) {
      params[param].depth = depth;
  }
  void load_wav_table(uint8_t table);
  void seq(MidiUartClass *uart_, MidiUartClass *uart2_);
};

#endif /* LFOSEQTRACK_H__ */
