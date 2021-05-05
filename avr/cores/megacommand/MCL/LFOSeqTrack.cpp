#include "MCL_impl.h"

void LFOSeqTrack::load_wav_table(uint8_t table) {
  SinLFO sin_lfo;
  TriLFO tri_lfo;
  RampLFO ramp_lfo;
  IRampLFO iramp_lfo;
  ExpLFO exp_lfo;
  IExpLFO iexp_lfo;
  LFO *lfo;
  switch (wav_type) {
  case SIN_WAV:
    lfo = (LFO *)&sin_lfo;
    offset_behaviour = LFO_OFFSET_CENTRE;
    break;
  case TRI_WAV:
    lfo = (LFO *)&tri_lfo;
    offset_behaviour = LFO_OFFSET_CENTRE;
    break;
  case IRAMP_WAV:
    lfo = (LFO *)&iramp_lfo;
    offset_behaviour = LFO_OFFSET_MAX;
    break;
  case RAMP_WAV:
    lfo = (LFO *)&ramp_lfo;
    offset_behaviour = LFO_OFFSET_MAX;
    break;
  case EXP_WAV:
    lfo = (LFO *)&exp_lfo;
    offset_behaviour = LFO_OFFSET_MAX;
    break;
  case IEXP_WAV:
    lfo = (LFO *)&iexp_lfo;
    offset_behaviour = LFO_OFFSET_MAX;
    break;
  }
  lfo->amplitude = params[table].depth;
  // ExpLFO exp_lfo(20);
  for (uint8_t n = 0; n < LFO_LENGTH; n++) {
    wav_table[table][n] = (float)lfo->get_sample(n);
  }
  wav_table_state[table] = true;
}

uint8_t LFOSeqTrack::get_wav_value(uint8_t sample_count, uint8_t param) {
  int8_t offset = params[param].offset;
  int8_t depth = params[param].depth;
  int8_t sample = wav_table[param][sample_count];
  int16_t val;

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

void LFOSeqTrack::seq(MidiUartParent *uart_) {
  MidiUartParent *uart_old = uart;
  uart = uart_;

  if ((MidiClock.mod12_counter == 0) && (mode != LFO_MODE_FREE) &&
      IS_BIT_SET64(pattern_mask, step_count)) {
    sample_count = 0;
  }
  if (enable) {
    for (uint8_t i = 0; i < NUM_LFO_PARAMS; i++) {
      uint8_t wav_value = get_wav_value(sample_count, i);
      if (last_wav_value[i] != wav_value) {

        if (params[i].dest > 0) {
          // MD CC LFO
          if (params[i].dest <= NUM_MD_TRACKS) {
            MD.setTrackParam_inline(params[i].dest - 1, params[i].param,
                                    wav_value, uart);
          }
          // MD FX LFO
          else {
            MD.sendFXParam(params[i].param, wav_value,
                           MD_FX_ECHO + params[i].dest - NUM_MD_TRACKS - 1, uart);
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
  uart = uart_old;
}

void LFOSeqTrack::check_and_update_params_offset(uint8_t track, uint8_t dest,
                                                 uint8_t value) {
  for (uint8_t n = 0; n < NUM_LFO_PARAMS; n++) {
    if ((params[n].dest == track) && (params[n].param == dest)) {
      wav_table_state[n] = false;
      params[n].offset = value;
    }
  }
}

void LFOSeqTrack::reset_params_offset() {
  if (enable) {
    for (uint8_t n = 0; n < NUM_LFO_PARAMS; n++) {
      params[n].reset_param_offset();
    }
  }
}

void LFOSeqTrack::update_params_offset() {
  for (uint8_t n = 0; n < NUM_LFO_PARAMS; n++) {
    params[n].update_offset();
  }
}

void LFOSeqTrack::update_kit_params() {
  for (uint8_t n = 0; n < NUM_LFO_PARAMS; n++) {
    params[n].update_kit();
  }
}

void LFOSeqParam::update_kit() {
  if (dest <= NUM_MD_TRACKS) {
    MD.kit.params[dest - 1][param] = offset;
  } else {
    switch (dest - NUM_MD_TRACKS - 1) {
    case MD_FX_ECHO - MD_FX_ECHO:
      MD.kit.delay[param] = offset;
      break;
    case MD_FX_DYN - MD_FX_ECHO:
      MD.kit.dynamics[param] = offset;
      break;
    case MD_FX_REV - MD_FX_ECHO:
      MD.kit.reverb[param] = offset;
      break;
    case MD_FX_EQ - MD_FX_ECHO:
      MD.kit.eq[param] = offset;
      break;
    }
  }
}

void LFOSeqParam::update_offset() { offset = get_param_offset(dest, param); }
void LFOSeqParam::reset_param_offset() { reset_param(dest, param, offset); }

void LFOSeqParam::reset_param(uint8_t dest, uint8_t param, uint8_t value) {
  if (dest <= NUM_MD_TRACKS) {
    MD.setTrackParam(dest - 1, param, value);
  } else {
    MD.sendFXParam(param, value, MD_FX_ECHO + dest - NUM_MD_TRACKS - 1);
  }
}

uint8_t LFOSeqParam::get_param_offset(uint8_t dest, uint8_t param) {
  if (dest <= NUM_MD_TRACKS) {
    return MD.kit.params[dest - 1][param];
  } else {
    switch (dest - NUM_MD_TRACKS - 1) {
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
  }
  return 255;
}
