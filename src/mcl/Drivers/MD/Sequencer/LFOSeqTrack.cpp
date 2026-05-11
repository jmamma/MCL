#include "LFOSeqTrack.h"
#include "LFO.h"
#include "MidiClock.h"
#include "MD.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "MidiSetup.h"

uint8_t LFOSeqTrack::wav_tables[4][WAV_LENGTH];

void LegacyLFOSeqTrackData::init() {
  enable = false;
  wav_type = 0;
  sample_hold = 0;
  speed = 0;
  mode = 0;
  offset_behaviour = LFO_OFFSET_CENTRE;
  pattern_mask = 0;
  length = 16;
  for (uint8_t a = 0; a < NUM_LFO_PARAMS; a++) {
    params[a].init();
    last_wav_value[a] = 255;
    wav_table_state[a] = false;
    for (uint8_t n = 0; n < WAV_LENGTH; ++n) {
      wav_table[a][n] = 0;
    }
  }
}

void LegacyLFOSeqTrackData::load_data(const SeqLFOData &data) {
  wav_type = data.wav_type;
  speed = data.speed;
  mode = data.mode;
  pattern_mask = data.pattern_mask;
  enable = data.enable;
  length = data.length;
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    params[i] = data.params[i];
  }
}

void LegacyLFOSeqTrackData::store_data(SeqLFOData *data) const {
  if (data == nullptr) {
    return;
  }
  data->wav_type = wav_type;
  data->speed = speed;
  data->mode = mode;
  data->pattern_mask = pattern_mask;
  data->enable = enable;
  data->length = length;
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    data->params[i] = params[i];
  }
}

void LFOSeqTrack::reset_runtime() {
  sample_hold = 0;
  sample_count = 0;
  step_count = 0;
}

void LFOSeqTrack::load_data(const SeqLFOData &data) {
  wav_type = data.wav_type;
  speed = data.speed;
  mode = data.mode;
  pattern_mask = data.pattern_mask;
  enable = data.enable;
  length = data.length ? data.length : 16;
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    params[i] = data.params[i];
    last_wav_value[i] = 255;
  }
  reset_runtime();
}

void LFOSeqTrack::store_data(SeqLFOData *data) const {
  if (data == nullptr) {
    return;
  }
  data->wav_type = wav_type;
  data->speed = speed;
  data->mode = mode;
  data->pattern_mask = pattern_mask;
  data->enable = enable;
  data->length = length;
  for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
    data->params[i] = params[i];
  }
}

void LFOSeqTrack::load_tables() {
  SinLFO sin_lfo;
  TriLFO tri_lfo;
  RampLFO ramp_lfo;
  IExpLFO iexp_lfo;

  sin_lfo.amplitude = 128;
  tri_lfo.amplitude = 128;
  ramp_lfo.amplitude = 128;
  iexp_lfo.amplitude = 128;

  for (uint8_t i = 0; i < LFO_LENGTH; i++) {
    wav_tables[SIN_WAV][i] = sin_lfo.get_sample(i);
    wav_tables[TRI_WAV][i] = tri_lfo.get_sample(i);
    wav_tables[RAMP_WAV][i] = ramp_lfo.get_sample(i);
    wav_tables[IEXP_WAV][i] = iexp_lfo.get_sample(i);
  }
}

int16_t LFOSeqTrack::get_sample(uint8_t n) {

  int16_t out = 0;
  switch (wav_type) {
  case IRAMP_WAV:
  case EXP_WAV:
    out = 128 - wav_tables[wav_type - 2][n];
    break;
  default:
    out = wav_tables[wav_type][n];
    break;
  }

  switch (wav_type) {
  // OFFSET CENTRE
  case SIN_WAV:
  case TRI_WAV:
    out -= 64;
    break;
  // OFFSET MAX
  default:
    out -= 128;
    break;
  }
  return out;
}

uint8_t LFOSeqTrack::get_wav_value(uint8_t sample_count, uint8_t dest,
                                   uint8_t param_id) {
  int8_t offset = get_param_offset(dest, param_id);
  int16_t depth = params[param_id].depth;

  int16_t sample = ((get_sample(sample_count) * depth) / 128) + offset;

  if (sample > 127) {
    return 127;
  }
  if (sample < 0) {
    return 0;
  }

  return (uint8_t)sample;
}

static void send_grid_y_lfo_param(uint8_t dest, uint8_t param,
                                  uint8_t value) {
  if (dest >= NUM_EXT_TRACKS) {
    return;
  }
#if defined(PLATFORM_TBD)
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    mcl_seq.midi_tracks[dest].send_cc(param, value);
    return;
  }
#endif
  mcl_seq.ext_tracks[dest].send_cc(param, value);
}

static void send_grid_x_lfo_param(uint8_t dest, uint8_t param,
                                  uint8_t value, MidiUartClass *uart_) {
#if defined(PLATFORM_TBD)
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    return;
  }
#endif
  if (dest >= NUM_MD_TRACKS + 4) {
    return;
  }
  if (dest >= NUM_MD_TRACKS) {
    MD.setFXParam(param, value, MD_FX_ECHO + dest - NUM_MD_TRACKS, false,
                  uart_);
  } else {
    MD.setTrackParam(dest, param, value, uart_);
  }
}

void LFOSeqTrack::seq(MidiUartClass *uart_, MidiUartClass *uart2_) {
  (void)uart2_;
  if ((MidiClock.mod12_counter == 0) && (mode != LFO_MODE_FREE) &&
      IS_BIT_SET64(pattern_mask, step_count)) {
    sample_count = 0;
  }
  if (enable) {
    for (uint8_t i = 0; i < NUM_LFO_PARAMS; i++) {
      if (params[i].dest == 0) {
        continue;
      }
      uint8_t dest = params[i].dest - 1;
      uint8_t wav_value = get_wav_value(sample_count, dest, i);
      if (last_wav_value[i] != wav_value) {
        uint8_t param = params[i].param;

        if (device_slot == 2) {
          send_grid_y_lfo_param(dest, param, wav_value);
        } else {
          send_grid_x_lfo_param(dest, param, wav_value, uart_);
        }

        last_wav_value[i] = wav_value;
      }
    }
  }
  if (speed < 1) {
    sample_count += 1;
  } else {
    sample_hold += 1;
    if (sample_hold >= (speed - 1)) {
      sample_hold = 0;
      sample_count += 1;
    }
  }

  /*
   if (speed < 8) {
     sample_count += 8 - speed;
   } else {
     sample_hold += 1;
     if (sample_hold >= (speed - 8)) {
       sample_hold = 0;
       sample_count += 1;
     }
   }
   */
  if (sample_count >= LFO_LENGTH) {
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

void LFOSeqTrack::reset_params() {
//  while (MidiClock.state == 2 && mod12_counter == MidiClock.mod12_counter) {}; 

  for (uint8_t i = 0; i < NUM_LFO_PARAMS; i++) {
    if (params[i].dest == 0) {
      continue;
    }
    uint8_t dest = params[i].dest - 1;
    uint8_t param = params[i].param;
    uint8_t wav_value = get_param_offset(dest, i);
    if (device_slot == 2) {
      send_grid_y_lfo_param(dest, param, wav_value);
    } else {
      send_grid_x_lfo_param(dest, param, wav_value, mcl_seq.primary_output);
    }
    last_wav_value[i] = 255;
  }
}

uint8_t LFOSeqTrack::get_param_offset(uint8_t dest, uint8_t param_id) {
  uint8_t param = params[param_id].param;
  if (device_slot == 2) {
    return params[param_id].offset;
  }
#if defined(PLATFORM_TBD)
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    return params[param_id].offset;
  }
#endif
  if (dest < NUM_MD_TRACKS) {
    return MD.kit.params[dest][param];
  } else if (dest < NUM_MD_TRACKS + 4) {
    switch (dest - NUM_MD_TRACKS) {
    case MD_FX_ECHO - MD_FX_ECHO:
      return MD.kit.delay[param];
      break;
    case MD_FX_DYN - MD_FX_ECHO:
      return MD.kit.dynamics[param];
      break;
    case MD_FX_REV - MD_FX_ECHO:
      return MD.kit.reverb[param];
      break;
    case MD_FX_EQ - MD_FX_ECHO:
      return MD.kit.eq[param];
      break;
    }
  } else {
    // MIDI
    return params[param_id].offset;
  }
  return 255;
}
