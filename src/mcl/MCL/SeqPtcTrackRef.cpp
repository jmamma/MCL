#include "SeqPtcTrackRef.h"

#include "../Drivers/MD/MD.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#include "Sequencer/MCLSeq.h"
#if defined(PLATFORM_TBD)
#include "MCLSysConfig.h"
#include "MidiSetup.h"
#endif
#include "PtcGroups.h"
#include "GUI/Pages/Sequencer/SeqPages.h"

namespace {

#if defined(PLATFORM_TBD)
bool ptc_primary_uses_tbd_tracks() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}

bool ptc_tbd_track_available(uint8_t track) {
  return ptc_primary_uses_tbd_tracks() && track < mcl_seq.num_tbd_tracks;
}
#endif

} // namespace

uint8_t SeqPtcTrackRef::track_count() { return PTC_GROUP_TRACKS; }

bool SeqPtcTrackRef::is_poly_voice_track(uint8_t track) {
  if (track >= track_count()) {
    return false;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    return true;
  }
#endif
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  return device != nullptr &&
         device->step_edit()->kit_sound_voice_allocatable(
             device_manager.primary_context(), track);
#else
  return MD.isMelodicTrack(track);
#endif
}

bool SeqPtcTrackRef::is_midi_voice_track(uint8_t track) {
  if (track >= track_count()) {
    return false;
  }
#if defined(PLATFORM_TBD)
  if (ptc_primary_uses_tbd_tracks()) {
    return false;
  }
#endif
  return (MD.kit.models[track] & 0xF0) == MID_01_MODEL;
}

bool SeqPtcTrackRef::can_polylink_param(uint8_t source_track,
                                        uint8_t target_track,
                                        uint8_t param) {
  if (source_track >= track_count() || target_track >= track_count()) {
    return false;
  }
#if defined(PLATFORM_TBD)
  if (ptc_primary_uses_tbd_tracks()) {
    return false;
  }
#endif
  return (param < 24 && param > 7) ||
         (param < 8 &&
          MD.kit.models[target_track] == MD.kit.models[source_track]);
}

bool SeqPtcTrackRef::is_mute_param(uint8_t param) {
  return param == MODEL_MUTE;
}

uint8_t SeqPtcTrackRef::note_from_pitch(uint8_t track, uint8_t pitch) {
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return 255;
  }
  DeviceContext ctx = device_manager.primary_context();
  return device->step_edit()->kit_sound_note_from_pitch(ctx, track, pitch);
#else
  if (track >= track_count()) {
    return 255;
  }
  if (is_midi_voice_track(track)) {
    return pitch;
  }

  tuning_t const *tuning = MD.getKitModelTuning(track);
  pitch -= ptc_param_fine_tune.getValue() - 32;
  if (pitch != 255 && tuning) {
    for (uint8_t i = 0; i < tuning->len; i++) {
      uint8_t cc_stored = pgm_read_byte(&tuning->tuning[i]);
      if (cc_stored >= pitch) {
        uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
        return i + note_offset;
      }
    }
  }
  return 255;
#endif
}

uint8_t SeqPtcTrackRef::pitch_from_note(uint8_t track, uint8_t note,
                                        uint8_t fine_tune) {
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return 255;
  }
  DeviceContext ctx = device_manager.primary_context();
  return device->step_edit()->kit_sound_pitch_from_note(ctx, track, note,
                                                        fine_tune);
#else
  if (track >= track_count()) {
    return 255;
  }
  if (is_midi_voice_track(track)) {
    return note;
  }
  if (fine_tune == 255) {
    fine_tune = ptc_param_fine_tune.getValue();
  }

  tuning_t const *tuning = MD.getKitModelTuning(track);
  if (tuning == nullptr) {
    return 255;
  }

  uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
  note = note - note_offset;
  if (note >= tuning->len) {
    return 255;
  }

  int8_t pitch = (int8_t)pgm_read_byte(&tuning->tuning[note]) +
                 (int8_t)fine_tune - 32;
  if (pitch < 0) {
    return 0;
  }
  return pitch > 127 ? 127 : (uint8_t)pitch;
#endif
}

bool SeqPtcTrackRef::parse_cc(uint8_t channel, uint8_t cc, uint8_t *track,
                              uint8_t *param) {
  if (track == nullptr || param == nullptr) {
    return false;
  }
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return false;
  }
  DeviceContext ctx = device_manager.primary_context();
  return device->step_edit()->parse_cc(ctx, channel, cc, track, param);
#else
  MD.parseCC(channel, cc, track, param);
  return *track != 255;
#endif
}

bool SeqPtcTrackRef::set_param(uint8_t track, uint8_t param, uint8_t value,
                               MidiUartClass *uart_, bool update_kit) {
  if (track >= track_count()) {
    return false;
  }
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return false;
  }
  DeviceContext ctx = device_manager.primary_context();
  return device->params()->set_param(ctx, track, param, value, uart_,
                                     update_kit);
#else
  MD.setTrackParam(track, param, value, uart_, update_kit);
  return true;
#endif
}

bool SeqPtcTrackRef::set_route_param(uint8_t track, uint8_t param,
                                     uint8_t value) {
  if (is_midi_voice_track(track) && param < MID_PB) {
    return true;
  }

  if (!set_param(track, param, value, nullptr, false)) {
    return false;
  }

#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    return true;
  }
#endif
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr || device->id != DEVICE_MD) {
    return true;
  }
  if (mcl_seq.using_spsx_tracks) {
    mcl_seq.spsx_tracks[track].onControlChangeCallback_Midi(param, value);
    return true;
  }
#endif
  mcl_seq.md_tracks[track].onControlChangeCallback_Midi(param, value);
  return true;
}

bool SeqPtcTrackRef::set_pitch(uint8_t track, uint8_t pitch,
                               MidiUartClass *uart_) {
  return set_param(track, 0, pitch, uart_);
}

void SeqPtcTrackRef::trigger(uint8_t track, uint8_t velocity,
                             MidiUartClass *uart_) {
  if (track >= track_count()) {
    return;
  }
#ifdef LFO_TRACKS
  mcl_seq.report_track_trig(DeviceIdx::Primary, track);
#endif
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return;
  }
  device->triggerTrack(track, velocity, uart_);
#else
  MD.triggerTrack(track, velocity, uart_);
#endif
}

bool SeqPtcTrackRef::trigger_voice(uint8_t track, uint8_t note,
                                   uint8_t fine_tune,
                                   MidiUartClass *uart_,
                                   uint8_t *record_pitch) {
  if (track >= track_count()) {
    return false;
  }

  if (is_midi_voice_track(track)) {
    send_notes_off(track);
    send_notes(track, note);
    if (record_pitch != nullptr) {
      *record_pitch = note;
    }
    return true;
  }

#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    (void)fine_tune;
    (void)uart_;
    send_notes(track, note);
    if (record_pitch != nullptr) {
      *record_pitch = note;
    }
    return true;
  }
#endif

  uint8_t machine_pitch = pitch_from_note(track, note, fine_tune);
  if (machine_pitch == 255) {
    return false;
  }
  set_pitch(track, machine_pitch, uart_);
  trigger(track, 127, uart_);
  if (record_pitch != nullptr) {
    *record_pitch = machine_pitch;
  }
  return true;
}

bool SeqPtcTrackRef::release_voice(uint8_t track) {
  if (track >= track_count()) {
    return false;
  }
  if (is_midi_voice_track(track)) {
    send_notes_off(track);
    return true;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    send_notes_off(track);
    return true;
  }
#endif
  return false;
}

void SeqPtcTrackRef::send_notes_on(uint8_t track) {
  if (track >= track_count()) {
    return;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    return;
  }
#endif
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    mcl_seq.spsx_tracks[track].send_notes_on();
    return;
  }
#endif
  mcl_seq.md_tracks[track].send_notes_on();
}

void SeqPtcTrackRef::send_notes_off(uint8_t track) {
  if (track >= track_count()) {
    return;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    mcl_seq.tbd_tracks[track].note_off();
    return;
  }
#endif
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    mcl_seq.spsx_tracks[track].send_notes_off();
    return;
  }
#endif
  mcl_seq.md_tracks[track].send_notes_off();
}

void SeqPtcTrackRef::send_notes(uint8_t track, uint8_t pitch) {
  if (track >= track_count()) {
    return;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    mcl_seq.tbd_tracks[track].note_on(pitch, 127);
    return;
  }
#endif
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    mcl_seq.spsx_tracks[track].send_notes(pitch);
    return;
  }
#endif
  mcl_seq.md_tracks[track].send_notes(pitch);
}

void SeqPtcTrackRef::record_track(uint8_t track, uint8_t velocity) {
  if (track >= track_count()) {
    return;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    mcl_seq.tbd_tracks[track].record_track(velocity);
    return;
  }
#endif
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    mcl_seq.spsx_tracks[track].record_track(velocity);
    return;
  }
#endif
  mcl_seq.md_tracks[track].record_track(velocity);
}

void SeqPtcTrackRef::record_pitch(uint8_t track, uint8_t pitch) {
  if (track >= track_count()) {
    return;
  }
#if defined(PLATFORM_TBD)
  if (ptc_tbd_track_available(track)) {
    mcl_seq.tbd_tracks[track].record_track_pitch(pitch);
    return;
  }
#endif
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    mcl_seq.spsx_tracks[track].record_track_pitch(pitch);
    return;
  }
#endif
  mcl_seq.md_tracks[track].record_track_pitch(pitch);
}

bool SeqPtcTrackRef::copy_track_label(uint8_t track, char *out, uint8_t len) {
  if (out == nullptr || len < 6 || track >= track_count()) {
    return false;
  }
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return false;
  }
  uint8_t pitch_max = 0;
  bool is_midi_model = false;
  DeviceContext ctx = device_manager.primary_context();
  return device->step_edit()->configure_kit_sound_panel(
      ctx, track, out, len, &pitch_max, &is_midi_model);
#else
  uint8_t model = MD.kit.get_model(track);
  const char *str = getMDMachineNameShort(model, 1);
  copyMachineNameShort(str, out);
  out[2] = '>';
  str = getMDMachineNameShort(model, 2);
  copyMachineNameShort(str, out + 3);
  out[5] = '\0';
  return true;
#endif
}

void SeqPtcTrackRef::popup_text(char *text) {
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  if (device == nullptr) {
    return;
  }
  device->step_edit()->popup_text(device_manager.primary_context(), text);
#else
  MD.popup_text(text);
#endif
}

MidiClass *SeqPtcTrackRef::param_midi() {
#if !defined(__AVR__)
  MidiDevice *device = device_manager.primary_device();
  return device != nullptr ? device->midi : nullptr;
#else
  return MD.midi;
#endif
}
