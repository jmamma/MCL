#pragma once

#include "LFOSeqTrack.h"
#include <inttypes.h>

class LFOTrackRef {
public:
  static LFOSeqTrack &current_track();
  static bool select_track(uint8_t track);

  static uint8_t track_count(DeviceIdx device_idx);

  static uint8_t target_count(DeviceIdx device_idx);
  static bool target_label(DeviceIdx device_idx, uint8_t dest, char *out,
                           uint8_t len);
  static uint8_t param_count(DeviceIdx device_idx, uint8_t dest);
  static bool param_label(DeviceIdx device_idx, uint8_t dest, uint8_t param,
                          char *out, uint8_t len);
  static bool get_base_param(DeviceIdx device_idx, uint8_t dest, uint8_t param,
                             uint8_t *value);
  static bool set_base_param(DeviceIdx device_idx, uint8_t dest, uint8_t param,
                             uint8_t value);
  static bool send_modulated_param(DeviceIdx device_idx, uint8_t dest,
                                   uint8_t param, uint8_t value,
                                   MidiUartClass *uart_ = nullptr,
                                   MidiUartClass *uart2_ = nullptr,
                                   uint8_t offset = 0);

  static void sync_panel(const LFOSeqTrack &track);
  static bool supports_trig_port(uint8_t port);
};
