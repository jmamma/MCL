#include "DiagnosticPage.h"
#include "MCLSeq.h"
#include "SeqTrackUtil.h"
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
#if defined(PLATFORM_TBD)
#include "../Drivers/Generic/Sequencer/StepSeqDefines.h"
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

#if defined(PLATFORM_TBD)
bool seq_grid_x_runs_tbd_tracks() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}

bool seq_grid_y_runs_tbd_midi_tracks() {
  return mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD;
}
#endif

bool seq_grid_y_runs_legacy_ext_tracks() {
#ifdef PLATFORM_TBD
  return mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER ||
         mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT;
#else
  return true;
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

void cleanup_mcl_seq_all_midi(MCLSeqMidiEvents *events) {
  cleanup_mcl_seq_midi(events, &Midi);
  cleanup_mcl_seq_midi(events, &Midi2);
  cleanup_mcl_seq_midi(events, &MidiUSB);
#ifdef PLATFORM_TBD
  cleanup_mcl_seq_midi(events, &MidiP4);
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
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].uart = secondary_output;
    midi_tracks[i].uart2 = primary_output;
  }
#endif
}

void MCLSeq::setup() {
  configure_clock_interpolation();

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
    md_tracks[i].set_length(16);
    md_tracks[i].speed = SEQ_SPEED_1X;
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
    md_arp_tracks[i].track_number = i;
  }
#ifdef LFO_TRACKS

  for (uint8_t i = 0; i < num_grid_x_lfo_tracks; i++) {
    grid_x_lfo_tracks[i].init();
    grid_x_lfo_tracks[i].track_number = i;
    grid_x_lfo_tracks[i].device_slot = 1;
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_grid_y_lfo_tracks; i++) {
    grid_y_lfo_tracks[i].init();
    grid_y_lfo_tracks[i].track_number = i;
    grid_y_lfo_tracks[i].device_slot = 2;
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
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].seq_data.clear();
    tbd_set_midi_sound_default(midi_tracks[i].p4_sound, i);
    midi_tracks[i].set_channel(midi_tracks[i].p4_sound.midi_channel);
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
#if defined(PLATFORM_TBD)
  // RP2040/TBD can host legacy and high-resolution engines together. Keep the
  // global scheduler at the highest engine resolution and gate legacy engines
  // in seq() so they still observe their historical 48 PPQN cadence.
  MidiClock.clock_interpolation = STEPSEQ_SEQ_INTERPOLATION;
#else
  MidiClock.clock_interpolation =
      using_spsx_tracks ? SPSX_SEQ_INTERPOLATION : LEGACY_SEQ_INTERPOLATION;
#endif
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
#if defined(PLATFORM_TBD)
    if (seq_grid_y_runs_tbd_midi_tracks()) {
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

void MCLSeq::onMidiContinueCallback() {
  update_params();
  seq_rec_play();
  SET_BIT16(MDSeqTrack::gui_update, last_md_track); //force cursor resync
}

void MCLSeq::onMidiStartImmediateCallback() {
  realtime = true;
#if !defined(__AVR__)
  legacy_tick_counter = 0;
#endif
  seq_tx1.txRb->init();
  seq_tx2.txRb->init();
  seq_tx3.txRb->init();
  seq_tx4.txRb->init();

  SeqTrackUtil::for_each_md_track([](auto &track, uint8_t) { track.reset(); });
#if !defined(__AVR__)
  if (using_spsx_tracks) {
    neighbor_trig_mask = 0;
    fill_mask = 0;
  } else
#endif
  {
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_arp_tracks[i].reset();
    }
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
  clear_lfo_track_trigs(1);
  clear_lfo_track_trigs(2);
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

void MCLSeq::onMidiStartCallback() {}

void MCLSeq::onMidiStopCallback() {
#ifdef EXT_TRACKS
  if (seq_grid_y_runs_legacy_ext_tracks()) {
    for (uint8_t i = 0; i < num_ext_tracks; i++) {
      ext_tracks[i].buffer_notesoff();
      ext_tracks[i].reset_params();
      ext_tracks[i].locks_slides_recalc = 255;
      for (uint8_t c = 0; c < NUM_LOCKS; c++) {
        ext_tracks[i].locks_slide_data[c].init();
      }
    }
  }
#endif
  if (seq_grid_x_runs_md_tracks()) {
    MD.reset_dsp_params();

    SeqTrackUtil::for_each_md_track([](auto &track, uint8_t) {
      track.reset_params();
      track.send_notes_off();
      track.locks_slides_recalc = 255;
      for (auto &sd : track.locks_slide_data) { sd.init(); }
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
  if (seq_grid_y_runs_tbd_midi_tracks()) {
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
#ifdef LFO_TRACKS
  const bool lfo_send_due = legacy_tick;
#endif

  MidiUartClass *uart;
  MidiUartClass *uart2;
  bool engage_sidechannel = true;
  const bool shared_output = primary_output == secondary_output;

  // If realtime, render the first tick immediately and defer later ticks.

  if (!realtime) {
  again:

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
    // clearLed2();
#if defined(__AVR__)
    primary_output->enable_tx_irq();
    secondary_output->enable_tx_irq();
#else
    //Have to flush the first byte to re-trigger the uart tx isr.
    LOCK();
    primary_output->tx_flush();
    secondary_output->tx_flush();
    CLEAR_LOCK();
#endif
    // Flip uart / side_channel buffer for next run
    uart_sidechannel = !uart_sidechannel;
  } else {
    uart = primary_output;
    uart2 = secondary_output;
  }
  //  Stopwatch sw;

  if (seq_grid_x_runs_md_tracks()) {
#if !defined(__AVR__)
  if (using_spsx_tracks) {
    spsx_tracks[0].pre_seq(uart);

    for (uint8_t i = 0; i < num_md_tracks; i++) {
      spsx_tracks[i].seq(uart, uart2);
    }

    spsx_tracks[0].post_seq(uart);

    if (legacy_tick) {
      for (uint8_t i = 0; i < num_md_tracks; i++) {
        md_arp_tracks[i].mute_state = spsx_tracks[i].mute_state;
        md_arp_tracks[i].seq(uart, uart2);
      }

      mdfx_track.seq();

      for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
        aux_tracks[i].seq();
      }

#ifdef LFO_TRACKS
      for (uint8_t i = 0; i < num_md_tracks; i++) {
        grid_x_lfo_tracks[i].seq(uart, uart2, lfo_send_due);
      }
      clear_lfo_track_trigs(1);
#endif

      perf_track.seq(uart, uart2);
    }

    // SPSX full-velocity trigs share the legacy parallel trigger mask.
    // Lower-velocity SPSX trigs are sent inline because parallelTrig has no
    // velocity lane.
    if (MDSeqTrack::md_trig_mask > 0) {
      MD.parallelTrig(MDSeqTrack::md_trig_mask, uart);
    }

    for (uint8_t i = 0; i < num_md_tracks; i++) {
      spsx_tracks[i].recalc_slides();
    }
  } else {
#endif
  if (legacy_tick) {
    MDSeqTrack::pre_seq(uart);

    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_tracks[i].seq(uart,uart2);
      md_arp_tracks[i].mute_state = md_tracks[i].mute_state;
      md_arp_tracks[i].seq(uart,uart2);
    }

    mdfx_track.seq();

    MDSeqTrack::post_seq(uart);

    for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
      aux_tracks[i].seq();
    }

#ifdef LFO_TRACKS
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      grid_x_lfo_tracks[i].seq(uart, uart2, lfo_send_due);
    }
    clear_lfo_track_trigs(1);
#endif

    perf_track.seq(uart, uart2);

    if (MDSeqTrack::md_trig_mask > 0) {
      MD.parallelTrig(MDSeqTrack::md_trig_mask, uart);
    }
  }
#if !defined(__AVR__)
  }
#endif
  }

#if defined(PLATFORM_TBD)
  if (seq_grid_x_runs_tbd_tracks()) {
    for (uint8_t i = 0; i < num_tbd_tracks; i++) {
      tbd_tracks[i].seq(uart);
    }
    if (legacy_tick) {
      for (uint8_t i = 0; i < num_tbd_tracks; i++) {
        md_arp_tracks[i].mute_state = tbd_tracks[i].mute_state;
        md_arp_tracks[i].seq(uart, uart2);
        grid_x_lfo_tracks[i].seq(uart, uart2, lfo_send_due);
      }
      clear_lfo_track_trigs(1);
    }
  }
  if (seq_grid_y_runs_tbd_midi_tracks()) {
    for (uint8_t i = 0; i < num_midi_tracks; i++) {
      midi_tracks[i].seq(uart2);
    }
    if (legacy_tick) {
      for (uint8_t i = 0; i < num_midi_tracks; i++) {
        ext_arp_tracks[i].mute_state = midi_tracks[i].mute_state;
        ext_arp_tracks[i].seq(uart, uart2);
        grid_y_lfo_tracks[i].seq(uart, uart2, lfo_send_due);
      }
      clear_lfo_track_trigs(2);
    }
  }
#endif

#if defined(EXT_TRACKS)
  if (seq_grid_y_runs_legacy_ext_tracks() && legacy_tick) {
    for (uint8_t i = 0; i < num_ext_tracks; i++) {
      ext_tracks[i].seq(uart2);
      ext_arp_tracks[i].mute_state = ext_tracks[i].mute_state;
      ext_arp_tracks[i].seq(uart,uart2);
      grid_y_lfo_tracks[i].seq(uart, uart2, lfo_send_due);
    }
    clear_lfo_track_trigs(2);
  }
#endif

  if (seq_grid_x_runs_md_tracks()) {
#if !defined(__AVR__)
  if (!using_spsx_tracks && legacy_tick) {
#endif
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_tracks[i].recalc_slides();
    }
#if !defined(__AVR__)
  }
#endif
  }

#if defined(EXT_TRACKS)
  if (seq_grid_y_runs_legacy_ext_tracks() && legacy_tick) {
    for (uint8_t i = 0; i < num_ext_tracks; i++) {
      ext_tracks[i].recalc_slides();
    }
  }
#endif
#if defined(PLATFORM_TBD)
  if (seq_grid_x_runs_tbd_tracks()) {
    for (uint8_t i = 0; i < num_tbd_tracks; i++) {
      tbd_tracks[i].recalc_slides();
    }
  }
  if (seq_grid_y_runs_tbd_midi_tracks()) {
    for (uint8_t i = 0; i < num_midi_tracks; i++) {
      midi_tracks[i].recalc_slides();
    }
  }
#endif

  if (realtime) {
    realtime = false;
    engage_sidechannel = false;
    goto again;
  }
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
  fill_mask = 0;
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
  fill_mask = 0;
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
    if (msg[0] != 153 && msg[2]) {
      mixer_page.track_trig(1, n, MD.kit.levels[n]);
    }

  }
}
void MCLSeqMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  if (track > 15) {
    return;
  }

  SeqTrackUtil::with_md_track(track, [track_param, value](auto &t) {
    t.onControlChangeCallback_Midi(track_param, value);
  });

  if (mcl.currentPage() == MIXER_PAGE) {
    mixer_page.onControlChangeCallback_Midi(track, track_param, value);
  }
  ram_page_a.onControlChangeCallback_Midi(track, track_param, value);

  if (track_param == MODEL_MUTE) { // Mute
    SeqTrackUtil::with_md_track(track, [&](auto &t) { t.mute_state = value > 0; });
   }
  if (track_param >=
      DeviceParamResolver::slot(1, track + 1).param_count()) {
    return;
  }
  uint8_t perf_dest = DeviceParamResolver::perf_dest_from_slot(1, track + 1);
  if (perf_dest != 255) {
    perf_page.learn_param(perf_dest, track_param, value);
  }
  lfo_page.learn_param(1, track + 1, track_param, value);

}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];

  if (param == device_manager.secondary_device()->get_mute_cc()) {

   for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
#if defined(PLATFORM_TBD)
      if (seq_grid_y_runs_tbd_midi_tracks()) {
        if (mcl_seq.midi_tracks[n].channel() != channel) {
          continue;
        }
        if (value > 0) {
          mcl_seq.midi_tracks[n].mute_state = SEQ_MUTE_ON;
        } else {
          mcl_seq.midi_tracks[n].mute_state = SEQ_MUTE_OFF;
        }
      } else {
        if (mcl_seq.ext_tracks[n].channel != channel) {
          continue;
        }
        if (value > 0) {
          mcl_seq.ext_tracks[n].mute_on();
        } else {
          mcl_seq.ext_tracks[n].mute_state = SEQ_MUTE_OFF;
        }
      }
#else
      if (mcl_seq.ext_tracks[n].channel != channel) {
        continue;
      }
      if (value > 0) {
        mcl_seq.ext_tracks[n].mute_on();
      } else {
        mcl_seq.ext_tracks[n].mute_state = SEQ_MUTE_OFF;
      }
#endif
    }
    mixer_page.redraw_mutes = true;
    return;
  }

  uint8_t track = mcl_seq.find_ext_track(channel);
  if (track != 255) {
    uint8_t dest = track + 1;
    uint8_t perf_dest = DeviceParamResolver::perf_dest_from_slot(2, dest);
    if (perf_dest != 255) {
      perf_page.learn_param(perf_dest, param, value);
    }
    lfo_page.learn_param(2, dest, param, value);
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
