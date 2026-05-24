#pragma once

#include "DeviceParamResolver.h"
#include <inttypes.h>

class PerfPageTarget {
public:
  uint8_t dest = 0;

  PerfPageTarget() = default;
  explicit PerfPageTarget(uint8_t dest_) : dest(dest_) {}

  uint8_t lfo_dest() const;
  DevicePerfTarget device_target() const;
  bool valid() const;
  DeviceIdx device_index() const;
  uint8_t param_count() const;
  bool target_label(char *out, uint8_t len) const;
  bool param_label(uint8_t param, char *out, uint8_t len) const;
  bool get_param(uint8_t param, uint8_t *value) const;
  bool set_param(uint8_t param, uint8_t value,
                 MidiUartClass *uart_ = nullptr,
                 MidiUartClass *uart2_ = nullptr) const;
  bool param_from_key(uint8_t key, uint8_t *param) const;
  bool key_for_param(uint8_t param, uint8_t *key) const;
  bool begin_param_editor(uint8_t *params, uint8_t count) const;
};

class PerfPageTargetRef {
public:
  static uint8_t target_count();
  static PerfPageTarget target(uint8_t dest);
  static uint8_t active_editor_dest();
  static bool begin_editor(uint8_t dest, uint8_t *params, uint8_t count);
  static void end_editor();
  static void set_rec_mode(uint8_t mode);
  static uint8_t pressed_scene();
};
