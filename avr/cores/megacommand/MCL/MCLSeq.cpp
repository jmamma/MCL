#include "DiagnosticPage.h"
#include "MCL_impl.h"

void MCLSeq::setup() {

 for (uint8_t i = 0; i < num_md_tracks; i++) {

    md_tracks[i].track_number = i;
    md_tracks[i].set_length(16);
    md_tracks[i].speed = SEQ_SPEED_1X;
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
    md_arp_tracks[i].track_number = i;
  }
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].track_number = i;
    if (i == 0) {
      lfo_tracks[i].params[0].dest = 17;
      lfo_tracks[i].params[1].dest = 18;
      lfo_tracks[i].params[0].param = 7;
      lfo_tracks[i].params[1].param = 7;
    }
  }
#endif
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].uart = &MidiUart2;
    ext_tracks[i].set_channel(i);
    ext_tracks[i].set_length(16);
    ext_tracks[i].speed = SEQ_SPEED_2X;
    ext_tracks[i].clear();
    ext_tracks[i].init_notes_on();
    ext_arp_tracks[i].track_number = i;
  }
#endif
  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    aux_tracks[i].length = 16;
    aux_tracks[i].speed = SEQ_SPEED_2X;
  }

  enable();

  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStopCallback);
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStartCallback);
  MidiClock.addOnMidiStartImmediateCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStartImmediateCallback);

  MidiClock.addOnMidiContinueCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiContinueCallback);
  midi_events.setup_callbacks();
};

void MCLSeq::enable() {
  if (state) {
    return;
  }
  MidiClock.addOn192Callback(this, (midi_clock_callback_ptr_t)&MCLSeq::seq);
  state = true;
}
void MCLSeq::disable() {
  if (!state) {
    return;
  }
  MidiClock.removeOn192Callback(this, (midi_clock_callback_ptr_t)&MCLSeq::seq);
  state = false;
}
// restore kit params
void MCLSeq::update_kit_params() {
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    mcl_seq.md_tracks[n].update_kit_params();
  }
#ifdef LFO_TRACKS
  for (uint8_t n = 0; n < NUM_LFO_TRACKS; n++) {
    mcl_seq.lfo_tracks[n].update_kit_params();
  }
#endif
}
void MCLSeq::update_params() {
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].update_params();
  }
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].update_params_offset();
  }
#endif
}

void seq_rec_play() {
  if (trig_interface.is_key_down(MDX_KEY_REC)) {
    //trig_interface.ignoreNextEvent(MDX_KEY_REC);
    seq_step_page.bootstrap_record();
    reset_undo();
  }
}

void MCLSeq::onMidiContinueCallback() { update_params(); seq_rec_play(); }

void MCLSeq::onMidiStartImmediateCallback() {
  realtime = true;
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    // ext_tracks[i].start_clock32th = 0;
    ext_tracks[i].reset();
    ext_arp_tracks[i].reset();
  }
#endif
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].reset();
    md_arp_tracks[i].reset();
  }

  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    aux_tracks[i].reset();
  }

#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].sample_hold = 0;
    lfo_tracks[i].step_count = 0;
  }
#endif

  sei();
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].update_params();
  }


#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].update_params_offset();
  }
#endif
  seq_rec_play();
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
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
    md_tracks[i].reset_params();
    md_tracks[i].locks_slides_recalc = 255;
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      md_tracks[i].locks_slide_data[c].init();
    }
  }
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].reset_params_offset();
  }
#endif
}

void MCLSeq::seq() {

  MidiUartParent *uart;
  MidiUartParent *uart2;
  bool engage_sidechannel = true;

  //If realtime, we render the first tick in realtime, subsequent ticks are
  //defered rendered.

  if (!realtime) {
again:
    UART_CLEAR_ISR_TX_BIT();
    UART2_CLEAR_ISR_TX_BIT();
    if (uart_sidechannel) {
      uart = &seq_tx2;
      uart2 = &seq_tx4;
      // If the side channel ring buffer is not empty, it means it did not
      // finish transmiting before next Seq() call. We will drain the old buffer
      // in to the new to retain the MIDI data.
      if (engage_sidechannel) {
/*
        while (!seq_tx2.txRb.isEmpty_isr()) {
          setLed2();
          seq_tx1.txRb.put_h_isr(seq_tx2.txRb.get_h_isr());
        }
        while (!seq_tx4.txRb.isEmpty_isr()) {
          setLed2();
          seq_tx3.txRb.put_h_isr(seq_tx4.txRb.get_h_isr());
        }
*/
        MidiUart.txRb_sidechannel = &(seq_tx1.txRb);
        MidiUart2.txRb_sidechannel = &(seq_tx3.txRb);
      }
      else {
      //Purge stale buffers (from MIDI CONTINUE).
      seq_tx2.txRb.init();
      seq_tx4.txRb.init();
      }
    } else {
      uart = &seq_tx1;
      uart2 = &seq_tx3;
      if (engage_sidechannel) {
/*
        while (!seq_tx1.txRb.isEmpty_isr()) {
          seq_tx2.txRb.put_h_isr(seq_tx1.txRb.get_h_isr());
        }
        while (!seq_tx3.txRb.isEmpty_isr()) {
          seq_tx4.txRb.put_h_isr(seq_tx3.txRb.get_h_isr());
        }
*/
        MidiUart.txRb_sidechannel = &(seq_tx2.txRb);
        MidiUart2.txRb_sidechannel = &(seq_tx4.txRb);
      }
      else {
      seq_tx1.txRb.init();
      seq_tx3.txRb.init();
      }
    }
    // clearLed2();
    UART_SET_ISR_TX_BIT();
    UART2_SET_ISR_TX_BIT();
    // Flip uart / side_channel buffer for next run
    uart_sidechannel = !uart_sidechannel;
  } else {
    uart = &MidiUart;
    uart2 = &MidiUart2;
  }
//  Stopwatch sw;

  MDSeqTrack::md_trig_mask = 0;
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].seq(uart);
    md_arp_tracks[i].seq(uart);
  }

  if (MDSeqTrack::md_trig_mask > 0) { MD.parallelTrig(MDSeqTrack::md_trig_mask, uart); }

  if (MDSeqTrack::sync_cursor) {
    auto &active_track = md_tracks[last_md_track];
    if (GUI.currentPage() == &seq_step_page && IS_BIT_SET(MDSeqTrack::sync_cursor, last_md_track)) { MD.sync_seqtrack(active_track.length, active_track.speed, active_track.length - 1, uart); }
    //MD.draw_pattern_idx(grid_page.last_active_row, grid_page.last_active_row, 0);
    grid_page.set_active_row(grid_page.last_active_row); //send led update
    MDSeqTrack::sync_cursor = 0;
  }
  // Arp

  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
     aux_tracks[i].seq();
  }

#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].seq(uart);
  }
#endif

#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].seq(uart2);
    ext_arp_tracks[i].seq(uart2);
  }
#endif

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].recalc_slides();
  }

  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].recalc_slides();
  }


  if (realtime) {
    realtime = false;
    engage_sidechannel = false;
    goto again;
  }

}

void MCLSeqMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  if (!update_params) {
    return;
  }
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  if (param >= 16) {
    MD.parseCC(channel, param, &track, &track_param);
    if (track_param > 23) { return; } //ignore level/mute

    mcl_seq.md_tracks[track].update_param(track_param, value);
#ifdef LFO_TRACKS
    for (uint8_t n = 0; n < mcl_seq.num_lfo_tracks; n++) {
      mcl_seq.lfo_tracks[n].check_and_update_params_offset(track + 1,
                                                           track_param, value);
    }
#endif
  }
}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
#ifdef EXT_TRACKS
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {
      if (param == 0x5E) {
        mcl_seq.ext_tracks[n].mute_state = value;
      } else {
        mcl_seq.ext_tracks[n].update_param(param, value);
      }
    }
  }
#endif
}

void MCLSeqMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  /*
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback_Midi);
  */
  update_params = true;
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
#ifdef EXT_TRACKS
  Midi2.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);
#endif
  state = true;
}

void MCLSeqMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  /*
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback_Midi);
  */
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);

  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
