#include "PerfPageTargetRef.h"

#include "LFOTrackRef.h"
#include "NoteInterface.h"
#include "SeqPages.h"

uint8_t PerfPageTarget::lfo_dest() const {
  uint8_t base = DeviceParamResolver::perf_target_count();
  if (dest <= base) {
    return 0;
  }
  return LFOTrackRef::track_lfo_dest_for_index(dest - base - 1);
}

DevicePerfTarget PerfPageTarget::device_target() const {
  return DeviceParamResolver::perf(dest);
}

bool PerfPageTarget::valid() const {
  uint8_t lfo = lfo_dest();
  if (lfo != 0) {
    return LFOTrackRef::param_count(DeviceIdx::None, lfo) > 0;
  }
  return device_target().valid();
}

DeviceIdx PerfPageTarget::device_index() const {
  uint8_t lfo = lfo_dest();
  return lfo != 0 ? DeviceIdx::None : device_target().device_index();
}

uint8_t PerfPageTarget::param_count() const {
  uint8_t lfo = lfo_dest();
  return lfo != 0 ? LFOTrackRef::param_count(DeviceIdx::None, lfo)
                  : device_target().param_count();
}

bool PerfPageTarget::target_label(char *out, uint8_t len) const {
  uint8_t lfo = lfo_dest();
  return lfo != 0 ? LFOTrackRef::target_label(DeviceIdx::None, lfo, out, len)
                  : device_target().target_label(out, len);
}

bool PerfPageTarget::param_label(uint8_t param, char *out, uint8_t len) const {
  uint8_t lfo = lfo_dest();
  return lfo != 0
             ? LFOTrackRef::param_label(DeviceIdx::None, lfo, param, out, len)
             : device_target().param_label(param, out, len);
}

bool PerfPageTarget::get_param(uint8_t param, uint8_t *value) const {
  uint8_t lfo = lfo_dest();
  return lfo != 0 ? LFOTrackRef::get_base_param(DeviceIdx::None, lfo, param,
                                                value)
                  : device_target().get_param(param, value);
}

bool PerfPageTarget::set_param(uint8_t param, uint8_t value,
                               MidiUartClass *uart_,
                               MidiUartClass *uart2_) const {
  uint8_t lfo = lfo_dest();
  if (lfo != 0) {
    return LFOTrackRef::send_modulated_param(DeviceIdx::None, lfo, param,
                                             value, uart_, uart2_, value);
  }
  DevicePerfTarget target = device_target();
  return target.set_param(
      param, value, target.device_index() == DeviceIdx::Secondary ? uart2_
                                                                  : uart_);
}

bool PerfPageTarget::param_from_key(uint8_t key, uint8_t *param) const {
  uint8_t lfo = lfo_dest();
  return lfo == 0 && device_target().param_from_key(key, param);
}

bool PerfPageTarget::key_for_param(uint8_t param, uint8_t *key) const {
  uint8_t lfo = lfo_dest();
  return lfo == 0 && device_target().key_for_param(param, key);
}

bool PerfPageTarget::begin_param_editor(uint8_t *params,
                                        uint8_t count) const {
  uint8_t lfo = lfo_dest();
  return lfo == 0 && device_target().begin_param_editor(params, count);
}

uint8_t PerfPageTargetRef::target_count() {
  return DeviceParamResolver::perf_target_count() +
         LFOTrackRef::track_lfo_target_count();
}

PerfPageTarget PerfPageTargetRef::target(uint8_t dest) {
  return PerfPageTarget(dest);
}

uint8_t PerfPageTargetRef::active_editor_dest() {
  return DeviceParamResolver::primary_perf_editor_dest(
      seq_primary_track_index());
}

bool PerfPageTargetRef::begin_editor(uint8_t dest, uint8_t *params,
                                     uint8_t count) {
  return target(dest).begin_param_editor(params, count);
}

void PerfPageTargetRef::end_editor() {
  DeviceParamResolver::end_perf_param_editor();
}

void PerfPageTargetRef::set_rec_mode(uint8_t mode) {
  DeviceParamResolver::set_perf_rec_mode(mode);
}

uint8_t PerfPageTargetRef::pressed_scene() {
  return note_interface.get_first_trig_note();
}
