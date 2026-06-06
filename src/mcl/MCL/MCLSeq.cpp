#include "DiagnosticPage.h"
#include "MCLSeq.h"
#include "SeqTrackUtil.h"
#if !defined(__AVR__)
#include "SpsHostArrBridge.h"  // SPS<->MCL arranger cell listener
#include "SpsHostSeqBridge.h"  // SPS<->MCL seq control listener
#endif
#include "MCLGUI.h"
#include "SeqPages.h"
#include "MCL.h"
#include "CommonPages.h"
#include "MDPages.h"
#include "MCLStrings.h"
#include "MCLSysConfig.h"
#include "MidiSetup.h"
#include "DeviceManager.h"
#include "DeviceParamResolver.h"
#include "global.h"
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "../Drivers/Generic/Sequencer/TrackLoadFadeRunner.h"
#if !defined(__AVR__)
#include "../Drivers/Generic/Sequencer/StepSeqDefines.h"
#endif
#if defined(PLATFORM_TBD)
#include "../Drivers/TBD/TBDTrack.h"
#endif
#include "../Drivers/MD/MD.h"

namespace {

bool seq_grid_x_runs_md_tracks() {
#ifdef PLATFORM_TBD
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_MD;
#else
  return true;
#endif
}

#if !defined(__AVR__)
uint32_t legacy_tick_count_from_div192(uint32_t div192) {
  uint32_t divider = MidiClock.clock_interpolation / LEGACY_SEQ_INTERPOLATION;
  if (divider == 0) {
    divider = 1;
  }
  return (div192 + divider - 1u) / divider;
}

template <typename Track>
void sync_seq_track_phase(Track &track, uint32_t track_ticks) {
  track.reset();
  const uint16_t ticks_per_step = track.get_ticks_per_step();
  const uint16_t length = track.length ? track.length : 1;
  if (ticks_per_step == 0) {
    return;
  }

  const uint32_t total_steps = track_ticks / ticks_per_step;
  const uint16_t tick_in_step = track_ticks % ticks_per_step;
  track.step_count = (uint8_t)(total_steps % length);
  track.mod12_counter = tick_in_step == 0
                            ? (uint8_t)0xffu
                            : (uint8_t)(tick_in_step - 1u);
  track.count_down = 0;
  track.cache_loaded = true;
}

void sync_md_track_phase(MDSeqTrack &track, uint32_t track_ticks) {
  sync_seq_track_phase(track, track_ticks);
  track.cur_event_idx = track.get_lockidx(track.step_count);
  uint8_t tps = track.get_ticks_per_step();
  uint8_t len = track.length ? track.length : 1;
  if (tps && track_ticks / tps < len) {
    track.conditional_flags |= SeqTrackCond::CONDITIONAL_FIRST_RUN;
  } else {
    track.conditional_flags &= ~SeqTrackCond::CONDITIONAL_FIRST_RUN;
  }
}

#ifdef EXT_TRACKS
void sync_ext_track_phase(ExtSeqTrack &track, uint32_t track_ticks) {
  sync_seq_track_phase(track, track_ticks);
  uint16_t end = 0;
  track.locate(track.step_count, track.cur_event_idx, end);
  uint8_t tps = track.get_ticks_per_step();
  uint8_t len = track.length ? track.length : 1;
  if (tps && track_ticks / tps < len) {
    track.conditional_flags |= SeqTrackCond::CONDITIONAL_FIRST_RUN;
  } else {
    track.conditional_flags &= ~SeqTrackCond::CONDITIONAL_FIRST_RUN;
  }
}
#endif

void sync_midi_track_phase(MidiSeqTrack &track, uint32_t div192) {
  track.reset();
  const uint16_t ticks_per_step = track.ticks_per_step();
  const uint16_t length = track.length ? track.length : 1;
  if (ticks_per_step == 0) {
    return;
  }

  const uint32_t total_steps = div192 / ticks_per_step;
  track.step_count = (uint8_t)(total_steps % length);
  track.tick_counter = (uint16_t)(div192 % ticks_per_step);
  const uint8_t legacy_tps = SeqTrack::get_speed_multiplier_int(track.speed);
  track.mod12_counter = (uint8_t)(((uint32_t)track.tick_counter * legacy_tps) /
                                  ticks_per_step);
  track.cur_event_idx = track.seq_data.locate_start(track.step_count);
  if (total_steps < length) {
    track.conditional_flags |= SeqTrackCond::CONDITIONAL_FIRST_RUN;
  } else {
    track.conditional_flags &= ~SeqTrackCond::CONDITIONAL_FIRST_RUN;
  }
}

void sync_spsx_track_phase(SPSXSeqTrack &track, uint32_t div192) {
  track.reset();
  const uint16_t ticks_per_step = track.get_ticks_per_step();
  const uint16_t length = track.length ? track.length : 1;
  if (ticks_per_step == 0) {
    return;
  }

  const uint32_t total_steps = div192 / ticks_per_step;
  track.step_count = (uint8_t)(total_steps % length);
  track.tick_counter = (uint16_t)(div192 % ticks_per_step);
  track.update_legacy_progress_counter(ticks_per_step);
  track.cur_event_idx = track.get_lockidx(track.step_count);
  track.set_first_run(total_steps < length);
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

  if (mixer->is_mute_param(track_param)) {
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

#if defined(PLATFORM_TBD)
bool seq_grid_x_runs_tbd_tracks() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}
#endif

#if !defined(__AVR__)
bool seq_grid_y_runs_midi_tracks() {
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_OFF) {
    return false;
  }
#if defined(PLATFORM_TBD)
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    return true;
  }
#endif
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    return true;
  }
  MidiDevice *secondary = device_manager.secondary_device();
  return secondary != nullptr &&
         (secondary->id == DEVICE_A4 || secondary->id == DEVICE_MNM);
}
#endif

bool seq_grid_y_runs_legacy_ext_tracks() {
#if defined(__AVR__)
  return true;
#else
  return mcl_cfg.grid_y_device != GRID_Y_DEVICE_OFF &&
         !seq_grid_y_runs_midi_tracks();
#endif
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

void reset_slide_track_locks(SeqSlideTrack &track) NOINLINE();
void reset_slide_track_locks(SeqSlideTrack &track) {
  track.locks_slides_recalc = 255;
  for (auto &sd : track.locks_slide_data) {
    sd.init();
  }
}

#if !defined(__AVR__)
// SPSXSeqTrack derives from StepSeqSlideTrack, not SeqSlideTrack, so the
// SeqSlideTrack& overload above doesn't accept it. Both bases expose the same
// {locks_slides_recalc, locks_slide_data[].init()} surface, so the body is
// identical — this overload exists purely so for_each_md_track's SPSX-branch
// lambda instantiation type-checks. AVR has no SPSX engine so it stays at one
// function and the dedup commit's flash savings are preserved there.
void reset_slide_track_locks(StepSeqSlideTrack &track) NOINLINE();
void reset_slide_track_locks(StepSeqSlideTrack &track) {
  track.locks_slides_recalc = 255;
  for (auto &sd : track.locks_slide_data) {
    sd.init();
  }
}
#endif

void cleanup_mcl_seq_all_midi(MCLSeqMidiEvents *events) {
  cleanup_mcl_seq_midi(events, &Midi);
  cleanup_mcl_seq_midi(events, &Midi2);
  cleanup_mcl_seq_midi(events, &MidiUSB);
#ifdef PLATFORM_TBD
  cleanup_mcl_seq_midi(events, &MidiP4);
#endif
}

inline __attribute__((always_inline)) void prepare_tx_buffers(MidiUartClass *primary_output,
                                       MidiUartClass *secondary_output,
                                       bool shared_output,
                                       bool engage_sidechannel,
                                       bool &uart_sidechannel,
                                       MidiUartClass *&uart,
                                       MidiUartClass *&uart2) {
  primary_output->disable_tx_irq();
  secondary_output->disable_tx_irq();

  MidiUartClass *primary_active = uart_sidechannel ? &seq_tx2 : &seq_tx1;
  MidiUartClass *secondary_active = uart_sidechannel ? &seq_tx4 : &seq_tx3;
  MidiUartClass *primary_side = uart_sidechannel ? &seq_tx1 : &seq_tx2;
  MidiUartClass *secondary_side = uart_sidechannel ? &seq_tx3 : &seq_tx4;

  uart = primary_active;
  uart2 = shared_output ? primary_active : secondary_active;

  // If the side channel ring buffer is not empty, it means it did not finish
  // transmitting before the next seq() call. Drain it through the new buffer.
  if (engage_sidechannel) {
    primary_output->txRb_sidechannel = primary_side->txRb;
    if (shared_output) {
      seq_tx3.txRb->init();
      seq_tx4.txRb->init();
    } else {
      secondary_output->txRb_sidechannel = secondary_side->txRb;
    }
  } else {
    // Purge stale buffers (from MIDI CONTINUE).
    primary_active->txRb->init();
    secondary_active->txRb->init();
  }
#if defined(__AVR__)
  primary_output->enable_tx_irq();
  secondary_output->enable_tx_irq();
#else
  // Have to flush the first byte to re-trigger the uart tx isr.
  LOCK();
  primary_output->tx_flush();
  secondary_output->tx_flush();
  CLEAR_LOCK();
#endif
  uart_sidechannel = !uart_sidechannel;
}

inline __attribute__((always_inline)) void run_md_tick(MCLSeq &self, MidiUartClass *uart,
                                MidiUartClass *uart2, bool legacy_tick) {
  if (!seq_grid_x_runs_md_tracks()) return;
#if !defined(__AVR__)
  if (self.using_spsx_tracks) {
    self.spsx_tracks[0].pre_seq(uart);

    for (uint8_t i = 0; i < self.num_md_tracks; i++) {
      self.spsx_tracks[i].seq(uart, uart2);
    }

    self.spsx_tracks[0].post_seq(uart);

    if (legacy_tick) {
      for (uint8_t i = 0; i < self.num_md_tracks; i++) {
        self.md_arp_tracks[i].mute_state = self.spsx_tracks[i].mute_state;
        self.md_arp_tracks[i].seq(uart, uart2);
      }

      self.mdfx_track.seq();

      for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
        self.aux_tracks[i].seq();
      }

#ifdef LFO_TRACKS
      for (uint8_t i = 0; i < self.num_md_tracks; i++) {
        self.grid_x_lfo_tracks[i].seq(uart, uart2);
      }
      self.clear_track_trigs(DeviceIdx::Primary);
#endif

      self.perf_track.seq(uart, uart2);
    }

    // SPSX full-velocity trigs share the legacy parallel trigger mask.
    // Lower-velocity SPSX trigs are sent inline because parallelTrig has no
    // velocity lane.
    if (MDSeqTrack::md_trig_mask > 0) {
      MD.parallelTrig(MDSeqTrack::md_trig_mask, uart);
    }

    for (uint8_t i = 0; i < self.num_md_tracks; i++) {
      self.spsx_tracks[i].recalc_slides();
    }
    return;
  }
#endif
  if (!legacy_tick) return;

  MDSeqTrack::pre_seq(uart);

  for (uint8_t i = 0; i < self.num_md_tracks; i++) {
    self.md_tracks[i].seq(uart, uart2);
    self.md_arp_tracks[i].mute_state = self.md_tracks[i].mute_state;
    self.md_arp_tracks[i].seq(uart, uart2);
  }

  self.mdfx_track.seq();

  MDSeqTrack::post_seq(uart);

  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    self.aux_tracks[i].seq();
  }

#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < self.num_md_tracks; i++) {
    self.grid_x_lfo_tracks[i].seq(uart, uart2);
  }
  self.clear_track_trigs(DeviceIdx::Primary);
#endif

  self.perf_track.seq(uart, uart2);

  if (MDSeqTrack::md_trig_mask > 0) {
    MD.parallelTrig(MDSeqTrack::md_trig_mask, uart);
  }
}

#if defined(PLATFORM_TBD)
inline __attribute__((always_inline)) void run_tbd_tick(MCLSeq &self, MidiUartClass *uart,
                                 MidiUartClass *uart2, bool legacy_tick) {
  if (!seq_grid_x_runs_tbd_tracks()) return;
  for (uint8_t i = 0; i < self.num_tbd_tracks; i++) {
    self.tbd_tracks[i].seq(uart);
  }
  if (legacy_tick) {
    for (uint8_t i = 0; i < self.num_tbd_tracks; i++) {
      self.md_arp_tracks[i].mute_state = self.tbd_tracks[i].mute_state;
      self.md_arp_tracks[i].seq(uart, uart2);
      self.grid_x_lfo_tracks[i].seq(uart, uart2);
    }
    self.clear_track_trigs(DeviceIdx::Primary);
  }
}
#endif

#if !defined(__AVR__)
inline __attribute__((always_inline)) void run_midi_tick(MCLSeq &self, MidiUartClass *uart,
                                  MidiUartClass *uart2, bool legacy_tick) {
  if (!seq_grid_y_runs_midi_tracks()) return;
  for (uint8_t i = 0; i < self.num_midi_tracks; i++) {
    self.midi_tracks[i].seq(uart2);
  }
  if (legacy_tick) {
    for (uint8_t i = 0; i < self.num_midi_tracks; i++) {
      self.ext_arp_tracks[i].mute_state = self.midi_tracks[i].mute_state;
      self.ext_arp_tracks[i].seq(uart, uart2);
      self.grid_y_lfo_tracks[i].seq(uart, uart2);
    }
    self.clear_track_trigs(DeviceIdx::Secondary);
  }
}
#endif

#if defined(EXT_TRACKS)
inline __attribute__((always_inline)) void run_legacy_ext_tick(MCLSeq &self, MidiUartClass *uart,
                                        MidiUartClass *uart2) {
  for (uint8_t i = 0; i < self.num_ext_tracks; i++) {
    self.ext_tracks[i].seq(uart2);
    self.ext_arp_tracks[i].mute_state = self.ext_tracks[i].mute_state;
    self.ext_arp_tracks[i].seq(uart, uart2);
    self.grid_y_lfo_tracks[i].seq(uart, uart2);
  }
  self.clear_track_trigs(DeviceIdx::Secondary);
}
#endif

// Ordering of recalc passes is preserved verbatim from the pre-refactor seq():
// legacy MD, legacy EXT, TBD, MIDI. Do not reorder without proving no engine
// observes the order.
inline __attribute__((always_inline)) void recalc_all_slides(MCLSeq &self, bool legacy_tick) {
  (void)legacy_tick;
  if (seq_grid_x_runs_md_tracks()) {
#if !defined(__AVR__)
    if (!self.using_spsx_tracks && legacy_tick)
#endif
    {
      for (uint8_t i = 0; i < self.num_md_tracks; i++) {
        self.md_tracks[i].recalc_slides();
      }
    }
  }
#if defined(EXT_TRACKS)
  if (seq_grid_y_runs_legacy_ext_tracks() && legacy_tick) {
    for (uint8_t i = 0; i < self.num_ext_tracks; i++) {
      self.ext_tracks[i].recalc_slides();
    }
  }
#endif
#if defined(PLATFORM_TBD)
  if (seq_grid_x_runs_tbd_tracks()) {
    for (uint8_t i = 0; i < self.num_tbd_tracks; i++) {
      self.tbd_tracks[i].recalc_slides();
    }
  }
#endif
#if !defined(__AVR__)
  if (seq_grid_y_runs_midi_tracks()) {
    for (uint8_t i = 0; i < self.num_midi_tracks; i++) {
      self.midi_tracks[i].recalc_slides();
    }
  }
#endif
}

} // namespace

void MCLSeq::set_outputs(MidiUartClass *primary_output_,
                         MidiUartClass *secondary_output_) {
  primary_output = primary_output_;
  secondary_output = secondary_output_;
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].uart = primary_output;
    md_tracks[i].uart2 = secondary_output;
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].uart = secondary_output;
    ext_tracks[i].uart2 = primary_output;
  }
#endif
#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_tbd_tracks; i++) {
    tbd_tracks[i].uart = primary_output;
    tbd_tracks[i].uart2 = secondary_output;
  }
#endif
#if !defined(__AVR__)
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].uart = secondary_output;
    midi_tracks[i].uart2 = primary_output;
  }
#endif
}

void MCLSeq::setup() {
  configure_clock_interpolation();
  TrackLoadFadeRunner::clear();
#if !defined(__AVR__)
  sps_host_arr_bridge.setup();  // register SPS host arranger cell listener
  sps_host_seq_bridge.setup();  // register SPS host sequencer-control listener
#endif

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
    md_tracks[i].set_length(16);
    md_tracks[i].speed = SEQ_SPEED_1X;
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
    md_arp_tracks[i].init();
    md_arp_tracks[i].track_number = i;
  }
#ifdef LFO_TRACKS

  for (uint8_t i = 0; i < num_grid_x_lfo_tracks; i++) {
    grid_x_lfo_tracks[i].init();
    grid_x_lfo_tracks[i].track_number = i;
    grid_x_lfo_tracks[i].device_idx = DeviceIdx::Primary;
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_grid_y_lfo_tracks; i++) {
    grid_y_lfo_tracks[i].init();
    grid_y_lfo_tracks[i].track_number = i;
    grid_y_lfo_tracks[i].device_idx = DeviceIdx::Secondary;
  }
#endif
#endif
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].set_channel(i);
    ext_tracks[i].set_length(16);
    ext_tracks[i].speed = SEQ_SPEED_1X;
    ext_tracks[i].clear();
    ext_tracks[i].init_notes_on();
    ext_tracks[i].track_number = i;
    ext_arp_tracks[i].init();
    ext_arp_tracks[i].track_number = i;
  }
#endif
#if !defined(__AVR__)
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    spsx_tracks[i].track_number = i;
    spsx_tracks[i].length = 16;
    spsx_tracks[i].speed = SPSX_SPEED_1X;
    spsx_tracks[i].mute_state = SPSX_MUTE_OFF;
    spsx_tracks[i].seq_class = this;
  }
#endif
#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_tbd_tracks; i++) {
    tbd_tracks[i].track_number = i;
    tbd_tracks[i].length = 16;
    tbd_tracks[i].speed = STEPSEQ_SPEED_1X;
    tbd_tracks[i].mute_state = STEPSEQ_MUTE_OFF;
    tbd_tracks[i].seq_class = this;
    tbd_set_step_sound_default(tbd_tracks[i].p4_sound, i);
  }
#endif
#if !defined(__AVR__)
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].seq_data.clear();
#if defined(PLATFORM_TBD)
    TbdP4SoundData *sound = tbd_midi_runtime_sound(i);
    if (sound != nullptr) {
      tbd_set_midi_sound_default(*sound, i);
      midi_tracks[i].set_channel(sound->midi_channel);
    }
#else
    midi_tracks[i].set_channel(i);
#endif
    midi_tracks[i].set_length(16);
    midi_tracks[i].set_speed(SEQ_SPEED_1X);
    midi_tracks[i].mute_state = SEQ_MUTE_OFF;
    midi_tracks[i].init_notes_on();
    midi_tracks[i].track_number = i;
  }
#endif
  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    aux_tracks[i].length = 16;
    aux_tracks[i].speed = SEQ_SPEED_1X;
  }

  mdfx_track.length = 16;
  mdfx_track.speed = SEQ_SPEED_1X;

  perf_track.length = 16;
  perf_track.speed = SEQ_SPEED_1X;

  enable();

  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStopCallback);
  //  MidiClock.addOnMidiStartCallback(
  //      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStartCallback);
  MidiClock.addOnMidiStartImmediateCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStartImmediateCallback);

  MidiClock.addOnMidiContinueCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiContinueCallback);
  midi_events.setup_callbacks();
};

// restore kit params
void MCLSeq::update_kit_params() {
#ifdef LFO_TRACKS
#endif
}
void MCLSeq::update_params() {
#ifdef LFO_TRACKS
#endif
}

void MCLSeq::configure_clock_interpolation() {
#if !defined(__AVR__)
  uint8_t interpolation = LEGACY_SEQ_INTERPOLATION;
#if defined(PLATFORM_TBD)
  // RP2040/TBD can host legacy and high-resolution engines together. Keep the
  // global scheduler at the highest engine resolution and gate legacy engines
  // in seq() so they still observe their historical 48 PPQN cadence.
  interpolation = STEPSEQ_SEQ_INTERPOLATION;
#else
  interpolation = seq_grid_y_runs_midi_tracks()
                      ? STEPSEQ_SEQ_INTERPOLATION
                      : using_spsx_tracks ? SPSX_SEQ_INTERPOLATION
                                           : LEGACY_SEQ_INTERPOLATION;
#endif
  MidiClock.set_clock_interpolation(interpolation);
  legacy_tick_counter = 0;
#endif
}

bool MCLSeq::legacy_tick_due() {
#if !defined(__AVR__)
  if (MidiClock.clock_interpolation <= LEGACY_SEQ_INTERPOLATION) {
    legacy_tick_counter = 0;
    return true;
  }

  uint8_t divider = MidiClock.clock_interpolation / LEGACY_SEQ_INTERPOLATION;
  if (divider <= 1) {
    legacy_tick_counter = 0;
    return true;
  }

  bool due = legacy_tick_counter == 0;
  legacy_tick_counter++;
  if (legacy_tick_counter >= divider) {
    legacy_tick_counter = 0;
  }
  return due;
#else
  return true;
#endif
}

void seq_rec_play() {
  if (key_interface.is_key_down(MDX_KEY_REC)) {
    // key_interface.ignoreNextEvent(MDX_KEY_REC);
    seq_step_page.bootstrap_record();
    reset_undo();
  }
}

uint8_t MCLSeq::find_ext_track(uint8_t channel) {
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
#if !defined(__AVR__)
    if (seq_grid_y_runs_midi_tracks()) {
      if (midi_tracks[n].channel() == channel) {
        return n;
      }
    } else {
      if (ext_tracks[n].channel == channel) {
        return n;
      }
    }
#else
    if (ext_tracks[n].channel == channel) {
      return n;
    }
#endif
  }
  return 255;
}

void MCLSeq::onMidiContinueCallback(uint32_t clock_count) {
  (void)clock_count;
  update_params();
  seq_rec_play();
  SET_BIT16(MDSeqTrack::gui_update, last_primary_track); //force cursor resync
}

void MCLSeq::onMidiStartImmediateCallback(uint32_t clock_count) {
  (void)clock_count;
  realtime = true;
#if !defined(__AVR__)
  legacy_tick_counter = 0;
#endif
  seq_tx1.txRb->init();
  seq_tx2.txRb->init();
  seq_tx3.txRb->init();
  seq_tx4.txRb->init();

  SeqTrackUtil::for_each_md_track([](auto &track, uint8_t) { track.reset(); });
  neighbor_trig_mask = 0;
  set_fill(false);
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_arp_tracks[i].reset();
  }

  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    // ext_tracks[i].start_clock32th = 0;
    ext_tracks[i].reset();
    ext_arp_tracks[i].reset();
  }
#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_tbd_tracks; i++) {
    tbd_tracks[i].reset();
  }
#endif
#if !defined(__AVR__)
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].reset();
  }
#endif

  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    aux_tracks[i].reset();
  }
  mdfx_track.reset();
  perf_track.reset();
#ifdef LFO_TRACKS
  clear_track_trigs(DeviceIdx::Primary);
  clear_track_trigs(DeviceIdx::Secondary);
  for (uint8_t i = 0; i < num_grid_x_lfo_tracks; i++) {
    grid_x_lfo_tracks[i].reset_runtime();
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_grid_y_lfo_tracks; i++) {
    grid_y_lfo_tracks[i].reset_runtime();
  }
#endif
#endif

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;

  sei();

#ifdef LFO_TRACKS
#endif
  seq_rec_play();

  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
  if (SeqPage::recording) {
    oled_display.textbox_P(mclstr_rec);
  }
}

#if !defined(__AVR__)
void MCLSeq::set_transport_position(uint32_t host_tick96) {
  const uint32_t div192 = MidiClock.host_transport_tick96_to_div192(host_tick96);
  uint32_t divider = MidiClock.clock_interpolation / LEGACY_SEQ_INTERPOLATION;
  if (divider == 0) {
    divider = 1;
  }
  legacy_tick_counter = (uint8_t)(div192 % divider);
  MDSeqTrack::md_trig_mask = 0;
  MDSeqTrack::load_machine_cache = 0;
  clear_track_trigs(DeviceIdx::Primary);
  clear_track_trigs(DeviceIdx::Secondary);
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_grid_x_lfo_tracks; i++) {
    grid_x_lfo_tracks[i].reset_runtime();
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_grid_y_lfo_tracks; i++) {
    grid_y_lfo_tracks[i].reset_runtime();
  }
#endif
#endif

  const uint32_t legacy_ticks = legacy_tick_count_from_div192(div192);

  neighbor_trig_mask = 0;
  set_fill(false);
  if (using_spsx_tracks) {
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      sync_spsx_track_phase(spsx_tracks[i], div192);
    }
  } else {
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      sync_md_track_phase(md_tracks[i], legacy_ticks);
    }
  }

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    sync_seq_track_phase(md_arp_tracks[i], legacy_ticks);
  }

#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    sync_ext_track_phase(ext_tracks[i], legacy_ticks);
    sync_seq_track_phase(ext_arp_tracks[i], legacy_ticks);
  }
#endif

  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    sync_midi_track_phase(midi_tracks[i], div192);
  }

  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    sync_seq_track_phase(aux_tracks[i], legacy_ticks);
  }
  sync_seq_track_phase(mdfx_track, legacy_ticks);
  sync_seq_track_phase(perf_track, legacy_ticks);
}
#endif

void MCLSeq::onMidiStartCallback(uint32_t clock_count) {
  (void)clock_count;
}

void MCLSeq::onMidiStopCallback(uint32_t clock_count) {
  (void)clock_count;
#ifdef EXT_TRACKS
  if (seq_grid_y_runs_legacy_ext_tracks()) {
    for (uint8_t i = 0; i < num_ext_tracks; i++) {
      ext_tracks[i].buffer_notesoff();
      ext_tracks[i].reset_params();
      reset_slide_track_locks(ext_tracks[i]);
    }
  }
#endif
  if (seq_grid_x_runs_md_tracks()) {
    MD.reset_dsp_params();

    SeqTrackUtil::for_each_md_track([](auto &track, uint8_t) {
      track.reset_params();
      track.send_notes_off();
      reset_slide_track_locks(track);
    });
  }
#if defined(PLATFORM_TBD)
  if (seq_grid_x_runs_tbd_tracks()) {
    for (uint8_t i = 0; i < num_tbd_tracks; i++) {
      tbd_tracks[i].send_notes_off();
      tbd_tracks[i].locks_slides_recalc = 255;
      for (auto &sd : tbd_tracks[i].locks_slide_data) { sd.init(); }
    }
  }
#endif
#if !defined(__AVR__)
  if (seq_grid_y_runs_midi_tracks()) {
    for (uint8_t i = 0; i < num_midi_tracks; i++) {
      midi_tracks[i].buffer_notesoff();
      midi_tracks[i].reset_params();
      midi_tracks[i].locks_slides_recalc = 255;
      for (auto &sd : midi_tracks[i].locks_slide_data) { sd.init(); }
    }
  }
#endif
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_grid_x_lfo_tracks; i++) {
    grid_x_lfo_tracks[i].reset_params();
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_grid_y_lfo_tracks; i++) {
    grid_y_lfo_tracks[i].reset_params();
  }
#endif
#endif
}

void MCLSeq::seq() {
  if (!state) { return; }

  const bool legacy_tick = legacy_tick_due();
  const bool shared_output = primary_output == secondary_output;
  bool engage_sidechannel = true;
  MidiUartClass *uart;
  MidiUartClass *uart2;

  // Realtime first pass writes direct to outputs for low latency, then loops
  // with realtime=false and engage_sidechannel=false so the second pass drains
  // engines through swapped tx buffers and purges any stale MIDI CONTINUE
  // bytes instead of streaming them. Non-realtime is a single pass.
  for (;;) {
    if (!realtime) {
      prepare_tx_buffers(primary_output, secondary_output, shared_output,
                         engage_sidechannel, uart_sidechannel, uart, uart2);
    } else {
      uart = primary_output;
      uart2 = secondary_output;
    }

    run_md_tick(*this, uart, uart2, legacy_tick);
#if defined(PLATFORM_TBD)
    run_tbd_tick(*this, uart, uart2, legacy_tick);
#endif
#if !defined(__AVR__)
    run_midi_tick(*this, uart, uart2, legacy_tick);
#endif
#if defined(EXT_TRACKS)
    if (seq_grid_y_runs_legacy_ext_tracks() && legacy_tick) {
      run_legacy_ext_tick(*this, uart, uart2);
    }
#endif
    recalc_all_slides(*this, legacy_tick);

    if (!realtime) break;
    realtime = false;
    engage_sidechannel = false;
  }
  TrackLoadFadeRunner::tick(uart, uart2);
}

#if !defined(__AVR__)
// Mode switch is only safe with the transport stopped: seq() runs in ISR
// context and the switch rebinds spsx_tracks state + clock_interpolation.
// Callers (currently MD::init_grid_devices on probe) must ensure transport
// is paused. Returns true on success, false if the switch was rejected.
bool MCLSeq::switch_to_spsx() {
  if (MidiClock.state != MidiClockClass::PAUSED) {
    DEBUG_PRINTLN(F("switch_to_spsx: refused, transport not PAUSED"));
    return false;
  }
  MidiClock.clock_interpolation = SPSX_SEQ_INTERPOLATION;
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    spsx_tracks[i].track_number = i;
    spsx_tracks[i].seq_class = this;
    spsx_tracks[i].reset();
  }
  using_spsx_tracks = true;
  configure_clock_interpolation();
  neighbor_trig_mask = 0;
  set_fill(false);
  return true;
}

bool MCLSeq::switch_to_legacy() {
  if (MidiClock.state != MidiClockClass::PAUSED) {
    DEBUG_PRINTLN(F("switch_to_legacy: refused, transport not PAUSED"));
    return false;
  }
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    spsx_tracks[i].send_notes_off();
  }
  // Drop any trig bits SPSX accumulated; legacy pre_seq won't run until next
  // tick and we don't want a parallelTrig drain seeing stale SPSX state.
  MDSeqTrack::md_trig_mask = 0;
  set_fill(false);
  neighbor_trig_mask = 0;
  using_spsx_tracks = false;
  configure_clock_interpolation();
  return true;
}
#endif

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
    mcl_seq.set_fill_track(DeviceIdx::Primary,
                           (control_ch << 2) + fill_track, value);
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

MCLSeq mcl_seq;
