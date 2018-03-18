#include "SeqPtcPage.h"
void SeqPtcPage::setup() {
  SeqPage::setup();
  midi_events.setup_callbacks();
}
void SeqPtcPage::cleanup() { midi_events.remove_callbacks(); }
void SeqPtcPage::init() {
  collect_trigs = false;

  encoders[1]->max = 8;
  encoders[2]->max = 64;
  encoders[3]->max = 64;
  encoders[4]->max = 15;
  encoders[2]->cur = 32;
  encoders[1]->cur = 1;

  encoders[3]->cur = PatternLengths[last_md_track];

  curpage = SEQ_PTC_PAGE;
}
void SeqPtcPage::pattern_len_handler(Encoder *enc) {
  if (grid.cur_col < 16) {
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < 16; c++) {
        PatternLengths[c] = encoders[3]->getValue();
      }
    }
    PatternLengths[last_md_track] = encoders[3]->getValue();
  } else {
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      for (uint8_t c = 0; c < 6; c++) {
        ExtPatternLengths[c] = encoders[3]->getValue();
      }
    }
    ExtPatternLengths[last_ext_track] = encoders[3]->getValue();
  }
}
md_exploit.init_notes();
}

bool SeqPtcPage::display() {
  const char *str1 = getMachineNameShort(MD.kit.models[grid.cur_col], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[grid.cur_col], 2);
  GUI.setLine(GUI.LINE1);

  if (recordmode) {
    GUI.put_string_at(0, "RPTC");
  } else {
    GUI.put_string_at(0, "PTC");
  }
  if (grid.cur_col < 16) {
    GUI.put_value_at(5, encoders[3]->getValue());
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
  } else {
    GUI.put_value_at(5, (encoders[3]->getValue() /
                         (2 / ExtPatternResolution[last_ext_track])));
    if (Analog4.connected) {
      GUI.put_string_at(9, "A4T");
    } else {
      GUI.put_string_at(9, "MID");
    }
    GUI.put_value_at1(12, grid.cur_col - 16 + 1);
  }
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "OC:");
  GUI.put_value_at2(3, encoders[1]->getValue());

  if (encoders[2]->getValue() < 32) {
    GUI.put_string_at(6, "F:-");
    GUI.put_value_at2(9, 32 - encoders[2]->getValue());

  } else if (encoders[2]->getValue() > 32) {
    GUI.put_string_at(6, "F:+");
    GUI.put_value_at2(9, encoders[2]->getValue() - 32);

  } else {
    GUI.put_string_at(6, "F: 0");
  }

  GUI.put_string_at(12, "S:");

  GUI.put_value_at2(14, encoders[4]->getValue());
}

uint8_t SeqPtcPage::calc_pitch(uint8_t note_num) {
  uint8_t size = scales[encoders[4]->cur]->size;
  uint8_t oct = note_num / size;
  note_num = note_num - oct * size;

  note_num = scales[encoders[4]->cur]->pitches[note_num];

  uint8_t pitch = encoders[1]->getValue() * 12 + oct * 12 + note_num;
}

uint8_t SeqPtcPage::get_next_track() {
  if (poly_count >= poly_max) {
    poly_count = 0;
  } else {
    poly_count++;
  }
  uint8_t next_track = last_md_track + poly_max - 1;
  if (next_track < 15) {
    return next_track;
  } else {
    return 15;
  }
}

uint8_t SeqPtcPage::get_machine_pitch(uint8_t track, uint8_t pitch) {
  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);

  if (tuning == NULL) {
    return 0;
  }

  if (pitch >= tuning->len) {
    pitch = tuning->len - 1;
  }

  machine_pitch =
      pgm_read_byte(&tuning->tuning[pitch]) + encoders[2]->getValue() - 32;
  return machine_pitch;
}
bool SeqPtcPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    // note interface presses are treated as musical notes here
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t note_num = track;
      encoders[3]->cur = PatternLengths[last_md_track];
      encoders[3]->max = 64;
      uint8_t pitch = calc_pitch(note_num);
      uint8_t next_track = poly_next_track();
      uint8_t machine_pitch = get_machine_pitch(next_track, pitch);
      setTrackParam(next_track, 0, machine_pitch);
      MD.triggerTrack(next_track, 127);
      if ((record) && (MidiClock.state == 2)) {
        mcl_seq.record_track(next_track, note_num, 127);
        mcl_seq.set_track_pitch(next_track, machine_pitch);
      }
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      //      draw_notes(0);

      return true;
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    recordmode = ~recordmode;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {

    load_seq_page(SEQ_STEP_PAGE);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {

    load_seq_page(SEQ_RTRK_PAGE);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    load_seq_page(SEQ_PARAM_A_PAGE);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {

    load_seq_page(SEQ_PTC_PAGE);

    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 6; n++) {
      mcl_seq.clear_ext_track(n);
    }

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON3)) {
    if (ExtPatternResolution[last_ext_track] == 1) {
      ExtPatternResolution[last_ext_track] = 2;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        load_seq_extstep_page(last_ext_track);
      }

    } else {
      ExtPatternResolution[last_ext_track] = 1;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        load_seq_extstep_page(last_ext_track);
      }
    }

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {

    if (grid.cur_col < 16) {
      clear_seq_track(last_md_track);
    } else {
      mcl_seq.clear_ext_track(last_ext_track);
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
  oct += pos / scales[encoders[4]->cur]->size;
  pos = pos -
        scales[encoders[4]->cur]->size * (pos / scales[encoders[4]->cur]->size);
  // }

  //  if (encoders[4]->getValue() > 0) {
  pitch = encoders[1]->getValue() * 12 +
          scales[encoders[4]->cur]->pitches[pos] + oct * 12;
  //   }

  return pitch;
}
void SeqPtcStep::onNoteOnCallback_Midi2(uint8_t *msg) {

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  last_Extseq_track = channel;
  grid.cur_col = 16 + channel;
  encoders[3]->max = 128;
  encoders[3]->cur = ExtPatternLengths[channel];

  pitch = seq_ext_pitch(note_num);
  MidiUart2.sendNoteOn(channel, pitch, msg[2]);
  if ((recordmode) && (MidiClock.state == 2)) {
    record_ext_track_noteon(channel, msg[2], msg[3]);
  }
}
void SeqPtcStep::onNoteOffCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  last_Extseq_track = channel;
  grid.cur_col = 16 + channel;
  encoders[3]->max = 128;
  encoders[3]->cur = ExtPatternLengths[channel];

  pitch = seq_ext_pitch(note_num);
  MidiUart2.sendNoteOff(channel, pitch, msg[2]);
  if (recordmode && (MidiClock.state == 2)) {
    record_ext_track_noteoff(channel, msg[2], msg[3]);
  }
}

void SeqPtcStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcStepMidiEvents::onNoteOnCallbackMidi2);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcStepMidiEvents::onNoteOffCallbackMidi2);
  state = true;
}

void SeqPtcStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqPtcStepMidiEvents::onNoteOnCallbackMidi2);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&SeqPtcStepMidiEvents::onNoteOffCallbackMidi2);
  state = false;
}

const scale_t *scales[16] {
  &chromaticScale,
  &ionianScale,
  //&dorianScale,
  &phrygianScale,
  //&lydianScale,
  //&mixolydianScale,
  //&aeolianScale,
  //&locrianScale,
  &harmonicMinorScale,
  &melodicMinorScale,
  //&lydianDominantScale,
  //&wholeToneScale,
  //&wholeHalfStepScale,
  //&halfWholeStepScale,
  &majorPentatonicScale,
  &minorPentatonicScale,
  &suspendedPentatonicScale,
  &inSenScale,
  &bluesScale,
  //&majorBebopScale,
  //&dominantBebopScale,
  //&minorBebopScale,
  &majorArp,
  &minorArp,
  &majorMaj7Arp,
  &majorMin7Arp,
  &minorMin7Arp,
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

