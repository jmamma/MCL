#include "LFOSeqTrack.h"
#include "MCL.h"

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
