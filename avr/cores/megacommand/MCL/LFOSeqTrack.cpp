#include "MCL_impl.h"

static uint8_t LFOSeqTrack::wav_tables[4][WAV_LENGTH];

void LFOSeqTrack::load_tables() {
  SinLFO sin_lfo;
  TriLFO tri_lfo;
  RampLFO ramp_lfo;
  IExpLFO iexp_lfo;
  LFO *lfo;


  for (uint8_t n = 0; n < 4; n++) {
    switch (n) {
    case SIN_WAV:
      lfo = (LFO *)&sin_lfo;
      break;
    case TRI_WAV:
      lfo = (LFO *)&tri_lfo;
      break;
    case RAMP_WAV:
      lfo = (LFO *)&ramp_lfo;
      break;
    case IEXP_WAV:
      lfo = (LFO *)&iexp_lfo;
      break;
    }
    lfo->amplitude = 128;
    for (uint8_t i = 0; i < LFO_LENGTH; i++) {
      wav_tables[n][i] = lfo->get_sample(i);
    }
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
    //OFFSET CENTRE
    case SIN_WAV:
    case TRI_WAV:
       out -= 64;
       break;
    //OFFSET MAX
    default:
       out -= 128;
       break;
  }
  return out;
}

uint8_t LFOSeqTrack::get_wav_value(uint8_t sample_count, uint8_t dest, uint8_t param) {
  int8_t offset = get_param_offset(dest, param);
  int16_t depth = params[param].depth;

  int16_t sample = ((get_sample(sample_count) * depth) /  128) + offset;

  if (sample > 127) { return 127; }
  if (sample < 0) { return 0; }

  return (uint8_t) sample;
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
      uint8_t dest = params[i].dest - 1;
      uint8_t wav_value = get_wav_value(sample_count, dest, i);
      if (last_wav_value[i] != wav_value) {
        uint8_t param = params[i].param;

        if (dest >= NUM_MD_TRACKS + 4) {
          uint8_t channel = dest - (NUM_MD_TRACKS + 4);
          MidiUart2.sendCC(channel, param, wav_value);
        } else if (dest >= NUM_MD_TRACKS) {
          MD.sendFXParam(param, wav_value, MD_FX_ECHO + dest - NUM_MD_TRACKS,
                         uart);
        } else {
          MD.setTrackParam_inline(dest, param, wav_value, uart);
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
  uart = uart_old;
}

uint8_t LFOSeqTrack::get_param_offset(uint8_t dest, uint8_t param) {
  if (dest < NUM_MD_TRACKS) {
    return MD.kit.params[dest][param];
  } else {
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
  }
  return 255;
}
