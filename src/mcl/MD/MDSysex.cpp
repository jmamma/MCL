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
    lfo_page.learn_param(fx_type + 16, param, value);
    if (GUI.currentPage() == &fx_page_a) { fx_page_a.update_encoders(); }
    if (GUI.currentPage() == &fx_page_b) { fx_page_b.update_encoders(); }

    break;

  case MD_SET_LFO_PARAM_ID:

    track = sysex->getByte(offset) >> 3;
    param = sysex->getByte(offset++) & 7;
    value = sysex->getByte(offset++);

    if (track > 15) { return; }
    if (param > 7) { return; }

    uint8_t *p = &MD.kit.lfos[track].destinationTrack;
    p[param] = value;

    //LFOS, LFOD, LFOM
    if (4 < param && param < 8) {
      MD.kit.params[track][param + 16] = value;
    }

    break;
  }
}

void MDSysexListenerClass::setup(MidiClass *_midi) {
  sysex = &(_midi->midiSysex);
  sysex->addSysexListener(this);
}
