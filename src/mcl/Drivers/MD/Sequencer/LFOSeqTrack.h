/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOSEQTRACK_H__
#define LFOSEQTRACK_H__
#include "platform.h"
#include "../../DeviceContext.h"
#include "LFOShapes.h"
#include "MidiUart.h"
#include "SeqTrackModData.h"

#define NUM_LFO_PARAMS 2

// LFO is free running as is never reset
#define LFO_MODE_FREE 0
// LFO resets on trig but plays continously.
#define LFO_MODE_TRIG 1
// LFO resets on trig but only plays 1 cycle
#define LFO_MODE_ONE 2
// LFO resets when the paired sequencer track fires a trig.
#define LFO_MODE_TRACK_TRIG 3
#define LFO_MODE_MASK 0x03
#define LFO_SPEED_MULT_SHIFT 2
#define LFO_SPEED_MULT_MASK 0x07

#define LFO_SPEED_MULT_1X 0
#define LFO_SPEED_MULT_2X 1
#define LFO_SPEED_MULT_4X 2
#define LFO_SPEED_MULT_8X 3
#define LFO_SPEED_MULT_1_2X 4
#define LFO_SPEED_MULT_1_4X 5
#define LFO_SPEED_MULT_1_10X 6
#define LFO_SPEED_MULT_1_100X 7
#define LFO_SPEED_MULT_COUNT 8

// LFO will symmetrically oscillate around offset value
#define LFO_OFFSET_CENTRE 0

// LFO maximum value will be equal to the offset.
#define LFO_OFFSET_MAX 1

#define WAV_LENGTH LFO_LENGTH

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
  uint8_t legacy_runtime_padding[NUM_LFO_PARAMS * LFO_LEGACY_LENGTH +
                                  NUM_LFO_PARAMS +
                                  NUM_LFO_PARAMS + 1];
  uint8_t speed;
  uint8_t mode;
  uint8_t legacy_offset_behaviour;
  uint64_t pattern_mask;
  bool enable;
  uint8_t length;

  void init();
  void load_data(const SeqLFOData &data);
  void store_data(SeqLFOData *data) const;
};

static_assert(sizeof(LegacyLFOSeqTrackData) == 219,
              "LegacyLFOSeqTrackData storage size changed");

class LFOSeqTrack : public LFOSeqTrackData {
public:
  uint8_t track_number;
  DeviceIdx device_idx;
  uint8_t step_count;
  uint16_t phase;
  uint16_t phase_inc;
  uint16_t random_state[2];
  int8_t state_phase;
  int8_t random_value;
  bool legacy_phase_offset;
  bool legacy_speed_curve;
  uint8_t last_wav_value[NUM_LFO_PARAMS];

  static const uint8_t wav_tables[LFO_TABLE_COUNT][WAV_LENGTH] PROGMEM;

  int16_t get_sample() NOINLINE();
  static uint8_t get_wav_table_sample(uint8_t table, uint8_t n);
  static int16_t get_preview_sample(uint8_t wav_type, uint16_t phase);
  static uint8_t get_preview_value(uint8_t wav_type, uint16_t phase);

  void init() {
    LFOSeqTrackData::init();
    track_number = 0;
    device_idx = DeviceIdx::Primary;
    legacy_phase_offset = false;
    legacy_speed_curve = false;
    for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
      last_wav_value[i] = 255;
    }
    reset_runtime();
  }
  void reset_runtime();
  void reset_phase();
  void reset_shape_state();
  void advance_phase();
  static uint8_t mode_base(uint8_t mode) { return mode & LFO_MODE_MASK; }
  static uint8_t mode_speed_multiplier(uint8_t mode) {
    return (mode >> LFO_SPEED_MULT_SHIFT) & LFO_SPEED_MULT_MASK;
  }
  static uint8_t pack_mode(uint8_t base_mode, uint8_t speed_multiplier) {
    return (base_mode & LFO_MODE_MASK) |
           ((speed_multiplier & LFO_SPEED_MULT_MASK) << LFO_SPEED_MULT_SHIFT);
  }
  uint8_t base_mode() const { return mode_base(mode); }
  uint8_t speed_multiplier() const { return mode_speed_multiplier(mode); }
  void set_mode(uint8_t base_mode) {
    mode = pack_mode(base_mode, speed_multiplier());
  }
  void set_speed_multiplier(uint8_t multiplier);
  static uint16_t speed_to_phase_increment(uint8_t speed);
  static uint16_t speed_to_phase_increment(uint8_t speed, bool legacy_curve);
  static uint16_t speed_to_phase_increment(uint8_t speed, bool legacy_curve,
                                           uint8_t multiplier);
  void load_data(const SeqLFOData &data);
  void load_data(const SeqLFOData &data, bool legacy_phase);
  void load_data(const SeqLFOData &data, bool legacy_phase,
                 bool legacy_shape_and_speed);
  void store_data(SeqLFOData *data) const;
  void store_legacy_data(SeqLFOData *data) const;
  void set_legacy_phase_offset(bool enable) { legacy_phase_offset = enable; }

  void reset_params();

  uint8_t get_wav_value(uint8_t offset, uint8_t param_id,
                        int16_t lfo_sample);

  void set_wav_type(uint8_t _wav_type);
  void set_speed(uint8_t _speed);
  void set_depth(uint8_t param, uint8_t depth) {
      params[param].depth = depth;
  }
  void seq(MidiUartClass *uart_, MidiUartClass *uart2_, bool send_due);
};

#endif /* LFOSEQTRACK_H__ */
