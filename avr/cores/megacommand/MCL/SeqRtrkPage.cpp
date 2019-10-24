#include "MCL.h"
#include "SeqRtrkPage.h"

void SeqRtrkPage::setup() { SeqPage::setup(); }

void SeqRtrkPage::config() {

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

  strcpy(info2, "RTRK");
}

void SeqRtrkPage::init() {
  SeqPage::init();

  note_interface.state = true;

  ((MCLEncoder *)encoders[0])->max = 4;
  ((MCLEncoder *)encoders[1])->max = 64;
  ((MCLEncoder *)encoders[2])->max = 64;
  ((MCLEncoder *)encoders[3])->max = 11;
  encoders[2]->cur = mcl_seq.md_tracks[last_md_track].length;
  midi_device = DEVICE_MD;
  curpage = SEQ_RTRK_PAGE;
  recording = true;
  config();
  md_exploit.on();
}

void SeqRtrkPage::cleanup() {
  SeqPage::cleanup();
}

#ifndef OLED_DISPLAY
void SeqRtrkPage::display() {
  if ((!redisplay) && (MidiClock.state == 2)) { return; }
  GUI.setLine(GUI.LINE1);
  GUI.put_value_at1(15, page_select + 1);

  GUI.put_string_at(0, "RTRK");

  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);
  if (SeqPage::midi_device == DEVICE_MD) {
    GUI.put_p_string_at(9, str1);
    GUI.put_p_string_at(11, str2);
    GUI.put_value_at(5, encoders[2]->getValue());
  }
#ifdef EXT_TRACKS
  else {
    GUI.put_value_at(5, (encoders[2]->getValue() /
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
  draw_pattern_mask(page_select * 16, DEVICE_MD, show_current_step);
  SeqPage::display();
}
#else
void SeqRtrkPage::display() {
  if ((!redisplay) && (MidiClock.state == 2)) { return; }
  SeqPage::display();

  bool show_current_step = false;
  draw_lock_mask(page_select * 16, show_current_step);
  draw_pattern_mask(page_select * 16, DEVICE_MD, show_current_step);

  oled_display.display();
}
#endif

bool SeqRtrkPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    midi_device = device;
    if (BUTTON_DOWN(Buttons.BUTTON2) || BUTTON_DOWN(Buttons.BUTTON3)) { return true; }
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device != DEVICE_MD) { return true; }
      last_md_track = track;

      encoders[2]->cur = mcl_seq.md_tracks[last_md_track].length;
      MD.triggerTrack(track, 127);
      if ((MidiClock.state == 2)) {
        mcl_seq.md_tracks[last_md_track].record_track(track, 127);

        return true;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
    }
    return true;
  }
  redisplay = true;
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    md_exploit.off();
    GUI.setPage(&seq_rlck_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      mcl_seq.md_tracks[n].clear_track();
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    if (SeqPage::midi_device == DEVICE_MD) {
      mcl_seq.md_tracks[last_md_track].clear_track();
    }
#ifdef EXT_TRACKS
    else {
      mcl_seq.ext_tracks[last_ext_track].clear_track();
    }
#endif
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
