/* Justin Mammarella jmamma@gmail.com 2018 */

#include "GUI/Pages/Performance/MixerPage.h"

#include "DeviceManager.h"
#include "GUI/Pages/Performance/MixerPerf.h"
#include "NoteInterface.h"
#include "SeqTrackUtil.h"
#include "../../../../Drivers/MD/MD.h"
#include "../../../../Drivers/MidiDevice.h"

namespace {

MidiDevice *fallback_primary_mixer_device() {
  return &MD;
}

#if !defined(__AVR__)
bool mixer_param_supported_for_held_tracks(const MixerTarget &target,
                                           uint8_t param) {
  uint8_t len = target.track_count();
  for (uint8_t i = 0; i < len; i++) {
    if (note_interface.is_note_on(i)) {
      MidiDeviceMixerParam info;
      if (target.param(i, param, &info)) {
        return true;
      }
    }
  }
  return false;
}
#endif

} // namespace

void MixerTarget::bind(DeviceIdx device_idx) {
  bind(device_manager.device_for_idx(device_idx), device_idx);
}

void MixerTarget::bind(MidiDevice *device, DeviceIdx device_idx) {
  if (device == nullptr || device == &null_midi_device) {
    ctx_ = DeviceContext::for_device(&null_midi_device, device_idx);
    mixer_ = nullptr;
    return;
  }
  ctx_ = DeviceContext::for_device(device, device_idx);
  mixer_ = device->mixer();
}

bool MixerTarget::bind_selected(DeviceIdx &device_idx) {
  bind(device_idx);
  if (!is_null()) {
    return true;
  }

  device_idx = DeviceIdx::Primary;
  bind(device_idx);
  if (!is_null()) {
    return true;
  }

  device_idx = DeviceIdx::Secondary;
  bind(device_idx);
  if (!is_null()) {
    return true;
  }

  device_idx = DeviceIdx::Primary;
  bind(fallback_primary_mixer_device(), DeviceIdx::Primary);
  return !is_null();
}

MidiDevice *MixerTarget::device() const {
  return ctx_.device();
}

bool MixerTarget::is_null() const {
  return mixer_ == nullptr;
}

bool MixerTarget::is_md_device() const {
  return SeqTrackUtil::is_md_device(device());
}

bool MixerTarget::perf_available() const {
  return MixerPerf::available(device());
}

uint8_t MixerTarget::default_param() const {
  return mixer_ != nullptr ? mixer_->default_param() : 0;
}

uint8_t MixerTarget::param_for_encoder(uint8_t encoder_idx, uint8_t display_mode,
                                       bool use_perf) const {
  if (use_perf) {
    return MixerPerf::mixer_param_for_encoder(encoder_idx);
  }
#if !defined(__AVR__)
  if (mixer_param_supported_for_held_tracks(*this, encoder_idx)) {
    return encoder_idx;
  }
  if (mixer_param_supported_for_held_tracks(*this, display_mode)) {
    return display_mode;
  }
#else
  (void)display_mode;
#endif
  return default_param();
}

uint8_t MixerTarget::track_count() const {
  if (mixer_ == nullptr) {
    return 0;
  }
  uint8_t count = mixer_->track_count(ctx_);
  return count > 16 ? 16 : count;
}

SeqTrack *MixerTarget::seq_track(uint8_t track) const {
  return mixer_ != nullptr ? mixer_->seq_track(ctx_, track) : nullptr;
}

bool MixerTarget::param(uint8_t track, uint8_t param_idx,
                        MidiDeviceMixerParam *out) const {
  return mixer_ != nullptr && mixer_->param(ctx_, track, param_idx, out);
}

uint8_t MixerTarget::param_value_7bit(const MidiDeviceMixerParam &param) const {
#ifdef PLATFORM_TBD
  if (param.max_value <= param.min_value) {
    return 0;
  }

  int32_t value = param.value;
  if (value < param.min_value) value = param.min_value;
  if (value > param.max_value) value = param.max_value;
  const uint16_t range = (uint16_t)(param.max_value - param.min_value);
  return (uint8_t)(((uint32_t)(value - param.min_value) * 127u +
                    (range / 2u)) /
                   range);
#else
  return param.value;
#endif
}

MidiDeviceMixerValue MixerTarget::clamp_param_value(
    const MidiDeviceMixerParam &param, int16_t value) const {
#ifdef PLATFORM_TBD
  if (value < param.min_value) return param.min_value;
  if (value > param.max_value) return param.max_value;
  return value;
#else
  (void)param;
  if (value < 0) return 0;
  if (value > 127) return 127;
  return value;
#endif
}

bool MixerTarget::set_param(uint8_t track, uint8_t param_idx,
                            MidiDeviceMixerValue value, bool send) const {
  return mixer_ != nullptr &&
         mixer_->set_param(ctx_, track, param_idx, value, send);
}

bool MixerTarget::set_seq_mute_state(uint8_t track, bool mute) const {
  return mixer_ != nullptr && mixer_->set_seq_mute_state(ctx_, track, mute);
}

void MixerTarget::mute_track(uint8_t track, bool mute) const {
  if (mixer_ != nullptr) {
    mixer_->mute_track(ctx_, track, mute);
  }
}

#if !defined(__AVR__)
void MixerTarget::fill_track(uint8_t track, bool fill) const {
  if (mixer_ != nullptr) {
    mixer_->fill_track(ctx_, track, fill);
  }
}
#endif

void MixerTarget::set_record_mutes(uint8_t track, bool state,
                                   bool clear) const {
  if (mixer_ != nullptr) {
    mixer_->set_record_mutes(ctx_, track, state, clear);
  }
}

uint8_t MixerTarget::trig_group(uint8_t track) const {
  return mixer_ != nullptr ? mixer_->trig_group(ctx_, track) : 255;
}

void MixerTarget::select_track(uint8_t track) const {
  if (mixer_ != nullptr) {
    mixer_->select_track(ctx_, track);
  }
}

void MixerTarget::restore_track_params(uint8_t track) const {
  if (mixer_ != nullptr) {
    mixer_->restore_track_params(ctx_, track);
  }
}

bool MixerTarget::is_mute_param(uint8_t param) const {
  return mixer_ != nullptr && mixer_->is_mute_param(param);
}

PageIndex MixerTarget::driver_mixer_page() const {
  return device() == &MD ? MD.mixer_fx_page() : NULL_PAGE;
}
