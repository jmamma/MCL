#include "MCL.h"
#include "SeqPtcPage.h"

#define MIDI_LOCAL_MODE 0

void SeqPtcPage::setup() {
  SeqPage::setup();
  init_poly();
  midi_events.setup_callbacks();
}
void SeqPtcPage::cleanup() {
  SeqPage::cleanup();
  record_mode = false;
  if (MidiClock.state != 2) {
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
  }
  //  midi_events.remove_callbacks();
}
void SeqPtcPage::config_encoders() {
  if (midi_device == DEVICE_MD) {
    ((MCLEncoder *)encoders[2])->max = 64;

    encoders[2]->cur = mcl_seq.md_tracks[last_md_track].length;
  } else {
    ((MCLEncoder *)encoders[2])->max = (uint8_t)128;
    ((MCLEncoder *)encoders[2])->cur =
        mcl_seq.ext_tracks[last_ext_track].length;
  }
}

uint8_t SeqPtcPage::calc_poly_count() {
  DEBUG_PRINT_FN();
  uint8_t count = 0;
  for (uint8_t x = 0; x < 16; x++) {
    if (IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      count++;
    }
  }
  DEBUG_PRINTLN(count);
  return count;
}

void SeqPtcPage::init_poly() {
  poly_count = 0;
  poly_max = calc_poly_count();
  for (uint8_t x = 0; x < 16; x++) {
    poly_notes[x] = -1;
  }
}

void SeqPtcPage::init() {
  DEBUG_PRINT_FN();
  SeqPage::init();
  ((MCLEncoder *)encoders[2])->handler = ptc_pattern_len_handler;
  record_mode = false;
 midi_events.setup_callbacks();
  DEBUG_PRINTLN("control mode:");
  DEBUG_PRINTLN(mcl_cfg.uart2_ctrl_mode);
  if (mcl_cfg.uart2_ctrl_mode == MIDI_LOCAL_MODE) {
    md_exploit.on();
    note_interface.state = true;
  } else {
    md_exploit.off();
    last_md_track = MD.currentTrack;
  }
  config_encoders();
  encoders[1]->cur = 32;
  encoders[0]->cur = 1;
  curpage = SEQ_PTC_PAGE;
}

void ptc_pattern_len_handler(Encoder *enc) {
  MCLEncoder *enc_ = (MCLEncoder *)enc;
  if (SeqPage::midi_device == DEVICE_MD) {

    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < 16; c++) {
        mcl_seq.md_tracks[c].set_length(enc_->cur);
      }
    } else {

      if (seq_ptc_page.poly_count > 1) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_seq.md_tracks[c].set_length(enc_->cur);
          }
        }
      } else {
        mcl_seq.md_tracks[last_md_track].set_length(enc_->cur);
      }
    }

  } else {
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < mcl_seq.num_ext_tracks; c++) {
        mcl_seq.ext_tracks[c].buffer_notesoff();
        mcl_seq.ext_tracks[c].set_length(enc_->cur);
      }
    } else {
      mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
      mcl_seq.ext_tracks[last_ext_track].set_length(enc_->cur);
    }
  }
}
void SeqPtcPage::loop() {
  if (encoders[0]->hasChanged() || encoders[3]->hasChanged()) {
    mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
  }
}
void SeqPtcPage::display() {
  uint8_t dev_num;
  if (!redisplay) {
    return true;
  }
  if (midi_device == DEVICE_MD) {
    dev_num = last_md_track;
  } else {
    dev_num = last_ext_track + 16;
  }
  const char *str1 = getMachineNameShort(MD.kit.models[dev_num], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[dev_num], 2);
  GUI.setLine(GUI.LINE1);

  if (record_mode) {
    GUI.put_string_at(0, "RPTC");
  } else {
    GUI.put_string_at(0, "PTC");
  }
  if (midi_device == DEVICE_MD) {
    GUI.put_value_at(5, encoders[2]->getValue());
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
  } else {
    GUI.put_value_at(5, (encoders[2]->getValue() /
                         (2 / mcl_seq.ext_tracks[last_ext_track].resolution)));
    if (Analog4.connected) {
      GUI.put_string_at(9, "A4T");
    } else {
      GUI.put_string_at(9, "MID");
    }
    GUI.put_value_at1(12, last_ext_track + 1);
  }

  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "OC:");
  GUI.put_value_at2(3, encoders[0]->getValue());

  if (encoders[1]->getValue() < 32) {
    GUI.put_string_at(6, "F:-");
    GUI.put_value_at2(9, 32 - encoders[1]->getValue());

  } else if (encoders[1]->getValue() > 32) {
    GUI.put_string_at(6, "F:+");
    GUI.put_value_at2(9, encoders[1]->getValue() - 32);

  } else {
    GUI.put_string_at(6, "F: 0");
  }

  GUI.put_string_at(12, "S:");

  GUI.put_value_at2(14, encoders[3]->getValue());
  SeqPage::display();
}

uint8_t SeqPtcPage::calc_pitch(uint8_t note_num) {
  uint8_t size = scales[encoders[3]->cur]->size;
  uint8_t oct = note_num / size;
  note_num = note_num - oct * size;

  note_num = scales[encoders[3]->cur]->pitches[note_num];

  uint8_t pitch = encoders[0]->getValue() * 12 + oct * 12 + note_num;
  return pitch;
}

uint8_t SeqPtcPage::get_next_voice(uint8_t pitch) {
  uint8_t voice = 255;
  uint8_t count = 0;

  if (poly_max == 0) {
  return last_md_track;
  }
  // If track previously played pitch, re-use this track
  for (uint8_t x = 0; x < 16; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (poly_notes[x] == pitch) {
        return x;
      }

      // Search for empty track.
      if (poly_notes[x] == -1) {
        voice = x;
      }
    }
  }
  // Reuse existing track for noew pitch
  for (uint8_t x = 0; x < 16 && voice == 255; x++) {
    if (MD.isMelodicTrack(x) && IS_BIT_SET16(mcl_cfg.poly_mask, x)) {
      if (count == poly_count) {
        voice = x;
      } else {
        count++;
      }
    }
  }
  poly_count++;
  if ((poly_count >= 15) || (poly_count >= poly_max)) {
    poly_count = 0;
  }
  poly_notes[voice] = pitch;
  return voice;
}

uint8_t SeqPtcPage::get_machine_pitch(uint8_t track, uint8_t pitch) {
  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[track]);

  if (tuning == NULL) {
    return 0;
  }

  if (pitch >= tuning->len) {
    pitch = tuning->len - 1;
  }

  uint8_t machine_pitch =
      pgm_read_byte(&tuning->tuning[pitch]) + encoders[1]->getValue() - 32;
  return machine_pitch;
}
void SeqPtcPage::trig_md(uint8_t note_num) {
  uint8_t pitch = calc_pitch(note_num);
  uint8_t next_track = get_next_voice(pitch);
  uint8_t machine_pitch = get_machine_pitch(next_track, pitch);
  MD.setTrackParam(next_track, 0, machine_pitch);
  if (!BUTTON_DOWN(Buttons.BUTTON2)) {
    MD.triggerTrack(next_track, 127);
  }
  if ((record_mode) && (MidiClock.state == 2)) {

    if (!BUTTON_DOWN(Buttons.BUTTON2)) {
      mcl_seq.md_tracks[next_track].record_track(note_num, 127);
    }
    mcl_seq.md_tracks[next_track].record_track_pitch(machine_pitch);
  }
}
void SeqPtcPage::trig_md_fromext(uint8_t note_num) {
  uint8_t pitch = seq_ext_pitch(note_num - 32);
  uint8_t next_track = get_next_voice(pitch);
  uint8_t machine_pitch = get_machine_pitch(next_track, pitch);
  MD.setTrackParam(next_track, 0, machine_pitch);
  if (!BUTTON_DOWN(Buttons.BUTTON2)) {
    MD.triggerTrack(next_track, 127);
  }
  if ((record_mode) && (MidiClock.state == 2)) {
    if (!BUTTON_DOWN(Buttons.BUTTON2)) {
      mcl_seq.md_tracks[next_track].record_track(note_num, 127);
    }
    mcl_seq.md_tracks[next_track].record_track_pitch(machine_pitch);
  }
}

bool SeqPtcPage::handleEvent(gui_event_t *event) {
//  if (SeqPage::handleEvent(event)) {
//    return;
//  }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    DEBUG_PRINTLN("yep");
    // note interface presses are treated as musical notes here
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device != DEVICE_MD) {
        return;
      }
      midi_device = device;
      config_encoders();
      trig_md(track);
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      //      draw_notes(0);

      return true;
    }
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    redisplay = true;
    record_mode = !record_mode;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {
    if (midi_device == DEVICE_MD) {
      for (uint8_t n = 0; n < mcl_seq.num_md_tracks; n++) {
        mcl_seq.md_tracks[n].clear_track();
      }

    } else {
      for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
        mcl_seq.ext_tracks[n].clear_track();
      }
    }

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON2)) {
    if (midi_device != DEVICE_MD) {
      if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
        mcl_seq.ext_tracks[last_ext_track].resolution = 2;
      } else {
        mcl_seq.ext_tracks[last_ext_track].resolution = 1;
      }
    }
    redisplay = true;
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {

    if (midi_device == DEVICE_MD) {

      if (poly_count > 1) {
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {

            mcl_seq.md_tracks[c].clear_track();
          }
        }
      }

      else {
        mcl_seq.md_tracks[last_md_track].clear_track();
      }
    } else {
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}

uint8_t SeqPtcPage::seq_ext_pitch(uint8_t note_num) {
  uint8_t note_orig = note_num;
  uint8_t pitch;

  uint8_t root_note = (note_num / 12) * 12;
  uint8_t pos = note_num - root_note;
  uint8_t oct = note_num / 12;
  // if (pos >= scales[encoders[4]->cur]->size) {
  oct += pos / scales[encoders[3]->cur]->size;
  pos = pos -
        scales[encoders[3]->cur]->size * (pos / scales[encoders[3]->cur]->size);
  // }

  //  if (encoders[4]->getValue() > 0) {
  pitch = encoders[0]->getValue() * 12 +
          scales[encoders[3]->cur]->pitches[pos] + oct * 12;
  //   }

  return pitch;
}
void SeqPtcMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  DEBUG_PRINTLN("note on midi2");
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  DEBUG_PRINTLN(channel);
  if ((GUI.currentPage() == &seq_step_page) ||
      (GUI.currentPage() == &seq_extstep_page) ||
      (GUI.currentPage() == &grid_save_page) ||
      (GUI.currentPage() == &grid_write_page)) {
    return;
  }
  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode == MIDI_OMNI_MODE)) {
    seq_ptc_page.trig_md_fromext(note_num);
    SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);
    return;
  }

  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
  if (channel >= mcl_seq.num_ext_tracks) {
    return;
  }
  if (last_ext_track != channel) {
    seq_ptc_page.redisplay = true;
  }
  last_ext_track = channel;
  seq_ptc_page.config_encoders();

  DEBUG_PRINTLN(mcl_seq.ext_tracks[channel].length);
  uint8_t pitch = seq_ptc_page.seq_ext_pitch(note_num);
  MidiUart2.sendNoteOn(channel, pitch, msg[2]);
  if ((seq_ptc_page.record_mode) && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[channel].record_ext_track_noteon(pitch, msg[2]);
  }
}
void SeqPtcMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  DEBUG_PRINTLN("note off midi2");
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if ((GUI.currentPage() == &seq_step_page) ||
      (GUI.currentPage() == &seq_extstep_page) ||
      (GUI.currentPage() == &grid_save_page) ||
      (GUI.currentPage() == &grid_write_page)) {
    return;
  }

  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode  == MIDI_OMNI_MODE)) {
    return;
  }

  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
  if (channel >= mcl_seq.num_ext_tracks) {
    return;
  }
  last_ext_track = channel;
  seq_ptc_page.config_encoders();

  uint8_t pitch = seq_ptc_page.seq_ext_pitch(note_num);
  MidiUart2.sendNoteOff(channel, pitch, msg[2]);
  if (seq_ptc_page.record_mode && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[channel].record_ext_track_noteoff(pitch, msg[2]);
  }
}

void SeqPtcMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks
  uint8_t param_true = 0;
  if (param >= 16) {
    param_true = 1;
  }
  if (param < 63) {
    param = param - 16;
    track = (param / 24) + (channel - MD.global.baseChannel) * 4;
    track_param = param - ((param / 24) * 24);
  } else if (param >= 72) {
    param = param - 72;
    track = (param / 24) + 2 + (channel - MD.global.baseChannel) * 4;
    track_param = param - ((param / 24) * 24);
  }
  uint8_t start_track;

  if ((seq_ptc_page.poly_max > 1)) {
    if (IS_BIT_SET16(mcl_cfg.poly_mask, track)) {

      for (uint8_t n = 0; n < 16; n++) {

        if (IS_BIT_SET16(mcl_cfg.poly_mask, n) && (n != track)) {
          if (param_true) {
            MD.setTrackParam(n, track_param, value);
          }
        }
        // in_sysex = 0;
      }
    }
  }
}

void SeqPtcMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);

  state = true;
}

void SeqPtcMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }

  DEBUG_PRINTLN("remove calblacks");
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcMidiEvents::onNoteOffCallback_Midi2);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqPtcMidiEvents::onControlChangeCallback_Midi);

  state = false;
}

scale_t *scales[16]{
    &chromaticScale, &ionianScale,
    //&dorianScale,
    &phrygianScale,
    //&lydianScale,
    //&mixolydianScale,
    //&aeolianScale,
    //&locrianScale,
    &harmonicMinorScale, &melodicMinorScale,
    //&lydianDominantScale,
    //&wholeToneScale,
    //&wholeHalfStepScale,
    //&halfWholeStepScale,
    &majorPentatonicScale, &minorPentatonicScale, &suspendedPentatonicScale,
    &inSenScale, &bluesScale,
    //&majorBebopScale,
    //&dominantBebopScale,
    //&minorBebopScale,
    &majorArp, &minorArp, &majorMaj7Arp, &majorMin7Arp, &minorMin7Arp,
    //&minorMaj7Arp,
    &majorMaj7Arp9,
    //&majorMaj7ArpMin9,
    //&majorMin7Arp9,
    //&majorMin7ArpMin9,
    //&minorMin7Arp9,
    //&minorMin7ArpMin9,
    //&minorMaj7Arp9,
    //&minorMaj7ArpMin9
};
