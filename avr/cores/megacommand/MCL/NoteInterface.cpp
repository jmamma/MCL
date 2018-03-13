/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "NoteInterface.h"

void NoteInterface::setup() {}

void NoteInterface::init_notes() {
  for (uint8_t i = 0; i < 20; i++) {
    notes[i] = 0;
    // notes_off[i] = 0;
  }
}

bool NoteInterface::is_event(event_t *event) {
  if (event->source >= 128) {
    return true;
  }
  return false;
}
void NoteInterface::note_on_event(uint8_t note_num) {
  if (!state) {
    return;
  }
  if (note_num > 20) {
    return;
  }
  if (notes[note_num] == 0) {
    notes[note_num] = 1;
  }

  gui_event_t event;
  event.source = note_num + 128;
  event.mask = EVENT_BUTTON_PRESSED;
  EventRB.putp(&event);

  if ((curpage == CUE_PAGE) && (trackinfo_param4.getValue() == 0)) {
    toggle_cue(note_num);
    md_exploit.send_globals();
  }

  if ((curpage == SEQ_STEP_PAGE) && (ice == DEVICE_A4)) {

    load_seq_extstep_page(channel);
    return;
  }
  if ((ice == DEVICE_A4) && (curpage == SEQ_EXTSTEP_PAGE)) {
    // curpage = SEQ_EXTSTEP_PAGE;
    load_seq_extstep_page(channel);
    return;
  }

  if ((curpage == SEQ_EXTSTEP_PAGE) && (ice == DEVICE_MD)) {
    note_hold = slow_clock;

    if ((note_num + (seq_page_select * 16)) >=
        ExtPatternLengths[last_extseq_track]) {
      notes[note_num] = 0;
      return;
    }

    int8_t utiming = Exttiming[last_extseq_track]
                              [(note_num + (seq_page_select * 16))]; // upper
    uint8_t condition =
        Extconditional[last_extseq_track]
                      [(note_num + (seq_page_select * 16))]; // lower
    trackinfo_param1.cur = condition;
    // Micro
    if (utiming == 0) {
      if (ExtPatternResolution[last_extseq_track] == 1) {
        utiming = 6;
        trackinfo_param2.max = 11;
      } else {
        trackinfo_param2.max = 23;
        utiming = 12;
      }
    }
    trackinfo_param2.cur = utiming;

    gui_last_trig_press = note_num;
  }

  if (curpage == SEQ_EUC_PAGE) {
  }
  if (curpage == SEQ_STEP_PAGE) {

    if ((note_num + (seq_page_select * 16)) >= PatternLengths[cur_col]) {
      notes[note_num] = 0;
      return;
    }

    trackinfo_param2.max = 23;
    note_hold = slow_clock;
    int8_t utiming =
        timing[cur_col][(note_num + (seq_page_select * 16))]; // upper
    uint8_t condition =
        conditional[cur_col][(note_num + (seq_page_select * 16))]; // lower

    // Cond
    trackinfo_param1.cur = condition;
    // Micro
    if (utiming == 0) {
      utiming = 12;
    }
    trackinfo_param2.cur = utiming;
  }

  else if ((curpage == SEQ_PARAM_A_PAGE) || (curpage == SEQ_PARAM_B_PAGE)) {
    note_hold = slow_clock;
    uint8_t param_offset;
    if (curpage == SEQ_PARAM_A_PAGE) {
      param_offset = 0;
    } else {
      param_offset = 2;
    }
    trackinfo_param1.cur = PatternLocksParams[last_md_track][param_offset];
    trackinfo_param3.cur = PatternLocksParams[last_md_track][param_offset + 1];

    trackinfo_param2.cur = PatternLocks[last_md_track][param_offset]
                                       [(note_num + (seq_page_select * 16))];
    trackinfo_param4.cur = PatternLocks[last_md_track][param_offset + 1]
                                       [(note_num + (seq_page_select * 16))];
    notes[note_num] = 1;

  } else {
    // draw_notes(0);
  }
}
void NoteInterface::note_off_event(uint8_t note_num) {
  if (!state) {
    return;
  }

  notes[note_num] = 3;

  gui_event_t event;
  event.source = i;
  event.mask = EVENT_BUTTON_RELEASED;
  EventRB.putp(&event);
}

uint8_t NoteInterface::note_to_track_map(uint8_t note) {
  uint8_t note_to_track_map[7] = {0, 2, 4, 5, 7, 9, 11};
  for (uint8_t i = 0; i < 7; i++) {
    if (note_to_track_map[i] == (note - (note / 12) * 12)) {
      return i + 16;
    }
  }
}
bool NoteInterface::notes_all_off() {
  bool all_notes_off = false;
  uint8_t a = 0;
  uint8_t b = 0;
  for (uint8_t i = 0; i < 20; i++) {
    if (notes[i] == 1) {
      a++;
    }
    if (notes[i] == 3) {
      b++;
    }
  }

  if ((a == 0) && (b > 0)) {
    all_notes_off = true;
  }
  return all_notes_off;
}

uint8_t NoteInterface::notes_count_off() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < 20; i++) {
    if (notes[i] == 3) {
      a++;
    }
  }
  return a;
}
uint8_t NoteInterface::notes_count() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < 20; i++) {
    if (notes[i] > 0) {
      a++;
    }
  }
  return a;
}

void NoteInterface::draw_notes(uint8_t line_number) {
  if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  } else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  /*Display 16 track cues on screen,
   For 16 tracks check to see if there is a cue*/
  for (int i = 0; i < 16; i++) {
    if (curpage == CUE_PAGE) {

      if (IS_BIT_SET32(cfg.cues, i)) {
        str[i] = 'X';
      }
    }
    if (notes[i] > 0) {
      /*If the bit is set, there is a cue at this position. We'd like to display
       * it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

      str[i] = (char)219;
    }
  }

  /*Display the cues*/
  GUI.put_string_at(0, str);
}

void NoteIntefaceMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  // only accept input if device is not a MD
  if (midi_active_peering.uart1_device != DEVICE_MD) {
  uint8_t note_num = note_to_track_map(msg[1]);
  note_on_event(note_num);
  }
}
void NoteIntefaceMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  uint8_t note_num = note_to_track_map(msg[1]);
  note_on_event(note_num);
}
void NoteIntefaceMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {
  // only accept input if device is not a MD
  if (midi_active_peering.uart1_device != DEVICE_MD) {
  uint8_t note_num = note_to_track_map(msg[1]);
  note_off_event(note_num);
  }
}
void NoteIntefaceMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  uint8_t note_num = note_to_track_map(msg[1]);
  note_off_event(note_num);
}


NoteInterface note_interface;
