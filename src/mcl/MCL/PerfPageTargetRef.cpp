#include "PerfPageTargetRef.h"

#include "LFOTrackRef.h"
#include "NoteInterface.h"
#include "SeqPages.h"

namespace {

uint8_t perf_lfo_dest_base() {
#if defined(__AVR__)
  return NUM_MD_TRACKS + 4 + DeviceParamResolver::RESERVED_SECONDARY_TARGETS;
#else
  return DeviceParamResolver::perf_target_count();
#endif
}

inline __attribute__((always_inline)) uint8_t perf_page_lfo_dest(uint8_t dest) {
  uint8_t base = perf_lfo_dest_base();
  if (dest <= base) {
    return 0;
  }
  uint8_t idx = dest - base - 1;
  return LFOTrackRef::track_lfo_dest_for_index(idx);
}

} // namespace

DeviceIdx PerfPageTarget::device_index() const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo != 0 ? DeviceIdx::None
                  : DeviceParamResolver::perf(dest).device_index();
}

uint8_t PerfPageTarget::param_count() const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo != 0 ? LFOTrackRef::param_count(DeviceIdx::None, lfo)
                  : DeviceParamResolver::perf(dest).param_count();
}

bool PerfPageTarget::target_label(char *out, uint8_t len) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo != 0 ? LFOTrackRef::target_label(DeviceIdx::None, lfo, out, len)
                  : DeviceParamResolver::perf(dest).target_label(out, len);
}

bool PerfPageTarget::param_label(uint8_t param, char *out, uint8_t len) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo != 0
             ? LFOTrackRef::param_label(DeviceIdx::None, lfo, param, out, len)
             : DeviceParamResolver::perf(dest).param_label(param, out, len);
}

bool PerfPageTarget::get_param(uint8_t param, uint8_t *value) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo != 0 ? LFOTrackRef::get_base_param(DeviceIdx::None, lfo, param,
                                                value)
                  : DeviceParamResolver::perf(dest).get_param(param, value);
}

bool PerfPageTarget::set_param(uint8_t param, uint8_t value,
                               MidiUartClass *uart_,
                               MidiUartClass *uart2_) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  if (lfo != 0) {
    return LFOTrackRef::send_modulated_param(DeviceIdx::None, lfo, param,
                                             value, uart_, uart2_, value);
  }
  DevicePerfTarget target = DeviceParamResolver::perf(dest);
  return target.set_param(
      param, value, target.device_index() == DeviceIdx::Secondary ? uart2_
                                                                  : uart_);
}

bool PerfPageTarget::param_from_key(uint8_t key, uint8_t *param) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo == 0 && DeviceParamResolver::perf(dest).param_from_key(key, param);
}

bool PerfPageTarget::key_for_param(uint8_t param, uint8_t *key) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo == 0 && DeviceParamResolver::perf(dest).key_for_param(param, key);
}

bool PerfPageTarget::begin_param_editor(uint8_t *params,
                                        uint8_t count) const {
  uint8_t lfo = perf_page_lfo_dest(dest);
  return lfo == 0 &&
         DeviceParamResolver::perf(dest).begin_param_editor(params, count);
}

uint8_t PerfPageTargetRef::target_count() {
  return perf_lfo_dest_base() + LFOTrackRef::track_lfo_target_count();
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
