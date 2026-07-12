#include "platform.h"

#include "MD.h"
#include "MDMessages.h"
#include "MDSysex.h"
#include "helpers.h"
#include "MCLPlatformFeatures.h"
#if MCL_FEATURE_HOST_ARRANGER
#include "Arrangement/MCLArrangement.h"
#endif
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
#include "Host/SpsHostArrBridge.h"
#include "MCLMemory.h"
#endif
#include "GUI/Pages/CommonPages.h"
#include "MDPages.h"

#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
namespace {

void notify_arranger_fx_recorded() {
  sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)spsarr::DIRTY_ARRANGEMENT);
}

void record_arranger_fx_param(uint8_t fx_type, uint8_t param, uint8_t value) {
  if (!mcl_arrangement.automationRecordArmed() || fx_type >= 4 || param >= 8) {
    return;
  }
  uint8_t target = (uint8_t)((fx_type << 3) | param);
  if (mcl_arrangement.recordAutomationPoint(
          (uint8_t)(NUM_MD_TRACKS + MDFX_TRACK_NUM),
          mclarrfile::AUTOMATION_TARGET_MD_PARAM, 0, target,
          mclarrfile::AUTOMATION_VALUE_U7, value,
          mclarrfile::AUTOMATION_INTERP_CURVE, 0)) {
    notify_arranger_fx_recorded();
  }
}

}  // namespace
#endif

void MDSysexListenerClass::start() {
  msgType = 255;
  isMDMessage = false;
}

void MDSysexListenerClass::handleByte(uint8_t byte) { }

void MDSysexListenerClass::end() {
  SysexView view(sysex);
  isMDMessage = view.getByte(3) == 0x02;
  if (!isMDMessage) {
    return;
  }
  uint8_t offset = sizeof(machinedrum_sysex_hdr);
  msgType = view.getByte(offset++);

  uint8_t param, value, fx_type, track;
  switch (msgType) {
  case MD_STATUS_RESPONSE_ID:
    param = view.getByte(offset++);
    value = view.getByte(offset++);
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

    param = view.getByte(offset++);
    value = view.getByte(offset++);
    fx_type = msgType - MD_SET_RHYTHM_ECHO_PARAM_ID;

    if (param > 7) { return; }

    {
      uint8_t *fx_params = MD.kit.fx_params(msgType);
      if (fx_params != nullptr) {
        fx_params[param] = value;
      }
    }

#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
    record_arranger_fx_param(fx_type, param, value);
#endif

    perf_page.learn_param(fx_type + 16, param, value);
    lfo_page.learn_perf_dest(fx_type + NUM_MD_TRACKS + 1, param, value);
    {
      uint8_t current_page = mcl.currentPage();
      if (current_page == FX_PAGE_A) {
        fx_page_a.update_encoders();
      } else if (current_page == FX_PAGE_B) {
        fx_page_b.update_encoders();
      }
    }

    break;

  case MD_SET_LFO_PARAM_ID:

    param = view.getByte(offset++);
    track = param >> 3;
    param &= 7;
    value = view.getByte(offset++);

    if (track > 15) { return; }
    if (param > 7) { return; }

    {
      uint8_t *p = &MD.kit.lfos[track].destinationTrack;
      p[param] = value;

      //LFOS, LFOD, LFOM
      if (4 < param && param < 8) {
        MD.kit.params[track][param + 16] = value;
        perf_page.learn_param(track, param + 16, value);
        lfo_page.learn_perf_dest(track + 1, param + 16, value);
      }
    }

    break;

  case MD_KIT_LOADED_ID:
    // Kit loaded notification: F0 00 20 3C 02 00 54 <kitIdx> F7
    {
      uint8_t kitIdx = view.getByte(offset++);
      MD.currentKit = kitIdx;
      // Could trigger kit reload callback here if needed
    }
    break;

  case MD_MACHINE_UPDATE_ID:
    // Machine update: F0 00 20 3C 02 00 63 <track> <model> <flags_byte> <data_flags> [data...] F7
    {
      track = view.getByte(offset++);
      if (track > 15) { return; }

      uint8_t model = view.getByte(offset++);
      uint8_t flags_byte = view.getByte(offset++);
      uint8_t data_flags = view.getByte(offset++);

      // Decode model: flags_byte = (tonal << 1) | uwFlag
      uint8_t uwFlag = flags_byte & 0x01;
      uint8_t tonal = (flags_byte >> 1) & 0x01;

      if (uwFlag) {
        model += 128;
      }
      MD.kit.models[track] = tonal ? ((uint32_t)model + 0x20000) : model;

      // Params (34 bytes for SPS-X, 24 for legacy)
      if (data_flags & 0x01) {
        uint8_t num_params = MD.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
        for (int i = 0; i < num_params; i++) {
          MD.kit.params[track][i] = view.getByte(offset++);
        }
      }

      // LFO (5 bytes: destTrack, destParam, shape1, shape2, type)
      // These five MDLFO members are consecutive uint8_t fields; read them
      // through a pointer in a loop (identical byte/field mapping).
      if (data_flags & 0x02) {
        uint8_t *lfo = &MD.kit.lfos[track].destinationTrack;
        for (uint8_t i = 0; i < 5; i++) {
          lfo[i] = view.getByte(offset++);
        }
      }

      // Level (1 byte)
      if (data_flags & 0x04) {
        MD.kit.levels[track] = view.getByte(offset++);
      }

      // Trig + Mute groups (2 bytes)
      if (data_flags & 0x08) {
        uint8_t trig = view.getByte(offset++);
        uint8_t mute = view.getByte(offset++);
        MD.kit.trigGroups[track] = (trig == 0x7F) ? 255 : trig;
        MD.kit.muteGroups[track] = (mute == 0x7F) ? 255 : mute;
      }
#if MCL_FEATURE_HOST_ARRANGER
      mcl_arrangement.markRuntimePrivateSourceEdited(track);
#endif
    }
    break;
  }
}

void MDSysexListenerClass::setup(MidiClass *_midi) {
  sysex = _midi->midiSysex;
  sysex->addSysexListener(this);
}
