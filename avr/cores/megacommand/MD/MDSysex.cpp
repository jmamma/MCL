#include "WProgram.h"

#include "MD.h"
#include "MDMessages.h"
#include "MDSysex.h"
#include "helpers.h"

#include "MCL_impl.h"

void MDSysexListenerClass::start() {
  msgType = 255;
  isMDMessage = false;
}

void MDSysexListenerClass::handleByte(uint8_t byte) { }

void MDSysexListenerClass::end_immediate() { }

void MDSysexListenerClass::end() {
  if (sysex->getByte(3) == 0x02) {
    isMDMessage = true;
  } else {
    isMDMessage = false;
    return;
  }
  uint8_t offset = sizeof(machinedrum_sysex_hdr);
  msgType = sysex->getByte(offset++);

  uint8_t param, value, fx_type, track;

  switch (msgType) {
  case MD_STATUS_RESPONSE_ID:
    onStatusResponseCallbacks.call(sysex->getByte(offset++), sysex->getByte(offset++));
    break;

  case MD_GLOBAL_MESSAGE_ID:
    onGlobalMessageCallbacks.call();
    break;

  case MD_KIT_MESSAGE_ID:
    onKitMessageCallbacks.call();
    break;

  case MD_PATTERN_MESSAGE_ID:
    onPatternMessageCallbacks.call();
    break;

  case MD_SONG_MESSAGE_ID:
    onSongMessageCallbacks.call();
    break;

  case MD_SAMPLE_NAME_ID:
    onSampleNameCallbacks.call();
    break;

  case MD_SET_RHYTHM_ECHO_PARAM_ID:
  case MD_SET_GATE_BOX_PARAM_ID:
  case MD_SET_EQ_PARAM_ID:
  case MD_SET_DYNAMIX_PARAM_ID:

    param = sysex->getByte(offset++);
    value = sysex->getByte(offset++);
    fx_type = msgType - MD_SET_RHYTHM_ECHO_PARAM_ID;

    if (param > 8) { return; }

    for (uint8_t n = 0; n < mcl_seq.num_lfo_tracks; n++) {
      mcl_seq.lfo_tracks[n].check_and_update_params_offset(17 + fx_type, param, value);
    }

    switch (msgType) {
      case MD_SET_RHYTHM_ECHO_PARAM_ID:
        MD.kit.delay[param] = value;
      break;
      case MD_SET_GATE_BOX_PARAM_ID:
        MD.kit.reverb[param] = value;
      break;
      case MD_SET_EQ_PARAM_ID:
        MD.kit.eq[param] = value;
      break;
      case MD_SET_DYNAMIX_PARAM_ID:
        MD.kit.dynamics[param] = value;
      break;
    }

    break;

  case MD_SET_LFO_PARAM_ID:

    track = sysex->getByte(offset) >> 3;
    param = sysex->getByte(offset++) & 7;
    value = sysex->getByte(offset++);

    if (track > 15) { return; }
    //LFOS, LFOD, LFOM
    if (4 < param && param < 8) {
      mcl_seq.md_tracks[track].update_param(param + 16, value);
      MD.kit.params[track][param + 16] = value;
      switch (param) {
        case 5:
          MD.kit.lfos[track].speed = value;
          break;
        case 6:
          MD.kit.lfos[track].depth = value;
          break;
        case 7:
          MD.kit.lfos[track].mix = value;
          break;
      }
    }

    for (uint8_t n = 0; n < mcl_seq.num_lfo_tracks; n++) {
      mcl_seq.lfo_tracks[n].check_and_update_params_offset(track + 1, param + 16, value);
    }

    break;
  }
}

void MDSysexListenerClass::setup(MidiClass *_midi) {
  sysex = &(_midi->midiSysex);
  sysex->addSysexListener(this);
}
