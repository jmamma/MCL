#include "MCL_impl.h"

void SeqParamPage::setup() { SeqPage::setup(); }
void SeqParamPage::config() {
  // config info labels
  const char *str1 = getMDMachineNameShort(MD.kit.get_model(last_md_track), 1);
  const char *str2 = getMDMachineNameShort(MD.kit.get_model(last_md_track), 2);

  // 0-1
  copyMachineNameShort(str1, info1);
  info1[2] = '>';
  // 3-4
  copyMachineNameShort(str2, info1 + 3);
  // 5
  info1[5] = 0;

  strcpy(info2, "PARAM-");
  info2[6] = 'A' + page_id;
  info2[7] = 0;

  // config menu
  config_as_lockedit();
}

void SeqParamPage::init() {
  SeqPage::init();
  config();
  trig_interface.on();
  toggle_device = false;
  note_interface.state = true;
  SeqPage::mask_type = MASK_PATTERN;

  seq_param1.max = 24;
  seq_lock1.max = 127;
  seq_param3.max = 24;
  seq_lock2.max = 127;
  seq_param3.min = 0;
  seq_param3.handler = NULL;

  seq_param1.cur = mcl_seq.md_tracks[last_md_track].locks_params[p1];
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].locks_params[p2];
  seq_lock1.cur =
      MD.kit.params[last_md_track]
                   [mcl_seq.md_tracks[last_md_track].locks_params[p1]];
  seq_lock2.cur =
      MD.kit.params[last_md_track]
                   [mcl_seq.md_tracks[last_md_track].locks_params[p2]];
  // Prevent hasChanged from being called
  seq_param1.old = seq_param1.cur;
  seq_lock1.old = seq_lock1.cur;
  seq_param3.old = seq_param3.cur;
  seq_lock2.old = seq_lock2.cur;

  midi_events.setup_callbacks();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void SeqParamPage::construct(uint8_t p1_, uint8_t p2_) {
  p1 = p1_;
  p2 = p2_;
}

void SeqParamPage::cleanup() {
  SeqPage::cleanup();
  midi_events.remove_callbacks();
}

#ifndef OLED_DISPLAY
void SeqParamPage::display() {
  GUI.setLine(GUI.LINE1);
  char myName[4] = "-- ";
  char myName2[4] = "-- ";
  if (seq_param1.getValue() == 0) {
    GUI.put_string_at(0, "--");
  } else {
    const char *modelname = model_param_name(MD.kit.get_model(last_md_track),
                                             seq_param1.getValue() - 1);
    if (modelname != NULL) {
      strncpy(myName, modelname, 4);
    }
    GUI.put_string_at(0, myName);
  }
  if (seq_lock1.getValue() == 0) {
    GUI.put_string_at(4, "--");
  } else {
    GUI.put_value_at2(4, seq_lock1.getValue());
  }
  if (seq_param3.getValue() == 0) {
    GUI.put_string_at(7, "--");
  } else {
    const char *modelname = model_param_name(MD.kit.get_model(last_md_track),
                                             seq_param1.getValue() - 1);
    if (modelname != NULL) {
      strncpy(myName2, modelname, 4);
    }
    GUI.put_string_at(7, myName2);
  }
  if (seq_lock2.getValue() == 0) {
    GUI.put_string_at(11, "--");
  } else {
    GUI.put_value_at2(11, seq_lock2.getValue());
  }
  if (page_id == 0) {
    GUI.put_string_at(14, "A");
  }
  if (page_id == 1) {
    GUI.put_string_at(14, "B");
  }
  GUI.put_value_at1(15, (page_select + 1));
  draw_lock_mask(page_select * 16);
  SeqPage::display();
}
#else
void SeqParamPage::display() {
  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();

  draw_knob_frame();

  char myName[4] = "-- ";
  char myName2[4] = "-- ";

  if (seq_param1.getValue() != 0) {
    const char *modelname = model_param_name(MD.kit.get_model(last_md_track),
                                             seq_param1.getValue() - 1);
    if (modelname != NULL) {
      strncpy(myName, modelname, 4);
    }
  }

  if (seq_param3.getValue() != 0) {
    const char *modelname = model_param_name(MD.kit.get_model(last_md_track),
                                             seq_param3.getValue() - 1);
    if (modelname != NULL) {
      strncpy(myName2, modelname, 4);
    }
  }

  draw_knob(0, "TGT", myName);
  draw_knob(2, "TGT", myName2);

  draw_knob(1, &seq_lock1, "VAL");
  draw_knob(3, &seq_lock2, "VAL");
  draw_mask(page_select * 16, DEVICE_MD);
  draw_lock_mask(page_select * 16);

  SeqPage::display();
  oled_display.display();
  oled_display.setFont(oldfont);
}

#endif
void SeqParamPage::loop() {

  if (seq_param1.hasChanged() || seq_lock1.hasChanged() ||
      seq_param3.hasChanged() || seq_lock2.hasChanged()) {
    for (uint8_t n = 0; n < 16; n++) {

      if (note_interface.notes[n] == 1) {
        uint8_t step = n + (page_select * 16);
        auto &active_track = mcl_seq.md_tracks[last_md_track];
        int8_t utiming = active_track.timing[step]; // upper

        // Fudge timing info if it's not there
        if (utiming == 0) {
          utiming = 12;
          mcl_seq.md_tracks[last_md_track].timing[step] = utiming;
        }

        active_track.set_track_locks_i(step, p1, seq_lock1.cur);
        active_track.set_track_locks_i(step, p2, seq_lock2.cur);
      }
    }
    if (seq_param1.hasChanged() || seq_param3.hasChanged()) {
      mcl_seq.md_tracks[last_md_track].reset_params();
      mcl_seq.md_tracks[last_md_track].locks_params[p1] = seq_param1.cur;
      mcl_seq.md_tracks[last_md_track].locks_params[p2] = seq_param3.cur;
      mcl_seq.md_tracks[last_md_track].update_params();
    }
  }

  SeqPage::loop();
}

bool SeqParamPage::handleEvent(gui_event_t *event) {

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;

    uint8_t track = event->source - 128;
    if (device == DEVICE_A4) {
      return true;
    }
    auto &active_track = mcl_seq.md_tracks[last_md_track];
    uint8_t step = track + (page_select * 16);

    step_select = track;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      seq_param1.cur = mcl_seq.md_tracks[last_md_track].locks_params[p1];
      seq_param3.cur = mcl_seq.md_tracks[last_md_track].locks_params[p2];

      if (!active_track.steps[step].locks_enabled) {
        // ignore next track event to prevent it being cleared on release
        note_interface.ignoreNextEvent(track);
        // enable here so we can access the lock values below
        active_track.enable_step_locks(step);
      }

      // get the values. if no values, load up the orig.
      seq_lock1.cur = active_track.get_track_lock(step, p1);
      seq_lock2.cur = active_track.get_track_lock(step, p2);

      active_track.set_track_locks_i(step, p1, seq_lock1.cur);
      active_track.set_track_locks_i(step, p2, seq_lock2.cur);
    } else if (event->mask == EVENT_BUTTON_RELEASED) {
      if (device == DEVICE_A4) {
        return true;
      }

      if (active_track.steps[step].locks_enabled) {
        if (clock_diff(note_interface.note_hold[port], slowclock) <
            TRIG_HOLD_TIME) {
          active_track.disable_step_locks(step);
        }
      }
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    uint8_t page_depth = page_id;
    if (page_depth < NUM_PARAM_PAGES - 1) {
      page_depth = page_depth + 1;
    } else {
      page_depth = 0;
    }

    GUI.setPage(&seq_param_page[page_depth]);
    return true;
  }

  return false;
}

void SeqParamPageMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {}

void SeqParamPageMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {}
void SeqParamPageMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  Midi2.addOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOffCallback_Midi2);

  state = true;
}

void SeqParamPageMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }

  Midi2.removeOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqParamPageMidiEvents::onNoteOffCallback_Midi2);

  state = false;
}
