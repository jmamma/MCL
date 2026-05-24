#include "LFOTrackRef.h"

#include "DeviceParamResolver.h"
#include "DeviceManager.h"
#include "MCLSeq.h"
#include "PerfPage.h"
#include "SeqPage.h"
#include "SeqPages.h"
#include "../Drivers/MidiDevice.h"
#include <string.h>

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#endif

extern PerfPage perf_page;

namespace {

bool lfo_perf_dest_index(DeviceIdx device_idx, uint8_t dest, uint8_t *idx) {
  (void)device_idx;
  uint8_t device_slots = DeviceParamResolver::perf_target_count();
  if (dest <= device_slots || dest > device_slots + NUM_PERF_CONTROLS) {
    return false;
  }
  if (idx != nullptr) {
    *idx = dest - device_slots - 1;
  }
  return true;
}

PerfEncoder *lfo_perf_encoder(uint8_t idx) {
  return idx < NUM_PERF_CONTROLS ? perf_page.perf_encoders[idx] : nullptr;
}

bool copy_lfo_perf_target_label(uint8_t idx, char *out, uint8_t len) {
  if (out == nullptr || len < 4 || idx >= NUM_PERF_CONTROLS) {
    return false;
  }
  out[0] = 'P';
  out[1] = 'F';
  out[2] = '1' + idx;
  out[3] = '\0';
  return true;
}

bool copy_lfo_perf_param_label(uint8_t param, char *out, uint8_t len) {
  if (out == nullptr || len < 4 || param != 0) {
    return false;
  }
  strcpy(out, "VAL");
  return true;
}

} // namespace

LFOSeqTrack &LFOTrackRef::current_track() {
#ifdef EXT_TRACKS
  if (SeqPage::current_device_idx() == DeviceIdx::Secondary) {
    return mcl_seq.grid_y_lfo_tracks[last_ext_track];
  }
#endif
  return mcl_seq.grid_x_lfo_tracks[last_primary_track];
}

bool LFOTrackRef::select_track(uint8_t track) {
  bool primary_tracks = SeqPage::current_device_idx() == DeviceIdx::Primary;
#ifndef EXT_TRACKS
  primary_tracks = true;
#endif
  uint8_t count = primary_tracks ? NUM_GRID_X_LFO_TRACKS
                                 : NUM_GRID_Y_LFO_TRACKS;
  if (track >= count) {
    return false;
  }
  if (primary_tracks) {
    last_primary_track = track;
  } else {
#ifdef EXT_TRACKS
    last_ext_track = track;
#endif
  }
  return true;
}

uint8_t LFOTrackRef::track_count(DeviceIdx device_idx) {
#ifdef EXT_TRACKS
  return device_idx == DeviceIdx::Primary ? NUM_GRID_X_LFO_TRACKS
                                          : NUM_GRID_Y_LFO_TRACKS;
#else
  (void)device_idx;
  return NUM_GRID_X_LFO_TRACKS;
#endif
}

uint8_t LFOTrackRef::target_count(DeviceIdx device_idx) {
  (void)device_idx;
  return DeviceParamResolver::perf_target_count() + NUM_PERF_CONTROLS;
}

bool LFOTrackRef::target_label(DeviceIdx device_idx, uint8_t dest, char *out,
                               uint8_t len) {
  (void)device_idx;
  uint8_t perf_idx = 0;
  if (lfo_perf_dest_index(device_idx, dest, &perf_idx)) {
    return copy_lfo_perf_target_label(perf_idx, out, len);
  }
  return DeviceParamResolver::perf(dest).target_label(out, len);
}

uint8_t LFOTrackRef::param_count(DeviceIdx device_idx, uint8_t dest) {
  if (lfo_perf_dest_index(device_idx, dest, nullptr)) {
    return 1;
  }
  return DeviceParamResolver::perf(dest).param_count();
}

bool LFOTrackRef::param_label(DeviceIdx device_idx, uint8_t dest, uint8_t param,
                              char *out, uint8_t len) {
  if (lfo_perf_dest_index(device_idx, dest, nullptr)) {
    return copy_lfo_perf_param_label(param, out, len);
  }
  return DeviceParamResolver::perf(dest).param_label(param, out, len);
}

bool LFOTrackRef::get_base_param(DeviceIdx device_idx, uint8_t dest,
                                 uint8_t param, uint8_t *value) {
  uint8_t perf_idx = 0;
  if (lfo_perf_dest_index(device_idx, dest, &perf_idx)) {
    PerfEncoder *encoder = lfo_perf_encoder(perf_idx);
    if (encoder == nullptr || param != 0 || value == nullptr) {
      return false;
    }
    *value = encoder->cur;
    return true;
  }
  return DeviceParamResolver::perf(dest).params.get_base_param(param, value);
}

bool LFOTrackRef::set_base_param(DeviceIdx device_idx, uint8_t dest,
                                 uint8_t param, uint8_t value) {
  uint8_t perf_idx = 0;
  if (lfo_perf_dest_index(device_idx, dest, &perf_idx)) {
    PerfEncoder *encoder = lfo_perf_encoder(perf_idx);
    if (encoder == nullptr || param != 0) {
      return false;
    }
    encoder->setValue(value);
    encoder->send();
    return true;
  }
  return DeviceParamResolver::perf(dest).params.set_base_param(param, value);
}

bool LFOTrackRef::send_modulated_param(DeviceIdx device_idx, uint8_t dest,
                                       uint8_t param, uint8_t value,
                                       MidiUartClass *uart_,
                                       MidiUartClass *uart2_) {
  uint8_t perf_idx = 0;
  if (lfo_perf_dest_index(device_idx, dest, &perf_idx)) {
    PerfEncoder *encoder = lfo_perf_encoder(perf_idx);
    if (encoder == nullptr || param != 0) {
      return false;
    }
    encoder->setValue(value);
    encoder->send(uart_, uart2_);
    return true;
  }

  DevicePerfTarget target = DeviceParamResolver::perf(dest);
  MidiUartClass *output_uart =
      target.device_index() == DeviceIdx::Secondary ? uart2_ : uart_;
  return target.set_param(param, value, output_uart);
}

void LFOTrackRef::sync_panel(const LFOSeqTrack &track) {
#if defined(__AVR__)
  MD.sync_seqtrack(track.length, track.speed, track.step_count);
#else
  device_manager.primary_device()->panel()->sync_seqtrack(
      track.length, track.speed, track.step_count);
#endif
}

bool LFOTrackRef::supports_trig_port(uint8_t port) {
  return device_manager.port_supports(port,
                                      MidiDeviceCapability::MdTrigInterface);
}
