#include "SeqRlckPage.h"

void SeqRlckPage::setup() {
  SeqPage::setup();
}


void SeqRlckPage::init() {
  collect_trigs = false;

  encoders[1]->max = 4;
  encoders[2]->max = 64;
  encoders[3]->max = 64;
  encoders[4]->max = 11;
  encoders[3]->cur = PatternLengths[last_md_track];

  curpage = SEQ_RTRK_PAGE;
  midi_events.setup_callbacks();
}
void SeqRlckPage::cleanup() {
  midi_events.remove_callbacks();
}
bool SeqRlckPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_value_at1(15, seq_page.page_select + 1);

  GUI.put_string_at(0, "RLCK");

  const char *str1 = getMachineNameShort(MD.kit.models[grid.cur_col], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[grid.cur_col], 2);
  if (grid.cur_col < 16) {
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
    GUI.put_value_at(5, encoders[3]->getValue());
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

  draw_lockmask(seq_page.page_select * 16);
}
bool SeqRlckPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    md_exploit.on();
    collect_trigs = false;
    curpage = SEQ_RTRK_PAGE;
    GUI.setPage(&seq_rtrk_page);
    return true;
  }

  if ((EVENT_PRESSED(evnt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(evnt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_locks(n);
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_seq_locks(grid.cur_col);
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}

void SeqRlckPageCallbacks::onControlChangeCallbackMidi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
      uint8_t param = msg[1];
      uint8_t value = msg[2];
      uint8_t track;
      uint8_t track_param;
      uint8_t param_true = 0;
      if (param >= 16) {
        param_true = 1;
      }
      if (param < 63) {
        param = param - 16;
        track = (param / 24) + (channel - MD.global.baseChannel) * 4;
        track_param = param - ((param / 24) * 24);
      }
      else if (param >= 72) {
        param = param - 72;
        track = (param / 24) + 2 + (channel - MD.global.baseChannel) * 4;
        track_param = param - ((param / 24) * 24);
      }

      if (param_true) {

        MD.kit.params[track][track_param] = value;
      }
  cur_col = track;
  last_md_track = track;
  encoders[3]->cur = PatternLengths[cur_col];
      mcl_seq.rec_track_locks(track, track_param, value);
}

void SeqRlckPageCallbacks::onControlChangeCallbackMidi2(uint8_t *msg) {


}

void SeqRlckPageMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
 Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqRlckPageMidiEvents::onControlChangeCallback);
  Midi2.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqRlckPageMidiEvents::onControlChangeCallbackMidi2);


  state = true;
}

void SeqRlckPageMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
 Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqRlckPageMidiEvents::onControlChangeCallback);
  Midi2.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqRlckPageMidiEvents::onControlChangeCallbackMidi2);


  state = false;
}
