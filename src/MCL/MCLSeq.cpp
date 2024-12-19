#include "DiagnosticPage.h"
#include "MCLSeq.h"
#include "MCLGUI.h"
#include "SeqPages.h"
#include "MCL.h"
#include "AuxPages.h"
#include "GridTask.h"

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
    ext_tracks[i].uart = &MidiUart2;
    ext_tracks[i].set_channel(i);
    ext_tracks[i].set_length(16);
    ext_tracks[i].speed = SEQ_SPEED_1X;
    ext_tracks[i].clear();
    ext_tracks[i].init_notes_on();
    ext_tracks[i].track_number = i;
    ext_arp_tracks[i].track_number = i;
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
  if (trig_interface.is_key_down(MDX_KEY_REC)) {
    // trig_interface.ignoreNextEvent(MDX_KEY_REC);
    seq_step_page.bootstrap_record();
    reset_undo();
  }
}

uint8_t MCLSeq::find_ext_track(uint8_t channel) {
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (ext_tracks[n].channel == channel) {
      return n;
    }
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

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].reset();
    md_arp_tracks[i].reset();
  }

  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    // ext_tracks[i].start_clock32th = 0;
    ext_tracks[i].reset();
    ext_arp_tracks[i].reset();
  }

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
    oled_display.textbox("REC", "");
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

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].reset_params();
    md_tracks[i].send_notes_off();
    md_tracks[i].locks_slides_recalc = 255;
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      md_tracks[i].locks_slide_data[c].init();
    }
  }
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
    UART_CLEAR_ISR_TX_BIT();
    UART2_CLEAR_ISR_TX_BIT();
#else
    MidiUart.disable_tx_irq();
    MidiUart2.disable_tx_irq();
#endif
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
        MidiUart.txRb_sidechannel = seq_tx1.txRb;
        MidiUart2.txRb_sidechannel = seq_tx3.txRb;
      } else {
        // Purge stale buffers (from MIDI CONTINUE).
        seq_tx2.txRb->init();
        seq_tx4.txRb->init();
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
        MidiUart.txRb_sidechannel = seq_tx2.txRb;
        MidiUart2.txRb_sidechannel = seq_tx4.txRb;
      } else {
        seq_tx1.txRb->init();
        seq_tx3.txRb->init();
      }
    }
    // clearLed2();
#if defined(__AVR__)
    UART_SET_ISR_TX_BIT();
    UART2_SET_ISR_TX_BIT()
#else
    //Have to flush the first byte to re-trigger the uart tx isr.
    MidiUart.tx_isr();
    MidiUart2.tx_isr();
#endif
    // Flip uart / side_channel buffer for next run
    uart_sidechannel = !uart_sidechannel;
  } else {
    uart = &MidiUart;
    uart2 = &MidiUart2;
  }
  //  Stopwatch sw;
  MDSeqTrack::md_trig_mask = 0;
  MDSeqTrack::load_machine_cache = 0;
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].seq(uart,uart2);
    md_arp_tracks[i].mute_state = md_tracks[i].mute_state;
    md_arp_tracks[i].seq(uart,uart2);
  }

  mdfx_track.seq();

  if (MDSeqTrack::load_machine_cache) {
    MD.setKitName(grid_task.kit_names[0], uart);
    MD.loadMachinesCache(MDSeqTrack::load_machine_cache, uart);
  }

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

#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].seq(uart2);
    ext_arp_tracks[i].mute_state = ext_tracks[i].mute_state;
    ext_arp_tracks[i].seq(uart,uart2);
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

void MCLSeqMidiEvents::onNoteCallback_Midi(uint8_t *msg) {
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t n = MD.noteToTrack(msg[1]);
  if (n < 16) {
    bool is_midi_machine = ((MD.kit.models[n] & 0xF0) == MID_01_MODEL);
    if (is_midi_machine) {
      if (msg[2]) {mcl_seq.md_tracks[n].send_notes(255); }
      //velocity 0 == NoteOff
      //Only send note off if the sequener is not running, otherwise defer to note length
      else if (MidiClock.state != 2) { mcl_seq.md_tracks[n].send_notes_off(); }

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

  mcl_seq.md_tracks[track].onControlChangeCallback_Midi(track_param, value);

  if (mcl.currentPage() == MIXER_PAGE) {
    mixer_page.onControlChangeCallback_Midi(track, track_param, value);
  }
  ram_page_a.onControlChangeCallback_Midi(track, track_param, value);

  if (track_param == 32) { // Mute
    mcl_seq.md_tracks[track].mute_state = value > 0;
    if (mixer_page.current_mute_set != 255) {
      if (value > 0) {
        CLEAR_BIT16(mixer_page.mute_sets[0].mutes[mixer_page.current_mute_set],
                    track);
      } else {
        SET_BIT16(mixer_page.mute_sets[0].mutes[mixer_page.current_mute_set],
                  track);
      }
    }
  }
  if (track_param > 23) {
    return;
  } // ignore level/mute
  perf_page.learn_param(track, track_param, value);
  lfo_page.learn_param(track, track_param, value);

  if (!update_params) {
    return;
  }
}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];

  if (param == midi_active_peering.get_device(UART2_PORT)->get_mute_cc()) {
   for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
      if (mcl_seq.ext_tracks[n].channel != channel) {
        continue;
      }
      mcl_seq.ext_tracks[n].mute_state = value > 0;
      if (mixer_page.current_mute_set == 255) {
        continue;
      }
      if (value > 0) {
        CLEAR_BIT16(
            mixer_page.mute_sets[1].mutes[mixer_page.current_mute_set], n);
        mcl_seq.ext_tracks[n].buffer_notesoff();
      } else {
        SET_BIT16(
            mixer_page.mute_sets[1].mutes[mixer_page.current_mute_set], n);
      }
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

  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  Midi.addOnNoteOffCallback(
       this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);

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

  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);

  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
