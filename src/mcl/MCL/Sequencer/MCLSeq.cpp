#include "MCLSeq_Internal.h"

#include "Sequencer/PtcVoiceRouter.h"
#include "Sequencer/SeqTrackUtil.h"
#if !defined(__AVR__)
#include "Host/SpsHostArrBridge.h"  // SPS<->MCL arranger cell listener
#include "Host/SpsHostSeqBridge.h"  // SPS<->MCL seq control listener
#endif
#include "global.h"
#include "../../Drivers/Generic/Sequencer/TrackLoadFadeRunner.h"
#if !defined(__AVR__)
#include "../../Drivers/Generic/Sequencer/StepSeqDefines.h"
#endif
#if defined(PLATFORM_TBD)
#include "../../Drivers/TBD/TBDTrack.h"
#endif
#include "../../Drivers/MD/MD.h"

namespace {

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
  spsx_accent_amount = 0;
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

#if !defined(__AVR__)
void MCLSeq::set_spsx_accent_amount(uint8_t amount, bool notify_host) {
  if (amount > 15) amount = 15;
  if (spsx_accent_amount == amount) return;
  spsx_accent_amount = amount;
  if (notify_host)
    sps_host_seq_bridge.notifyDirty(0xFF, spsseq::DIRTY_META);
}
#endif

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

uint8_t MCLSeq::find_ext_track(uint8_t channel) {
  if (ptc_route_channel_is_primary(channel)) {
    return 255;
  }
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

void MCLSeq::seq() {
  if (!state) { return; }

  const bool legacy_tick = legacy_tick_due();
  const bool shared_output = primary_output == secondary_output;
  bool engage_sidechannel = true;
  MidiUartClass *uart;
  MidiUartClass *uart2;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  bool load_fade_ticked = false;
#endif

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

#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
    if (!load_fade_ticked) {
      TrackLoadFadeRunner::tick(uart, uart2);
      load_fade_ticked = true;
    }
#endif

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
#if !MCL_FEATURE_HOST_LOAD_FADE_SEEK
  TrackLoadFadeRunner::tick(uart, uart2);
#endif
}

MCLSeq mcl_seq;
