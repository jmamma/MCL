#include "DiagnosticPage.h"
#include "MCLSeq.h"
#include "SeqTrackUtil.h"
#include "MCLGUI.h"
#include "SeqPages.h"
#include "MCL.h"
#include "AuxPages.h"
#include "MCLStrings.h"
#include "DeviceManager.h"
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "../Drivers/MD/MD.h"

void MCLSeq::set_ports(MidiUartClass *md_uart_, MidiUartClass *ext_uart_) {
  md_uart = md_uart_;
  ext_uart = ext_uart_;
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].uart = md_uart;
    md_tracks[i].uart2 = ext_uart;
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].uart = ext_uart;
    ext_tracks[i].uart2 = md_uart;
  }
#endif
#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].uart = ext_uart;
    midi_tracks[i].uart2 = md_uart;
  }
#endif
}

void MCLSeq::setup() {

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
    md_tracks[i].set_length(16);
    md_tracks[i].speed = SEQ_SPEED_1X;
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
    md_arp_tracks[i].track_number = i;
  }
#ifdef LFO_TRACKS

  lfo_tracks[0].load_tables(); //Only needs to be done once

  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].track_number = i;
  }
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
    tbd_tracks[i].p4_sound.p4_track_index = i;
  }
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].seq_data.clear();
    midi_tracks[i].set_channel(i);
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
    if (midi_tracks[n].channel() == channel) {
      return n;
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
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].sample_hold = 0;
    lfo_tracks[i].step_count = 0;
  }
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
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].buffer_notesoff();
    ext_tracks[i].reset_params();
    ext_tracks[i].locks_slides_recalc = 255;
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      ext_tracks[i].locks_slide_data[c].init();
    }
  }
#endif
  MD.reset_dsp_params();

  SeqTrackUtil::for_each_md_track([](auto &track, uint8_t) {
    track.reset_params();
    track.send_notes_off();
    track.locks_slides_recalc = 255;
    for (auto &sd : track.locks_slide_data) { sd.init(); }
  });
#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_tbd_tracks; i++) {
    tbd_tracks[i].locks_slides_recalc = 255;
    for (auto &sd : tbd_tracks[i].locks_slide_data) { sd.init(); }
  }
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].buffer_notesoff();
    midi_tracks[i].reset_params();
  }
#endif
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].reset_params();
  }

#endif
}

void MCLSeq::seq() {
  if (!state) { return; }

  MidiUartClass *uart;
  MidiUartClass *uart2;
  bool engage_sidechannel = true;

  // If realtime, we render the first tick in realtime, subsequent ticks are
  // defered rendered.

  if (!realtime) {
  again:

#if defined(__AVR__)
    md_uart->disable_tx_irq();
    ext_uart->disable_tx_irq();
#else
    md_uart->disable_tx_irq();
    ext_uart->disable_tx_irq();
#endif
    if (uart_sidechannel) {
      uart = &seq_tx2;
      uart2 = &seq_tx4;
      // If the side channel ring buffer is not empty, it means it did not
      // finish transmiting before next Seq() call. We will drain the old buffer
      // in to the new to retain the MIDI data.
      if (engage_sidechannel) {
        md_uart->txRb_sidechannel = seq_tx1.txRb;
        ext_uart->txRb_sidechannel = seq_tx3.txRb;
      } else {
        // Purge stale buffers (from MIDI CONTINUE).
        seq_tx2.txRb->init();
        seq_tx4.txRb->init();
      }
    } else {
      uart = &seq_tx1;
      uart2 = &seq_tx3;
      if (engage_sidechannel) {
        md_uart->txRb_sidechannel = seq_tx2.txRb;
        ext_uart->txRb_sidechannel = seq_tx4.txRb;
      } else {
        seq_tx1.txRb->init();
        seq_tx3.txRb->init();
      }
    }
    // clearLed2();
#if defined(__AVR__)
    md_uart->enable_tx_irq();
    ext_uart->enable_tx_irq();
#else
    //Have to flush the first byte to re-trigger the uart tx isr.
    LOCK();
    md_uart->tx_flush();
    ext_uart->tx_flush();
    CLEAR_LOCK();
#endif
    // Flip uart / side_channel buffer for next run
    uart_sidechannel = !uart_sidechannel;
  } else {
    uart = md_uart;
    uart2 = ext_uart;
  }
  //  Stopwatch sw;

#if !defined(__AVR__)
  if (using_spsx_tracks) {
    spsx_tracks[0].pre_seq(uart);

    for (uint8_t i = 0; i < num_md_tracks; i++) {
      spsx_tracks[i].seq(uart, uart2);
    }

    mdfx_track.seq();

    spsx_tracks[0].post_seq(uart);

    for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
      aux_tracks[i].seq();
    }

#ifdef LFO_TRACKS
    for (uint8_t i = 0; i < num_lfo_tracks; i++) {
      lfo_tracks[i].seq(uart, uart2);
    }
#endif

    perf_track.seq(uart, uart2);

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
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].seq(uart, uart2);
  }
#endif

  perf_track.seq(uart, uart2);

  if (MDSeqTrack::md_trig_mask > 0) {
    MD.parallelTrig(MDSeqTrack::md_trig_mask, uart);
  }
#if !defined(__AVR__)
  }
#endif

#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_tbd_tracks; i++) {
    tbd_tracks[i].seq(uart2);
  }
  for (uint8_t i = 0; i < num_midi_tracks; i++) {
    midi_tracks[i].seq(uart2);
  }
#endif

#if defined(EXT_TRACKS) && !defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].seq(uart2);
    ext_arp_tracks[i].mute_state = ext_tracks[i].mute_state;
    ext_arp_tracks[i].seq(uart,uart2);
  }
#endif

#if !defined(__AVR__)
  if (!using_spsx_tracks) {
#endif
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_tracks[i].recalc_slides();
    }
#if !defined(__AVR__)
  }
#endif

#if !defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].recalc_slides();
  }
#endif
#if defined(PLATFORM_TBD)
  for (uint8_t i = 0; i < num_tbd_tracks; i++) {
    tbd_tracks[i].recalc_slides();
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
  MidiClock.clock_interpolation = LEGACY_SEQ_INTERPOLATION;
  using_spsx_tracks = false;
  return true;
}
#endif

void MCLSeqMidiEvents::onNoteCallback_Midi(uint8_t *msg) {
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
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
      mixer_page.disp_levels[n] = MD.kit.levels[n];
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
  // Engine, not device: perf/lfo learn_param indexes engine-shaped state.
  if (track_param >= (mcl_seq.using_spsx_tracks ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK)) {
    return;
  }
  perf_page.learn_param(track, track_param, value);
  lfo_page.learn_param(track, track_param, value);

}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];

  if (param == device_manager.secondary_device()->get_mute_cc()) {

   for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
#if defined(PLATFORM_TBD)
      if (mcl_seq.midi_tracks[n].channel() != channel) {
        continue;
      }
      if (value > 0) {
        mcl_seq.midi_tracks[n].mute_state = SEQ_MUTE_ON;
      } else {
        mcl_seq.midi_tracks[n].mute_state = SEQ_MUTE_OFF;
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

  perf_page.learn_param(channel + 16 + 4, param, value);
  lfo_page.learn_param(channel + 16 + 4, param, value);

}

void MCLSeqMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  MD.midi->addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  MD.midi->addOnNoteOffCallback(
       this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);

  MD.midi->addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
#ifdef EXT_TRACKS
  generic_midi_device.midi->addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);
#endif
  state = true;
}

void MCLSeqMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }

  MD.midi->removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  MD.midi->removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  MD.midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);

  generic_midi_device.midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
