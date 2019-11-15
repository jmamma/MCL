#include "MCL.h"
#include "SeqRlckPage.h"

void SeqRlckPage::setup() { SeqPage::setup(); }

void SeqRlckPage::config() {
  // config info labels
  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

  constexpr uint8_t len1 = sizeof(info1);

  char buf[len1] = {'\0'};
  m_strncpy_p(buf, str1, len1);
  strncpy(info1, buf, len1);
  strncat(info1, ">", len1);
  m_strncpy_p(buf, str2, len1);
  strncat(info1, buf, len1);

  strcpy(info2, "RLCK");
  display_page_index = false;

  // config menu
  config_as_lockedit();
}

void SeqRlckPage::init() {
  SeqPage::init();
  if (MidiClock.state == 2) {
    MD.midi_events.disable_live_kit_update();
  }
  md_exploit.off();
  note_interface.state = false;
  recording = true;
  config();

  seq_param1.max = 4;
  seq_param2.max = 64;
  seq_param3.max = 64;
  seq_param4.max = 11;
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].length;

  curpage = SEQ_RTRK_PAGE;
  midi_events.setup_callbacks();
}

void SeqRlckPage::cleanup() {
  SeqPage::cleanup();
  if (MidiClock.state == 2) {
    MD.midi_events.enable_live_kit_update();
  }
  midi_events.remove_callbacks();
}

#ifndef OLED_DISPLAY
void SeqRlckPage::display() {
  if ((!redisplay) && (MidiClock.state == 2)) {
    return;
  }
  GUI.setLine(GUI.LINE1);
  GUI.put_value_at1(15, page_select + 1);

  GUI.put_string_at(0, "RLCK");

  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);
  if (SeqPage::midi_device == DEVICE_MD) {
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
    GUI.put_value_at(5, seq_param3.getValue());
  }
#ifdef EXT_TRACKS
  else {
    GUI.put_value_at(5, (seq_param3.getValue() /
                         (2 / mcl_seq.ext_tracks[last_ext_track].resolution)));
    if (Analog4.connected) {
      GUI.put_string_at(9, "A4T");
    } else {
      GUI.put_string_at(9, "MID");
    }
    GUI.put_value_at1(12, last_ext_track + 1);
  }
#endif
  bool show_current_step = false;
  draw_lock_mask(page_select * 16, show_current_step);
  SeqPage::display();
}
#else
void SeqRlckPage::display() {
  if ((!redisplay) && (MidiClock.state == 2)) {
    return;
  }

  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();

  draw_knob_frame();

  uint8_t len = seq_param3.getValue();
#ifdef EXT_TRACKS
  if (SeqPage::midi_device != DEVICE_MD) {
    len = len / (2 / mcl_seq.ext_tracks[last_ext_track].resolution);
  }
#endif

  char K[4];
  itoa(len, K, 10);
  draw_knob(2, "LEN", K);

  bool show_current_step = false;
  draw_lock_mask(page_select * 16, show_current_step);
  draw_pattern_mask(page_select * 16, DEVICE_MD, show_current_step);

  SeqPage::display();
  oled_display.display();
  oled_display.setFont(oldfont);
}
#endif

bool SeqRlckPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }
  redisplay = true;
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    curpage = SEQ_RTRK_PAGE;
    GUI.setPage(&seq_rtrk_page);
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    if (MD.getCurrentTrack(CALLBACK_TIMEOUT) != last_md_track) {
     for (uint8_t c = 0; c < 4; c++) {
      if (mcl_seq.md_tracks[last_md_track].locks_params[c] > 0) {
        last_param_id = mcl_seq.md_tracks[last_md_track].locks_params[c] - 1;
      }
    }
    last_md_track = MD.currentTrack;
    }
    mcl_seq.md_tracks[last_md_track].clear_param_locks(last_param_id);
    for (uint8_t c = 0; c < 4; c++) {
      if (mcl_seq.md_tracks[last_md_track].locks_params[c] > 0) {
        last_param_id = mcl_seq.md_tracks[last_md_track].locks_params[c] - 1;
      }
    }
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}

void SeqRlckPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);

  if (MidiClock.state != 2) {
    return;
  }

  last_md_track = track;
  //ignore level
  if (track_param > 31) { return; }
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].length;

  mcl_seq.md_tracks[track].update_param(track_param, value);

  MD.kit.params[track][track_param] = value;
  mcl_seq.md_tracks[track].record_track_locks(track_param, value);
  seq_rlck_page.last_param_id = track_param;
}

void SeqRlckPageMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {}

void SeqRlckPageMidiEvents::onMidiStopCallback() {
  MD.midi_events.enable_live_kit_update();
}

void SeqRlckPageMidiEvents::onMidiStartCallback() {
  MD.midi_events.disable_live_kit_update();
}

void SeqRlckPageMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnControlChangeCallback(this,
                                  (midi_callback_ptr_t)&SeqRlckPageMidiEvents::
                                      onControlChangeCallback_Midi);
  Midi2.addOnControlChangeCallback(this,
                                   (midi_callback_ptr_t)&SeqRlckPageMidiEvents::
                                       onControlChangeCallback_Midi2);

  MidiClock.addOnMidiStopCallback(
      this,
      (midi_clock_callback_ptr_t)&SeqRlckPageMidiEvents::onMidiStopCallback);
  MidiClock.addOnMidiStartCallback(
      this,
      (midi_clock_callback_ptr_t)&SeqRlckPageMidiEvents::onMidiStartCallback);
  MidiClock.addOnMidiContinueCallback(
      this,
      (midi_clock_callback_ptr_t)&SeqRlckPageMidiEvents::onMidiStartCallback);

  state = true;
}

void SeqRlckPageMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqRlckPageMidiEvents::
                onControlChangeCallback_Midi);
  Midi2.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqRlckPageMidiEvents::
                onControlChangeCallback_Midi2);
  MidiClock.removeOnMidiStopCallback(
      this,
      (midi_clock_callback_ptr_t)&SeqRlckPageMidiEvents::onMidiStopCallback);
  MidiClock.removeOnMidiStartCallback(
      this,
      (midi_clock_callback_ptr_t)&SeqRlckPageMidiEvents::onMidiStartCallback);
  MidiClock.removeOnMidiContinueCallback(
      this,
      (midi_clock_callback_ptr_t)&SeqRlckPageMidiEvents::onMidiStartCallback);

  state = false;
}
