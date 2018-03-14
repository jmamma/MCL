#include "SeqPage.h"

bool SeqPage::handleEvent(gui_event_t *event) {
  //  if (note_interface.is_event(event)) {

  //  return true;
  //  }
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

  if (EVENT_PRESSED(evt, Buttons.BUTTON2) ) {
    uint8_t pagemax = 4;
    seq_page_select += 1;

    if (cur_col > 15) {
      pagemax = 8;

    }
    if (seq_page_select >= pagemax) {
      seq_page_select = 0;
    }

    return true;

  }

  return false;
}
void SeqPage::setup() {
  trackinfo_param2.min = 0;
  if (curpage == 0) {
    create_chars_seq();
    currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
    // curpage = page;

    // Don't save kit if sequencer is running, otherwise parameter locks will be
    // stored.
    if (MidiClock.state != 2) {
      MD.saveCurrentKit(currentkit_temp);
    }

    MD.getBlockingKit(currentkit_temp);
    MD.getCurrentTrack(CALLBACK_TIMEOUT);

    md_exploit.on();
  } else {
    md_exploit.init_notes();
  }
  cur_col = last_md_track;
  cur_row = param2.getValue();
}

void SeqPageMidiEvents::setup_callbacks() {
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback);
}

void SeqPageMidiEvents::remove_callbacks() {
  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqPageMidiEvents::onControlChangeCallback);
}

void SeqPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  if (md_exploit.off()) {
    GUI.setPage(&page);
    curpage = 0;
  }
}
