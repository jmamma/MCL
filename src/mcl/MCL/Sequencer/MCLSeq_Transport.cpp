#include "MCLSeq_Internal.h"

#include "KeyInterface.h"
#include "MCLGUI.h"
#include "MCLStrings.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"
#include "global.h"
#include "../../Drivers/Generic/Sequencer/StepSeqDefines.h"
#include "../../Drivers/MD/MD.h"

namespace {

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

void seq_rec_play() {
  if (key_interface.is_key_down(MDX_KEY_REC)) {
    // key_interface.ignoreNextEvent(MDX_KEY_REC);
    seq_step_page.bootstrap_record();
    reset_undo();
  }
}

} // namespace

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
