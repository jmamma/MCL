#include "LFOTrackRef.h"

#include "DeviceParamResolver.h"
#include "DeviceManager.h"
#include "MCLGUI.h"
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

constexpr uint8_t LFO_TRACK_PARAM_SPEED = 0;
constexpr uint8_t LFO_TRACK_PARAM_DEPTH1 = 1;
constexpr uint8_t LFO_TRACK_PARAM_DEPTH2 = 2;
constexpr uint8_t LFO_TRACK_PARAM_COUNT = 3;

constexpr uint8_t LFO_TRACK_PARAM_READ = 0;
constexpr uint8_t LFO_TRACK_PARAM_SET_BASE = 1;
constexpr uint8_t LFO_TRACK_PARAM_SET_MODULATED = 2;

constexpr uint8_t LFO_DEST_DEVICE = 0;
constexpr uint8_t LFO_DEST_PERF = 1;
constexpr uint8_t LFO_DEST_TRACK = 2;

uint8_t lfo_perf_dest_base() {
#if defined(__AVR__)
  return NUM_MD_TRACKS + 4 + DeviceParamResolver::RESERVED_SECONDARY_TARGETS;
#else
  return DeviceParamResolver::perf_target_count();
#endif
}

uint8_t lfo_track_dest_base() {
  return lfo_perf_dest_base() + NUM_PERF_CONTROLS;
}

constexpr uint8_t lfo_track_dest_count() {
  return NUM_GRID_X_LFO_TRACKS + NUM_GRID_Y_LFO_TRACKS;
}

inline __attribute__((always_inline)) uint8_t
lfo_custom_dest_index(uint8_t dest, uint8_t *idx) {
  uint8_t base = lfo_perf_dest_base();
  if (dest <= base) {
    return LFO_DEST_DEVICE;
  }
  dest -= base + 1;
  if (dest < NUM_PERF_CONTROLS) {
    if (idx != nullptr) {
      *idx = dest;
    }
    return LFO_DEST_PERF;
  }
  dest -= NUM_PERF_CONTROLS;
  if (dest >= lfo_track_dest_count()) {
    return LFO_DEST_DEVICE;
  }
  if (idx != nullptr) {
    *idx = dest;
  }
  return LFO_DEST_TRACK;
}

LFOSeqTrack *lfo_track_for_index(uint8_t idx) {
  if (idx < NUM_GRID_X_LFO_TRACKS) {
    return &mcl_seq.grid_x_lfo_tracks[idx];
  }
#ifdef EXT_TRACKS
  idx -= NUM_GRID_X_LFO_TRACKS;
  if (idx < NUM_GRID_Y_LFO_TRACKS) {
    return &mcl_seq.grid_y_lfo_tracks[idx];
  }
#endif
  return nullptr;
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

bool copy_lfo_track_target_label(uint8_t idx, char *out, uint8_t len) {
  if (out == nullptr || len < 3 || (idx >= 9 && len < 4)) {
    return false;
  }
  out[0] = 'L';
  mcl_gui.put_value_at(idx + 1, out + 1);
  return true;
}

bool copy_lfo_track_param_label(uint8_t param, char *out, uint8_t len) {
  if (out == nullptr || len < 4 || param >= LFO_TRACK_PARAM_COUNT) {
    return false;
  }
  out[0] = param == LFO_TRACK_PARAM_SPEED ? 'S' : 'D';
  out[1] = 'P';
  out[2] = param == LFO_TRACK_PARAM_SPEED ? 'D' : '0' + param;
  out[3] = '\0';
  return true;
}

PerfEncoder *lfo_perf_encoder(uint8_t idx, uint8_t param) NOINLINE();
PerfEncoder *lfo_perf_encoder(uint8_t idx, uint8_t param) {
  PerfEncoder *encoder = perf_page.perf_encoders[idx];
  if (encoder == nullptr || param != 0) {
    return nullptr;
  }
  return encoder;
}

bool lfo_track_dispatch(uint8_t idx, uint8_t param, uint8_t *value,
                        uint8_t op) NOINLINE();
bool lfo_track_dispatch(uint8_t idx, uint8_t param, uint8_t *value,
                        uint8_t op);

bool lfo_track_param(LFOSeqTrack &track, uint8_t param, uint8_t *value,
                     uint8_t op) {
  if (value == nullptr || param >= LFO_TRACK_PARAM_COUNT) {
    return false;
  }
  if (op == LFO_TRACK_PARAM_READ) {
    *value = param == LFO_TRACK_PARAM_SPEED ? track.speed
                                            : track.params[param - 1].depth;
  } else if (param == LFO_TRACK_PARAM_SPEED) {
    if (op == LFO_TRACK_PARAM_SET_BASE) {
      track.set_speed(*value);
    } else {
      track.set_modulated_speed(*value);
    }
  } else if (op == LFO_TRACK_PARAM_SET_BASE) {
    track.set_depth(param - 1, *value);
  } else {
    track.set_modulated_depth(param - 1, *value);
  }
  return true;
}

bool lfo_track_dispatch(uint8_t idx, uint8_t param, uint8_t *value,
                        uint8_t op) {
  LFOSeqTrack *track = lfo_track_for_index(idx);
  return lfo_track_param(*track, param, value, op);
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

uint8_t LFOTrackRef::target_count() {
  return lfo_track_dest_base() + lfo_track_dest_count();
}

uint8_t LFOTrackRef::track_lfo_target_count() {
  return lfo_track_dest_count();
}

uint8_t LFOTrackRef::track_lfo_dest_for_index(uint8_t idx) {
  return idx < lfo_track_dest_count() ? lfo_track_dest_base() + idx + 1 : 0;
}

bool LFOTrackRef::target_label(uint8_t dest, char *out, uint8_t len) {
  uint8_t idx = 0;
  uint8_t dest_type = lfo_custom_dest_index(dest, &idx);
  if (dest_type == LFO_DEST_PERF) {
    uint8_t perf_idx = idx;
    return copy_lfo_perf_target_label(perf_idx, out, len);
  }
  if (dest_type == LFO_DEST_TRACK) {
    uint8_t lfo_idx = idx;
    return copy_lfo_track_target_label(lfo_idx, out, len);
  }
  return DeviceParamResolver::perf(dest).target_label(out, len);
}

uint8_t LFOTrackRef::param_count(uint8_t dest) {
  uint8_t lfo_idx = 0;
  uint8_t dest_type = lfo_custom_dest_index(dest, &lfo_idx);
  if (dest_type == LFO_DEST_PERF) {
    return 1;
  }
  if (dest_type == LFO_DEST_TRACK) {
    return LFO_TRACK_PARAM_COUNT;
  }
  return DeviceParamResolver::perf(dest).param_count();
}

bool LFOTrackRef::param_label(uint8_t dest, uint8_t param, char *out,
                              uint8_t len) {
  uint8_t lfo_idx = 0;
  uint8_t dest_type = lfo_custom_dest_index(dest, &lfo_idx);
  if (dest_type == LFO_DEST_PERF) {
    return copy_lfo_perf_param_label(param, out, len);
  }
  if (dest_type == LFO_DEST_TRACK) {
    return copy_lfo_track_param_label(param, out, len);
  }
  return DeviceParamResolver::perf(dest).param_label(param, out, len);
}

bool LFOTrackRef::get_base_param(uint8_t dest, uint8_t param,
                                 uint8_t *value) {
  uint8_t idx = 0;
  uint8_t dest_type = lfo_custom_dest_index(dest, &idx);
  if (dest_type == LFO_DEST_PERF) {
    PerfEncoder *encoder = lfo_perf_encoder(idx, param);
    if (encoder == nullptr || value == nullptr) {
      return false;
    }
    *value = encoder->cur;
    return true;
  }
  if (dest_type == LFO_DEST_TRACK) {
    return lfo_track_dispatch(idx, param, value, LFO_TRACK_PARAM_READ);
  }
  return DeviceParamResolver::perf(dest).params.get_base_param(param, value);
}

bool LFOTrackRef::set_base_param(uint8_t dest, uint8_t param,
                                 uint8_t value) {
  uint8_t idx = 0;
  uint8_t dest_type = lfo_custom_dest_index(dest, &idx);
  if (dest_type == LFO_DEST_PERF) {
    PerfEncoder *encoder = lfo_perf_encoder(idx, param);
    if (encoder == nullptr) {
      return false;
    }
    encoder->setValue(value);
    perf_page.send_perf_encoder(idx);
    return true;
  }
  if (dest_type == LFO_DEST_TRACK) {
    return lfo_track_dispatch(idx, param, &value, LFO_TRACK_PARAM_SET_BASE);
  }
  return DeviceParamResolver::perf(dest).params.set_base_param(param, value);
}

bool LFOTrackRef::send_modulated_param(uint8_t dest, uint8_t param,
                                       uint8_t value,
                                       MidiUartClass *uart_,
                                       MidiUartClass *uart2_, uint8_t offset) {
  uint8_t idx = 0;
  uint8_t dest_type = lfo_custom_dest_index(dest, &idx);
  if (dest_type == LFO_DEST_PERF) {
    if (lfo_perf_encoder(idx, param) == nullptr) {
      return false;
    }
    perf_page.set_lfo_mod(idx, (int8_t)((int16_t)value - (int16_t)offset));
    return true;
  }
  if (dest_type == LFO_DEST_TRACK) {
    return lfo_track_dispatch(idx, param, &value,
                              LFO_TRACK_PARAM_SET_MODULATED);
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
