#include "DiagnosticPage.h"
#include "MCL_impl.h"

void MCLSeq::setup() {
  // uart = &MidiUart;

  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i++) {

    seq_param_page[i].setEncoders(&seq_param1, &seq_lock1, &seq_param3,
                                  &seq_lock2);
    seq_param_page[i].construct(i * 2, 1 + i * 2);
    seq_param_page[i].page_id = i;
  }
  /*  for (uint8_t i = 0; i < NUM_LFO_PAGES; i++) {
      seq_lfo_page[i].id = i;
      seq_lfo_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3,
                                    &seq_param4);
      for (uint8_t n = 0; n < 48; n++) {
      mcl_seq.lfos[0].samples[n] = n;
              //(uint8_t) (((float) n / (float)48) * (float)96);
      }    } */
  for (uint8_t i = 0; i < num_md_tracks; i++) {

    md_tracks[i].track_number = i;
    md_tracks[i].set_length(16);
    md_tracks[i].speed = SEQ_SPEED_1X;
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
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
    ext_tracks[i].channel = i;
    ext_tracks[i].set_length(16);
    ext_tracks[i].speed = SEQ_SPEED_2X;
  }
#endif
  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    aux_tracks[i].channel = i;
    aux_tracks[i].length = 16;
    aux_tracks[i].speed = SEQ_SPEED_2X;
  }
  //   MidiClock.addOnClockCallback(this,
  //   (midi_clock_callback_ptr_t)&MDSequencer::MDSetup);

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

void MCLSeq::onMidiContinueCallback() { update_params(); }

void MCLSeq::onMidiStartImmediateCallback() {
  realtime = true;
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    // ext_tracks[i].start_clock32th = 0;
    ext_tracks[i].reset();
  }
#endif
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].reset();
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

#ifdef DEFER_SEQ

#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].update_params_offset();
  }
#endif
#endif
}

void MCLSeq::onMidiStartCallback() {}

void MCLSeq::onMidiStopCallback() {
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].buffer_notesoff();
  }
#endif
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
    md_tracks[i].reset_params();
    md_tracks[i].locks_slides_recalc = 255;
    for (uint8_t c = 0; c < 4; c++) {
      md_tracks[i].locks_slide_data[c].init();
    }
  }
  seq_ptc_page.onMidiStopCallback();
#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].reset_params_offset();
  }
#endif
}

void MCLSeq::seq() {

  Stopwatch sw;
  MidiUartParent *uart;
  MidiUartParent *uart2;
  bool engage_sidechannel = true;

  //If realtime, we render the first tick in realtime, subsequent ticks are
  //defered rendered.

  if (!realtime) {
again:
    UART_CLEAR_ISR_TX_BIT();
    UART2_CLEAR_ISR_TX_BIT();
#ifdef DEFER_SEQ
    if (uart_sidechannel) {
      uart = &seq_tx2;
      uart2 = &seq_tx4;
      // If the side channel ring buffer is not empty, it means it did not
      // finish transmiting before next Seq() call. We will drain the old buffer
      // in to the new to retain the MIDI data.
      if (engage_sidechannel) {
        while (!seq_tx2.txRb.isEmpty_isr()) {
          seq_tx1.txRb.put_h_isr(seq_tx2.txRb.get_h_isr());
        }
        while (!seq_tx4.txRb.isEmpty_isr()) {
          seq_tx3.txRb.put_h_isr(seq_tx4.txRb.get_h_isr());
        }
        MidiUart.txRb_sidechannel = &(seq_tx1.txRb);
        MidiUart2.txRb_sidechannel = &(seq_tx3.txRb);
      }
    } else {
      uart = &seq_tx1;
      uart2 = &seq_tx3;
      if (engage_sidechannel) {
        while (!seq_tx1.txRb.isEmpty_isr()) {
          seq_tx2.txRb.put_h_isr(seq_tx1.txRb.get_h_isr());
        }
        while (!seq_tx3.txRb.isEmpty_isr()) {
          seq_tx4.txRb.put_h_isr(seq_tx3.txRb.get_h_isr());
        }
        MidiUart.txRb_sidechannel = &(seq_tx2.txRb);
        MidiUart2.txRb_sidechannel = &(seq_tx4.txRb);
      }
    }
    // clearLed2();
    UART_SET_ISR_TX_BIT();
    UART2_SET_ISR_TX_BIT();
    // Flip uart / side_channel buffer for next run
    uart_sidechannel = !uart_sidechannel;
#else
    uart = &MidiUart;
    uart2 = &MidiUart2;
#endif
  } else {
    uart = &MidiUart;
    uart2 = &MidiUart2;
  }
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].seq(uart);
  }
  // Arp
  seq_ptc_page.on_192_callback();

  for (uint8_t i = 0; i < NUM_AUX_TRACKS; i++) {
    //  aux_tracks[i].seq();
  }

#ifdef LFO_TRACKS
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].seq();
  }
#endif

#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].seq(uart2);
  }
#endif

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].recalc_slides();
  }

  if (realtime) {
    realtime = false;
    engage_sidechannel = false;
    goto again;
  }
  auto seq_time = sw.elapsed();
  // DIAG_MEASURE(0, seq_time);
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
  if (channel < mcl_seq.num_ext_tracks) {
    if (param == 0x5E) {
      mcl_seq.ext_tracks[channel].mute_state = value;
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
