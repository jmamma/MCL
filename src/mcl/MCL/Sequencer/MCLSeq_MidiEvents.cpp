#include "MCLSeq_Internal.h"

#include "GUI/Pages/CommonPages.h"
#include "Devices/DeviceParamResolver.h"
#include "MCL.h"
#include "MCLPlatformFeatures.h"
#include "MDPages.h"
#include "Devices/MidiSetup.h"
#include "Sequencer/SeqTrackUtil.h"
#include "../../Drivers/Generic/GenericMidiDevice.h"
#include "../../Drivers/MD/MD.h"
#if MCL_FEATURE_HOST_ARRANGER
#include "Arrangement/MCLArrangement.h"
#endif
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
#include "Host/SpsHostArrBridge.h"
#endif

namespace {

#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
uint8_t arrangement_record_device(DeviceIdx device_idx) {
  return device_idx == DeviceIdx::Secondary ? 1 : 0;
}

void notify_arranger_recorded() {
  sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)spsarr::DIRTY_ARRANGEMENT);
}

void record_arranger_param_cc(DeviceIdx device_idx, uint8_t track,
                              uint8_t track_param, uint8_t value,
                              bool mute_param) {
  if (!mcl_arrangement.automationRecordArmed() || track >= NUM_MD_TRACKS) {
    return;
  }
  uint8_t device = arrangement_record_device(device_idx);
  if (mute_param) {
    if (mcl_arrangement.recordAutomationPoint(
        track, mclarrfile::AUTOMATION_TARGET_MUTE, device, 0,
        mclarrfile::AUTOMATION_VALUE_BOOL, value != 0 ? 1 : 0,
        mclarrfile::AUTOMATION_INTERP_HOLD, 0)) {
      notify_arranger_recorded();
    }
    return;
  }
  if (mcl_arrangement.recordAutomationPoint(
      track, mclarrfile::AUTOMATION_TARGET_MD_PARAM, device, track_param,
      mclarrfile::AUTOMATION_VALUE_U7, value,
      mclarrfile::AUTOMATION_INTERP_CURVE, 0)) {
    notify_arranger_recorded();
  }
}

void record_arranger_fill_cc(DeviceIdx device_idx, uint8_t track,
                             uint8_t value) {
  if (!mcl_arrangement.automationRecordArmed() || track >= NUM_MD_TRACKS) {
    return;
  }
  if (mcl_arrangement.recordAutomationPoint(
      track, mclarrfile::AUTOMATION_TARGET_FILL,
      arrangement_record_device(device_idx), 0,
      mclarrfile::AUTOMATION_VALUE_BOOL, value != 0 ? 1 : 0,
      mclarrfile::AUTOMATION_INTERP_HOLD, 0)) {
    notify_arranger_recorded();
  }
}
#endif

bool handle_mixer_cc(DeviceIdx device_idx, MidiDevice *device, uint8_t channel,
                     uint8_t cc, uint8_t value, uint8_t *track_out,
                     uint8_t *param_out) {
  if (device == nullptr || device_idx == DeviceIdx::None) {
    return false;
  }
  DeviceContext ctx = DeviceContext::for_device(device, device_idx);
  uint8_t track = 255;
  uint8_t track_param = 255;
  DeviceMixerCapability *mixer = device->mixer();
  if (mixer == nullptr) {
    return false;
  }
  if (!mixer->parse_cc(ctx, channel, cc, &track, &track_param) ||
      track == 255) {
    return false;
  }

  mixer->update_from_cc(ctx, track, track_param, value);
  *track_out = track;
  *param_out = track_param;

  bool mute_param = mixer->is_mute_param(track_param);
#if MCL_FEATURE_HOST_ARRANGER
  if (!mute_param) {
    GridSlot slot = device_idx == DeviceIdx::Secondary
                        ? (GridSlot)(GRID_WIDTH + track)
                        : (GridSlot)track;
    mcl_arrangement.markRuntimePrivateSourceEdited(slot);
  }
#endif
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
  record_arranger_param_cc(device_idx, track, track_param, value, mute_param);
#endif

  if (mute_param) {
    mixer_page.redraw_mutes = true;
    return true;
  }

  if (mcl.currentPage() == MIXER_PAGE) {
    mixer_page.onControlChangeCallback_Midi(device_idx, track, track_param,
                                            value);
  }

  uint8_t perf_dest =
      DeviceParamResolver::perf_data_dest_for_target(device_idx, track);
  if (perf_dest != DeviceParamResolver::INVALID_PERF_DATA_DEST) {
    perf_page.learn_param(perf_dest, track_param, value);
    lfo_page.learn_perf_dest(perf_dest + 1, track_param, value);
  }
  return true;
}

void setup_mcl_seq_md_midi(MCLSeqMidiEvents *events, MidiClass *midi) {
  if (midi == nullptr) return;
  midi->addOnNoteOnCallback(
      events, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  midi->addOnNoteOffCallback(
      events, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  midi->addOnControlChangeCallback(
      events,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
}

void setup_mcl_seq_secondary_midi(MCLSeqMidiEvents *events, MidiClass *midi) {
  if (midi == nullptr) return;
  midi->addOnControlChangeCallback(
      events,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);
}

#ifdef PLATFORM_TBD
MidiClass *mcl_seq_secondary_midi() {
  PortSlot slots[SLOT_COUNT];
  resolve_slots(slots);
  const PortSlot &slot =
      mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT ? slots[SLOT_ELEKT]
                                                   : slots[SLOT_GENER];
  if (slot.midi != nullptr) {
    return slot.midi;
  }
  MidiDevice *secondary = device_manager.secondary_device();
  return secondary->midi != nullptr ? secondary->midi : generic_midi_device.midi;
}
#endif

void cleanup_mcl_seq_midi(MCLSeqMidiEvents *events, MidiClass *midi) {
  if (midi == nullptr) return;
  midi->removeOnNoteOnCallback(
      events, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  midi->removeOnNoteOffCallback(
      events, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  midi->removeOnControlChangeCallback(
      events,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
  midi->removeOnControlChangeCallback(
      events,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);
}

void cleanup_mcl_seq_all_midi(MCLSeqMidiEvents *events) {
  cleanup_mcl_seq_midi(events, &Midi);
  cleanup_mcl_seq_midi(events, &Midi2);
  cleanup_mcl_seq_midi(events, &MidiUSB);
#ifdef PLATFORM_TBD
  cleanup_mcl_seq_midi(events, &MidiP4);
#endif
}

} // namespace

void MCLSeqMidiEvents::onNoteCallback_Midi(uint8_t *msg) {
  uint8_t n = MD.noteToTrack(msg[1]);
  if (n < 16) {
    bool is_midi_machine = ((MD.kit.models[n] & 0xF0) == MID_01_MODEL);
    if (is_midi_machine) {
      SeqTrackUtil::with_md_track(n, [&](auto &track) {
        if (msg[2]) { track.send_notes(255); }
        else if (MidiClock.state != 2) { track.send_notes_off(); }
      });
    }
    if ((msg[0] & 0x10) && msg[2]) {
      mixer_page.track_trig(DeviceIdx::Primary, n, MD.kit.levels[n]);
    }
  }
}

void MCLSeqMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track = 255;
  uint8_t track_param = 255;

  uint8_t fill_track = param - 68;
  uint8_t control_ch = channel - MD.global.baseChannel;
  if (fill_track < 4 && control_ch < 4) {
    uint8_t track = (control_ch << 2) + fill_track;
    mcl_seq.set_fill_track(DeviceIdx::Primary, track, value);
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
    record_arranger_fill_cc(DeviceIdx::Primary, track, value);
#endif
    return;
  }

  MidiDevice *device = device_manager.primary_device();
  if (!handle_mixer_cc(DeviceIdx::Primary, device, channel, param, value, &track,
                       &track_param) ||
      track > 15) {
    return;
  }

  if (SeqTrackUtil::is_md_device(device)) {
    SeqTrackUtil::with_md_track(track, [track_param, value](auto &t) {
      t.onControlChangeCallback_Midi(track_param, value);
    });
    ram_page_a.onControlChangeCallback_Midi(track, track_param, value);
  }
}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];

  MidiDevice *device = device_manager.secondary_device();
  if (handle_mixer_cc(DeviceIdx::Secondary, device, channel, param, value,
                      &channel, &param)) {
    return;
  }

  uint8_t track = mcl_seq.find_ext_track(channel);
  if (track != 255) {
    uint8_t perf_dest =
        DeviceParamResolver::perf_data_dest_for_target(DeviceIdx::Secondary,
                                                       track);
    if (perf_dest != DeviceParamResolver::INVALID_PERF_DATA_DEST) {
      perf_page.learn_param(perf_dest, param, value);
      lfo_page.learn_perf_dest(perf_dest + 1, param, value);
    }
  }
}

void MCLSeqMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  if (seq_grid_x_runs_md_tracks()) {
    setup_mcl_seq_md_midi(this, MD.midi);
  }
#ifdef EXT_TRACKS
#ifdef PLATFORM_TBD
  setup_mcl_seq_secondary_midi(this, mcl_seq_secondary_midi());
#else
  MidiDevice *secondary = device_manager.secondary_device();
  MidiClass *secondary_midi =
      secondary->midi != nullptr ? secondary->midi : generic_midi_device.midi;
  setup_mcl_seq_secondary_midi(this, secondary_midi);
#endif
#endif
  state = true;
}

void MCLSeqMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }

  cleanup_mcl_seq_all_midi(this);
  state = false;
}
