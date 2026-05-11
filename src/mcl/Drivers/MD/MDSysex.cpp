#include "platform.h"

#include "MD.h"
#include "MDMessages.h"
#include "MDSysex.h"
#include "helpers.h"
#include "CommonPages.h"
#include "MDPages.h"

void MDSysexListenerClass::start() {
  msgType = 255;
  isMDMessage = false;
}

void MDSysexListenerClass::handleByte(uint8_t byte) { }

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
    param = sysex->getByte(offset++);
    value = sysex->getByte(offset++);
    onStatusResponseCallbacks.call(param, value);
    break;

  case MD_GLOBAL_MESSAGE_ID:
  case MD_KIT_MESSAGE_ID:
  case MD_PATTERN_MESSAGE_ID:
  case MD_SONG_MESSAGE_ID:
    onMessageCallbacks.call();
    break;

  case MD_SET_RHYTHM_ECHO_PARAM_ID:
  case MD_SET_GATE_BOX_PARAM_ID:
  case MD_SET_EQ_PARAM_ID:
  case MD_SET_DYNAMIX_PARAM_ID:

    param = sysex->getByte(offset++);
    value = sysex->getByte(offset++);
    fx_type = msgType - MD_SET_RHYTHM_ECHO_PARAM_ID;

    if (param > 7) { return; }

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

    perf_page.learn_param(fx_type + 16, param, value);
    lfo_page.learn_param(1, fx_type + NUM_MD_TRACKS + 1, param, value);
    if (GUI.currentPage() == &fx_page_a) { fx_page_a.update_encoders(); }
    if (GUI.currentPage() == &fx_page_b) { fx_page_b.update_encoders(); }

    break;

  case MD_SET_LFO_PARAM_ID:

    track = sysex->getByte(offset) >> 3;
    param = sysex->getByte(offset++) & 7;
    value = sysex->getByte(offset++);

    if (track > 15) { return; }
    if (param > 7) { return; }

    {
      uint8_t *p = &MD.kit.lfos[track].destinationTrack;
      p[param] = value;

      //LFOS, LFOD, LFOM
      if (4 < param && param < 8) {
        MD.kit.params[track][param + 16] = value;
        perf_page.learn_param(track, param + 16, value);
        lfo_page.learn_param(1, track + 1, param + 16, value);
      }
    }

    break;

  case MD_KIT_LOADED_ID:
    // Kit loaded notification: F0 00 20 3C 02 00 54 <kitIdx> F7
    {
      uint8_t kitIdx = sysex->getByte(offset++);
      MD.currentKit = kitIdx;
      // Could trigger kit reload callback here if needed
    }
    break;

  case MD_MACHINE_UPDATE_ID:
    // Machine update: F0 00 20 3C 02 00 63 <track> <model> <flags_byte> <data_flags> [data...] F7
    {
      track = sysex->getByte(offset++);
      if (track > 15) { return; }

      uint8_t model = sysex->getByte(offset++);
      uint8_t flags_byte = sysex->getByte(offset++);
      uint8_t data_flags = sysex->getByte(offset++);

      // Decode model: flags_byte = (tonal << 1) | uwFlag
      uint8_t uwFlag = flags_byte & 0x01;
      // uint8_t tonal = (flags_byte >> 1) & 0x01;

      if (uwFlag) {
        model += 128;
      }
      MD.kit.models[track] = model;

      // Params (34 bytes for SPS-X, 24 for legacy)
      if (data_flags & 0x01) {
        uint8_t num_params = MD.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
        for (int i = 0; i < num_params; i++) {
          MD.kit.params[track][i] = sysex->getByte(offset++);
        }
      }

      // LFO (5 bytes: destTrack, destParam, shape1, shape2, type)
      if (data_flags & 0x02) {
        MD.kit.lfos[track].destinationTrack = sysex->getByte(offset++);
        MD.kit.lfos[track].destinationParam = sysex->getByte(offset++);
        MD.kit.lfos[track].shape1 = sysex->getByte(offset++);
        MD.kit.lfos[track].shape2 = sysex->getByte(offset++);
        MD.kit.lfos[track].type = sysex->getByte(offset++);
      }

      // Level (1 byte)
      if (data_flags & 0x04) {
        MD.kit.levels[track] = sysex->getByte(offset++);
      }

      // Trig + Mute groups (2 bytes)
      if (data_flags & 0x08) {
        uint8_t trig = sysex->getByte(offset++);
        uint8_t mute = sysex->getByte(offset++);
        MD.kit.trigGroups[track] = (trig == 0x7F) ? 255 : trig;
        MD.kit.muteGroups[track] = (mute == 0x7F) ? 255 : mute;
      }
    }
    break;
  }
}

void MDSysexListenerClass::setup(MidiClass *_midi) {
  sysex = _midi->midiSysex;
  sysex->addSysexListener(this);
}
