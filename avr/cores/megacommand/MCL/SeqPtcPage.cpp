#include "ArpPage.h"
#include "MCL.h"
#include "SeqPtcPage.h"

#define MIDI_LOCAL_MODE 0
#define NUM_KEYS 24

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

typedef char scale_name_t[4];

const scale_name_t scale_names[] PROGMEM = {
    "---", "ION", "PHR", "mHA", "mME", "MPE", "mPE", "sPE",
    "ISS", "BLU", "MAJ", "MIN", "MM7", "Mm7", "mm7", "M79",
};

void SeqPtcPage::setup() {
  SeqPage::setup();
  init_poly();
  midi_events.setup_callbacks();
}
void SeqPtcPage::cleanup() {
  SeqPage::cleanup();
  recording = false;
  if (MidiClock.state != 2) {
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
  }
  //  midi_events.remove_callbacks();
}
void SeqPtcPage::config_encoders() {
  if (midi_device == DEVICE_MD) {
    ptc_param_len.max = 64;
    ptc_param_len.cur = mcl_seq.md_tracks[last_md_track].length;
  }
#ifdef EXT_TRACKS
  else {
    ptc_param_len.max = (uint8_t)128;
    ptc_param_len.cur = mcl_seq.ext_tracks[last_ext_track].length;
  }
#endif
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
  seq_menu_page.menu.enable_entry(0, true);
  ptc_param_len.handler = ptc_pattern_len_handler;
  recording = false;
  midi_events.setup_callbacks();
  note_mask = 0;
  DEBUG_PRINTLN("control mode:");
  DEBUG_PRINTLN(mcl_cfg.uart2_ctrl_mode);
  if (mcl_cfg.uart2_ctrl_mode == MIDI_LOCAL_MODE) {
    trig_interface.on();
    note_interface.state = true;
  } else {
    trig_interface.off();
    last_md_track = MD.currentTrack;
  }
  curpage = SEQ_PTC_PAGE;

  config();
  re_init = false;
}

void SeqPtcPage::config() {
  config_encoders();
  ptc_param_finetune.cur = 32;
  ptc_param_oct.cur = 1;

  // config info labels
  constexpr uint8_t len1 = sizeof(info1);
  char buf[len1] = {'\0'};

  char str_first[3] = "--";
  char str_second[3] = "--";
  if (midi_device == DEVICE_MD) {
    char *str1;
    char *str2;
    str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
    str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

    m_strncpy_p(str_first, str1, len1);

    m_strncpy_p(str_second, str2, len1);
  }
#ifdef EXT_TRACKS
  else {
    if (Analog4.connected) {
      strcpy(str_first, "A4");
    } else {
      strcpy(str_first, "MI");
    }
    str_second[0] = 'T';
    str_second[1] = last_ext_track + '1';
  }
#endif
  strncpy(info1, str_first, len1);
  strncat(info1, ">", len1);
  strncat(info1, str_second, len1);

  strcpy(info2, "CHROMAT");
  display_page_index = false;

  // config menu
  config_as_trackedit();
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

  }
#ifdef EXT_TRACKS
  else {
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
#endif
}

void SeqPtcPage::loop() {
  if (re_init) {
    init();
  }
#ifdef EXT_TRACKS
  if (ptc_param_oct.hasChanged() || ptc_param_scale.hasChanged()) {
    mcl_seq.ext_tracks[last_ext_track].buffer_notesoff();
    note_mask = 0;
    recalc_notemask();
    render_arp();
  }
#endif

  if (ptc_param_oct.hasChanged() || ptc_param_finetune.hasChanged() ||
      ptc_param_len.hasChanged() || ptc_param_scale.hasChanged()) {
    queue_redraw();
  }

  if (last_midi_state != MidiClock.state) {
    last_midi_state = MidiClock.state;
    redisplay = true;
  }

  if (deferred_timer != 0 &&
      clock_diff(deferred_timer, slowclock) > render_defer_time) {
    deferred_timer = 0;
    redisplay = true;
  }

  SeqPage::loop();
}

#ifndef OLED_DISPLAY
void SeqPtcPage::display() {
  uint8_t dev_num;
  if (!redisplay) {
    return true;
  }
  if (midi_device == DEVICE_MD) {
    dev_num = last_md_track;
  }
#ifdef EXT_TRACKS
  else {
    dev_num = last_ext_track + 16;
  }
#endif
  const char *str1 = getMachineNameShort(MD.kit.models[dev_num], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[dev_num], 2);
  GUI.setLine(GUI.LINE1);

  if (recording) {
    GUI.put_string_at(0, "RPTC");
  } else {
    GUI.put_string_at(0, "PTC");
  }
  if (midi_device == DEVICE_MD) {
    GUI.put_value_at(5, ptc_param_len.getValue());
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
  }
#ifdef EXT_TRACKS
  else {
    GUI.put_value_at(5, (ptc_param_len.getValue() /
                         (2 / mcl_seq.ext_tracks[last_ext_track].resolution)));
    if (Analog4.connected) {
      GUI.put_string_at(9, "A4T");
    } else {
      GUI.put_string_at(9, "MID");
    }
    GUI.put_value_at1(12, last_ext_track + 1);
  }
#endif

  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "OC:");
  GUI.put_value_at2(3, ptc_param_oct.getValue());

  if (ptc_param_finetune.getValue() < 32) {
    GUI.put_string_at(6, "F:-");
    GUI.put_value_at2(9, 32 - ptc_param_finetune.getValue());

  } else if (ptc_param_finetune.getValue() > 32) {
    GUI.put_string_at(6, "F:+");
    GUI.put_value_at2(9, ptc_param_finetune.getValue() - 32);

  } else {
    GUI.put_string_at(6, "F: 0");
  }

  GUI.put_string_at(12, "S:");

  GUI.put_value_at2(14, ptc_param_scale.getValue());
  SeqPage::display();
}
#else
void SeqPtcPage::display() {
  uint8_t dev_num;
  if (!redisplay) {
    return;
  }

  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();

  if (midi_device == DEVICE_MD) {
    dev_num = last_md_track;
  }
#ifdef EXT_TRACKS
  else {
    dev_num = last_ext_track + 16;
  }
#endif

  draw_knob_frame();
  char buf1[4];

  // draw OCTAVE
  itoa(ptc_param_oct.getValue(), buf1, 10);
  draw_knob(0, "OCT", buf1);

  // draw FREQ
  if (ptc_param_finetune.getValue() < 32) {
    strcpy(buf1, "-");
    itoa(32 - ptc_param_finetune.getValue(), buf1 + 1, 10);
  } else if (ptc_param_finetune.getValue() > 32) {
    strcpy(buf1, "+");
    itoa(ptc_param_finetune.getValue() - 32, buf1 + 1, 10);
  } else {
    strcpy(buf1, "0");
  }
  draw_knob(1, "DET", buf1); // detune

  // draw LEN
  if (midi_device == DEVICE_MD) {
    itoa(ptc_param_len.getValue(), buf1, 10);
    draw_knob(2, "LEN", buf1);
  }
#ifdef EXT_TRACKS
  else {
    itoa(ptc_param_len.getValue() /
             (2 / mcl_seq.ext_tracks[last_ext_track].resolution),
         buf1, 10);
    draw_knob(2, "LEN", buf1);
  }
#endif

  // draw SCALE
  m_strncpy_p(buf1, scale_names[ptc_param_scale.getValue()], 4);
  draw_knob(3, "SCA", buf1);

  // draw TI keyboard
  mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, note_mask);

  SeqPage::display();
  oled_display.display();
  oled_display.setFont(oldfont);
}
#endif

uint8_t SeqPtcPage::calc_pitch(uint8_t note_num) {
  uint8_t size = scales[ptc_param_scale.cur]->size;
  uint8_t oct = note_num / size;
  note_num = note_num - oct * size;

  return scales[ptc_param_scale.cur]->pitches[note_num] + oct * 12;
}

uint8_t SeqPtcPage::get_next_voice(uint8_t pitch) {
  uint8_t voice = 255;
  uint8_t count = 0;

  if (poly_max == 0 || (!IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track) &&
                        (mcl_cfg.uart2_ctrl_mode == 0))) {
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

  uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[pitch]) +
                          ptc_param_finetune.getValue() - 32;
  return machine_pitch;
}

void SeqPtcPage::trig_md(uint8_t pitch) {
  pitch = ptc_param_oct.cur * 12 + pitch;
  uint8_t next_track = get_next_voice(pitch);
  uint8_t machine_pitch = get_machine_pitch(next_track, pitch);
  MD.setTrackParam(next_track, 0, machine_pitch);
  if (!BUTTON_DOWN(Buttons.BUTTON2)) {
    MD.triggerTrack(next_track, 127);
  }
  if ((recording) && (MidiClock.state == 2)) {

    if (!BUTTON_DOWN(Buttons.BUTTON2)) {
      mcl_seq.md_tracks[next_track].record_track(127);
    }
    mcl_seq.md_tracks[next_track].record_track_pitch(machine_pitch);
  }
}

void SeqPtcPage::clear_trig_fromext(uint8_t note_num) {
  uint8_t pitch = seq_ext_pitch(note_num - 32);
  CLEAR_BIT64(note_mask, pitch);
  render_arp();
}

void SeqPtcPage::trig_md_fromext(uint8_t note_num) {
  uint8_t pitch = seq_ext_pitch(note_num - 32);
  uint8_t next_track = get_next_voice(pitch);
  uint8_t machine_pitch = get_machine_pitch(next_track, pitch);
  SET_BIT64(note_mask, pitch);
  render_arp();
  MD.setTrackParam(next_track, 0, machine_pitch);
  if (!BUTTON_DOWN(Buttons.BUTTON2)) {
    MD.triggerTrack(next_track, 127);
  }
  if ((recording) && (MidiClock.state == 2)) {
    if (!BUTTON_DOWN(Buttons.BUTTON2)) {
      mcl_seq.md_tracks[next_track].record_track(127);
    }
    mcl_seq.md_tracks[next_track].record_track_pitch(machine_pitch);
  }
}

void SeqPtcPage::queue_redraw() { deferred_timer = slowclock; }

void SeqPtcPage::setup_arp() {
  if (arp_enabled) {
    return;
  }
  arp_enabled = true;
  arp_len = 0;
  arp_idx = 0;
  render_arp();
  MidiClock.addOn16Callback(
      this, (midi_clock_callback_ptr_t)&SeqPtcPage::on_16_callback);
}

void SeqPtcPage::remove_arp() {
  if (!arp_enabled) {
    return;
  }
  arp_enabled = false;
  MidiClock.removeOn16Callback(
      this, (midi_clock_callback_ptr_t)&SeqPtcPage::on_16_callback);
}

uint8_t SeqPtcPage::arp_get_next_note_down(uint8_t cur) {}
#define NOTE_RANGE 24

uint8_t SeqPtcPage::arp_get_next_note_up(int8_t cur) {

  for (int8_t i = cur + 1; i < NOTE_RANGE; i++) {
    if (IS_BIT_SET32(note_mask, i)) {
      return i;
    }
  }
  return 255;
}

void SeqPtcPage::render_arp() {
  DEBUG_PRINT_FN();
  if (!arp_enabled) {
    return;
  }
  arp_len = 0;

  uint8_t num_of_notes;
  uint8_t note = 0;
  uint8_t b = 0;

  uint8_t sort_up[NOTE_RANGE];
  uint8_t sort_down[NOTE_RANGE];

  note = arp_get_next_note_up(-1);
  if (note != 255) {
    num_of_notes++;
    sort_up[0] = note;
  } else {
    return;
  }

  // Collect notes, sort in ascending order
  DEBUG_PRINTLN("collecting notes");
  for (uint8_t i = 1; i < NOTE_RANGE && note != 255; i++) {
    note = arp_get_next_note_up(sort_up[i - 1]);
    if (note != 255) {
      num_of_notes++;
      sort_up[i] = note;
      DEBUG_PRINTLN(i);
    }
  }
  DEBUG_PRINTLN("finish");
  DEBUG_PRINTLN(num_of_notes);
  if (num_of_notes == 0) {
    return;
  }
  // Sort notes in descending order

  for (uint8_t i = 0; i < num_of_notes; i++) {
    sort_down[num_of_notes - i - 1] = sort_up[i];
  }
  note = 255;

  // Generate 1 itteration of the arp
  switch (arp_mode.cur) {
  case ARP_RND:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[random(0, num_of_notes)];
      arp_notes[arp_len++] = calc_pitch(note) + 12 * random(0,arp_oct.cur);
    }
    break;

  case ARP_UP2:
  case ARP_UPP:
  case ARP_UP:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_DOWN2:
  case ARP_DOWNP:
  case ARP_DOWN:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    break;

  case ARP_UPDOWN:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    for (uint8_t i = 1; i < num_of_notes - 1; i++) {
      note = sort_down[i];
      arp_notes[arp_len] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_DOWNUP:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    for (uint8_t i = 1; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      arp_notes[arp_len] = calc_pitch(note);
      arp_len++;
    }

    break;
  case ARP_UPNDOWN:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[arp_len] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_DOWNNUP:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[arp_len] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_CONV:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_DIV:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_up[b];
        b++;
      } else {
        note = sort_down[b];
      }
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_CONVDIV:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    b = 0;
    for (uint8_t i = 1; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      arp_notes[i] = calc_pitch(note);
      arp_len++;
    }
    break;
  case ARP_PINKUP:
    if (num_of_notes == 1) {
      note = sort_up[0];
      arp_notes[arp_len++] = calc_pitch(note);
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      arp_notes[arp_len++] = calc_pitch(note);
      note = sort_down[0];
      arp_notes[arp_len++] = calc_pitch(note);
    }

    break;

  case ARP_PINKDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[arp_len++] = calc_pitch(note);
      note = sort_down[0];
      arp_notes[arp_len++] = calc_pitch(note);
    }
    break;

  case ARP_THUMBUP:
    if (num_of_notes == 1) {
      note = sort_down[0];
      arp_notes[arp_len++] = calc_pitch(note);
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      arp_notes[arp_len++] = calc_pitch(note);
      note = sort_down[0];
      arp_notes[arp_len++] = calc_pitch(note);
    }

    break;

  case ARP_THUMBDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[arp_len++] = calc_pitch(note);
      note = sort_down[0];
      arp_notes[arp_len++] = calc_pitch(note);
    }
    break;
  }

  // Generate subsequent octave itterations
  if (arp_mode.cur <= ARP_THUMBDOWN) {
    for (uint8_t n = 0; n < arp_oct.cur; n++) {
      for (uint8_t i = 0; i < num_of_notes && arp_len < ARP_MAX_NOTES; i++) {
        arp_notes[arp_len] = arp_notes[i] + (n + 1) * 12;
        arp_len++;
      }
    }
  } else {
    switch (arp_mode.cur) {
    case ARP_UPP:
    case ARP_DOWNP:
      for (uint8_t n = 0; n < arp_oct.cur; n++) {
        for (uint8_t i = 0; i < num_of_notes && arp_len < ARP_MAX_NOTES; i++) {
          arp_notes[arp_len] = arp_notes[i];
          if (i == num_of_notes - 1) {
            arp_notes[arp_len] += (n + 1) * 12;
          }
          arp_len++;
        }
      }
      break;
    case ARP_UP2:
    case ARP_DOWN2:
      for (uint8_t n = 0; n < arp_oct.cur; n++) {
        for (uint8_t i = 0; i < num_of_notes && arp_len < ARP_MAX_NOTES; i++) {
          arp_notes[arp_len] = arp_notes[i];
          if (!(i & 1)) {
            arp_notes[arp_len] += (n + 1) * 12;
          }
          arp_len++;
        }
      }
      break;
    }
  }
  if (arp_idx >= arp_len) {
    arp_idx = arp_len - 1;
  }
}
/*
void SeqPtcPage::render_arp2() {
  if (!arp_enabled) {
    return;
  }
  if (arp_mode.cur == ARP_RND) {
    arp_len = 1;
    return;
  }

  uint8_t first_note = 255;
  uint8_t last_note = 255;
  arp_len = 0;
  for (uint8_t i = 0; i < ARP_MAX_NOTES; i++) {
    if (IS_BIT_SET32(note_mask, i)) {
      last_note = i;
      if (first_note == 255) {
        first_note = i;
      }
    }
  }
  if (first_note == 255) {
    return;
  }
  uint8_t idx = first_note;
  uint8_t idx2 = last_note;
  uint8_t note = 255;

  uint8_t pitch = 255;
  uint8_t first_pitch = calc_pitch(first_note);

  while (((pitch != first_pitch) || (arp_len == 1)) &&
         arp_len < ARP_MAX_NOTES) {
    bool ignore_base = false;

    switch (arp_mode.cur) {
    case ARP_UP:
      if (arp_len == 0) {
        note = first_note;
      } else {
        note = arp_get_next_note_up(idx);
        if (note <= idx) {
          if (arp_base < arp_oct.cur) {
            arp_base++;
          } else {
            arp_base = 0;
          }
        }
      }
      break;
    case ARP_DOWN:
      note = arp_get_next_note_down(idx);
      if (note >= idx) {
        if (arp_base > 0) {
          arp_base--;
        } else {
          arp_base = arp_oct.cur;
        }
      }
      break;
    case ARP_CONV:
      if (arp_dir == 0) {
        uint8_t note_tmp = arp_get_next_note_up(idx);
        if (note_tmp < note) {
          if (arp_base < arp_oct.cur) {
            arp_base++;
          } else {
            arp_base = 0;
          }
        }
        note = note_tmp;
        arp_dir = 1;
      } else {
        note = arp_get_next_note_down(idx2);
        idx2 = note;
        arp_dir = 0;
      }
      break;
    case ARP_CIRC:
      uint8_t note_tmp;
      if (arp_dir == 0) {
        note_tmp = arp_get_next_note_up(idx);
        if (note_tmp > idx) {
          note = note_tmp;
        } else {
          if (arp_base < arp_oct.cur) {
            arp_base++;
            note = note_tmp;
          } else {
            arp_dir = 1;
            note = arp_get_next_note_down(idx);
          }
        }
      } else if (arp_dir == 1) {
        note_tmp = arp_get_next_note_down(idx);
        if (note_tmp < idx) {
          note = note_tmp;
        } else {
          if (arp_base > 0) {
            arp_base--;
            note = note_tmp;
          } else {
            arp_dir = 0;
            note = arp_get_next_note_up(idx);
          }
        }
      }
      break;
    case ARP_UPTHUMB:
      note = arp_get_next_note_up(idx);
      note_tmp = arp_get_next_note_up(idx);
      ignore_base = true;
      if (note_tmp < idx) {
        ignore_base = false;
      }
      if (note < idx) {
        if (arp_base < arp_oct.cur) {
          arp_base++;
        } else {
          arp_base = 0;
        }
      }
      break;
    case ARP_DOWNPINK:
      note = arp_get_next_note_down(idx);
      note_tmp = arp_get_next_note_down(idx);
      ignore_base = true;
      if (note_tmp > idx) {
        ignore_base = false;
      }
      if (note > idx) {
        if (arp_base > 0) {
          arp_base--;
        } else {
          arp_base = arp_oct.cur;
        }
      }
      break;
    }
    if (note < NUM_MD_TRACKS) {
      idx = note;
      arp_notes[arp_len] = calc_pitch(note);
      if (!ignore_base) {
        arp_notes[arp_len] += arp_base * 12;
        pitch = arp_notes[arp_len];
      } else {
        pitch = arp_notes[arp_len] + arp_base * 12;
      }
      arp_len++;
    } else {
      break;
    }
  }
  if (arp_idx >= arp_len) {
    arp_idx = arp_len - 1;
  }
}
*/
void SeqPtcPage::on_16_callback() {
  bool trig = false;
  uint8_t note;

  switch (arp_speed.cur) {
  case 0:
    trig = true;
    break;
  case 1:
    if ((arp_count == 0) || (arp_count == 2) || (arp_count == 4) ||
        (arp_count == 6)) {
      trig = true;
    }
    break;
  case 2:
    if ((arp_count == 0) || (arp_count == 4)) {
      trig = true;
    }
    break;
  case 3:
    if (arp_count == 0) {
      trig = true;
    }
    break;
  }

  bool ignore_base = false;

  if (trig == true) {
    if (arp_len > 0) {
      trig_md(arp_notes[arp_idx]);
      arp_idx++;
      if (arp_idx == arp_len) {
        arp_idx = 0;
      }
    }
  }

  arp_count++;
  if (arp_count > 7) {
    arp_count = 0;
  }
}

void SeqPtcPage::recalc_notemask() {
  for (uint8_t i = 0; i < 24; i++) {
    if (note_interface.notes[i] == 1) {
      uint8_t pitch = calc_pitch(i);
      SET_BIT64(note_mask, pitch);
    }
  }
}

bool SeqPtcPage::handleEvent(gui_event_t *event) {

  if (SeqPage::handleEvent(event)) {
    if (show_seq_menu) {
      redisplay = true;
      return true;
    }
    queue_redraw();
  }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    // do not route EXT TI events to MD.
    if (device != DEVICE_MD) {
      return false;
    }

    uint8_t note = event->source - 128;
    uint8_t pitch = calc_pitch(note);
    DEBUG_PRINTLN("yep");
    // note interface presses are treated as musical notes here
    if (mask == EVENT_BUTTON_PRESSED) {

      SET_BIT64(note_mask, pitch);
      render_arp();
      if (midi_device != DEVICE_MD) {
        midi_device = device;
        config();
      } else {
        config_encoders();
      }
      midi_device = device;

      if ((!arp_enabled) || (MidiClock.state != 2)) {
        trig_md(pitch);
      }
    } else if (mask == EVENT_BUTTON_RELEASED) {
      if (arp_und.cur != ARP_LATCH) {
      CLEAR_BIT64(note_mask, pitch);
      render_arp();
      }
    }

    // deferred trigger redraw to update TI keyboard feedback.
    queue_redraw();

    return true;
  } // TI events

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    if (BUTTON_DOWN(Buttons.BUTTON4)) {
      re_init = true;
      GUI.pushPage(&poly_page);
      return true;
    }
    seq_ptc_page.queue_redraw();
    recording = !recording;
    return true;
  }
  /*
    if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
      GUI.setPage(&grid_page);
      return true;
    }
  */

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    if (BUTTON_DOWN(Buttons.BUTTON1)) {
      GUI.pushPage(&poly_page);
      return true;
    }
    if (midi_device == DEVICE_MD) {

      if (poly_count > 1) {
#ifdef OLED_DISPLAY
        oled_display.textbox("CLEAR ", "POLY TRACKS");
#endif
        for (uint8_t c = 0; c < 16; c++) {
          if (IS_BIT_SET16(mcl_cfg.poly_mask, c)) {
            mcl_seq.md_tracks[c].clear_track();
          }
        }
      } else {
#ifdef OLED_DISPLAY
        oled_display.textbox("CLEAR ", "TRACK");
#endif
        mcl_seq.md_tracks[last_md_track].clear_track();
      }
    }
#ifdef EXT_TRACKS
    else {
#ifdef OLED_DISPLAY
      oled_display.textbox("CLEAR ", "EXT TRACK");
#endif
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
#endif
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
  // if (pos >= scales[seq_param5.cur]->size) {
  oct += pos / scales[ptc_param_scale.cur]->size;
  pos = pos - scales[ptc_param_scale.cur]->size *
                  (pos / scales[ptc_param_scale.cur]->size);
  // }

  //  if (seq_param5.getValue() > 0) {
  pitch = ptc_param_oct.cur * 12 + scales[ptc_param_scale.cur]->pitches[pos] +
          oct * 12;
  //   }

  return pitch;
}

void SeqPtcMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  if ((GUI.currentPage() == &seq_step_page) ||
#ifdef EXT_TRACKS
      (GUI.currentPage() == &seq_extstep_page) ||
#endif
      (GUI.currentPage() == &grid_save_page) ||
      (GUI.currentPage() == &grid_write_page)) {
    return;
  }

  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  DEBUG_PRINT("note on midi2: ");
  DEBUG_DUMP(channel);

  // matches control channel, or MIDI2 is OMNI?
  // then route midi message to MD
  uint8_t pitch = seq_ptc_page.calc_pitch(note_num);

  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode == MIDI_OMNI_MODE)) {
    seq_ptc_page.trig_md_fromext(pitch);
    SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);
    seq_ptc_page.queue_redraw();
    return;
  }

  uint8_t scaled_pitch = pitch - (pitch / 24) * 24;
  SET_BIT64(seq_ptc_page.note_mask, scaled_pitch);
  seq_ptc_page.render_arp();
#ifdef EXT_TRACKS
  // otherwise, translate the message and send it back to MIDI2.
  if (SeqPage::midi_device != midi_active_peering.get_device(UART2_PORT) ||
      (last_ext_track != channel)) {

    SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
    last_ext_track = channel;
    seq_ptc_page.config();
  }
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
  if (channel >= mcl_seq.num_ext_tracks) {
    return;
  }
  last_ext_track = channel;
  seq_ptc_page.config_encoders();

  DEBUG_PRINTLN(mcl_seq.ext_tracks[channel].length);
  MidiUart2.sendNoteOn(channel, pitch, msg[2]);
  if ((seq_ptc_page.recording) && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[channel].record_ext_track_noteon(pitch, msg[2]);
  }
  seq_ptc_page.queue_redraw();
#endif
}

void SeqPtcMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  if ((GUI.currentPage() == &seq_step_page) ||
#ifdef EXT_TRACKS
      (GUI.currentPage() == &seq_extstep_page) ||
#endif
      (GUI.currentPage() == &grid_save_page) ||
      (GUI.currentPage() == &grid_write_page)) {
    return;
  }

  DEBUG_PRINTLN("note off midi2");
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t pitch = seq_ptc_page.calc_pitch(note_num);
  uint8_t scaled_pitch = pitch - (pitch / 24) * 24;
  if (arp_und.cur != ARP_LATCH) {
  CLEAR_BIT64(seq_ptc_page.note_mask, scaled_pitch);
  seq_ptc_page.render_arp();
  }
  if ((mcl_cfg.uart2_ctrl_mode - 1 == channel) ||
      (mcl_cfg.uart2_ctrl_mode == MIDI_OMNI_MODE)) {
    seq_ptc_page.clear_trig_fromext(pitch);
    seq_ptc_page.queue_redraw();
    return;
  }
#ifdef EXT_TRACKS
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
  if (channel >= mcl_seq.num_ext_tracks) {
    return;
  }
  last_ext_track = channel;
  seq_ptc_page.config_encoders();

  MidiUart2.sendNoteOff(channel, pitch, msg[2]);
  if (seq_ptc_page.recording && (MidiClock.state == 2)) {
    mcl_seq.ext_tracks[channel].record_ext_track_noteoff(pitch, msg[2]);
  }
  seq_ptc_page.queue_redraw();
#endif
}

void SeqPtcMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks
  MD.parseCC(channel, param, &track, &track_param);
  uint8_t start_track;

  if ((seq_ptc_page.poly_max > 1)) {
    if (IS_BIT_SET16(mcl_cfg.poly_mask, track)) {

      for (uint8_t n = 0; n < 16; n++) {

        if (IS_BIT_SET16(mcl_cfg.poly_mask, n) && (n != track)) {
          if (track_param < 24) {
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
