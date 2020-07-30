#include "MCL.h"
#include "SeqExtStepPage.h"

void SeqExtStepPage::setup() { SeqPage::setup(); }
void SeqExtStepPage::config() {
#ifdef EXT_TRACKS
  seq_param3.cur = mcl_seq.ext_tracks[last_ext_track].length;
#endif
  // config info labels
  constexpr uint8_t len1 = sizeof(info1);

#ifdef EXT_TRACKS
/*
  if (mcl_seq.ext_tracks[last_ext_track].speed == EXT_SPEED_2X) {
    strcpy(info1, "HI-RES");
  } else {
    strcpy(info1, "LOW-RES");
  }
*/
#endif

  strcpy(info2, "EXT");

  // config menu
  config_as_trackedit();
}

void SeqExtStepPage::config_encoders() {
#ifdef EXT_TRACKS
  uint8_t timing_mid = mcl_seq.ext_tracks[last_ext_track].get_timing_mid();
  seq_param1.max = NUM_TRIG_CONDITIONS * 2;
  seq_param2.cur = timing_mid;
  seq_param2.old = timing_mid;
  seq_param2.max = timing_mid * 2 - 1;
  seq_param3.max = 128;
  config();
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
#endif
}

void SeqExtStepPage::init() {
  page_count = 8;
  DEBUG_PRINTLN("seq extstep init");
  curpage = SEQ_EXTSTEP_PAGE;
  trig_interface.on();
  note_interface.state = true;
  config_encoders();
  midi_events.setup_callbacks();
}

void SeqExtStepPage::cleanup() {
  SeqPage::cleanup();
  midi_events.remove_callbacks();
}


#define MAX_FOV_W 96

void SeqExtStepPage::draw_pianoroll() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];

  uint16_t ticks_per_step = active_track.get_speed_multiplier() * get_timing_mid();

  int16_t roll_width = active_track.length * ticks_per_step;
  uint8_t roll_height = 127; //127, Notes.

  uint8_t fov_x = 0

  uint8_t zoom = 16;

  uint8_t fov_w = min(zoom * ticks_per_step, MAX_FOV_W);
  uint8_t fov_y = 64;
  uint8_t fov_h = 8;

  uint8_t fov_start_step = fov_x / (ticks_per_step);
  uint8_t fov_end_step = fov_start_step + fov_w / ticks_per_step;


  for (int i = 0; i < active_track.length; i++) {
     for (uint8_t a = 0; a < NUM_EXT_NOTES; a++) {
        int8_t note = active_track.notes[a][i];
        //Check if note is visible with fov vertical range.
        if ((abs(note) >= fov_y) && (abs(note) < fov_y + fov_h)) {
        //Determine note start and end positions
           uint8_t match = 255;
           for (uint8_t j = i; j < active_track.length && match == 255; j++) {

           }

        }
        if (active_track.notes[a][i] > 0) {
          noteson++;
        }
        if (active_track.notes[a][i] < 0) {
          notesoff++;
        }
     }

  }

}

#ifndef OLED_DISPLAY
void SeqExtStepPage::display() {
 SeqPage::display();
}
#else
void SeqExtStepPage::display() {

#ifdef EXT_TRACKS
  oled_display.clearDisplay();


  auto &active_track = mcl_seq.ext_tracks[last_ext_track];

  uint8_t timing_mid = mcl_seq.ext_tracks[last_ext_track].get_timing_mid();

  //draw_knob_conditional(seq_param1.getValue());
  //draw_knob_timing(seq_param2.getValue(),timing_mid);

  MusicalNotes number_to_note;
  uint8_t notes_held = 0;
  uint8_t i, j;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      notes_held += 1;
    }
  }
  char K[4];
  itoa(seq_param3.getValue(), K, 10);
  draw_knob(2, "LEN", K);

  if (notes_held > 0) {
    uint8_t x = mcl_gui.knob_x0 + mcl_gui.knob_w * 3 + 2;
    auto *oldfont = oled_display.getFont();
    oled_display.setFont(&TomThumb);
    uint8_t note_idx = 0;
    for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++) {
        oled_display.setCursor(x + j * 11, 6 + i * 8);
        const int8_t &c_note =
            active_track
                .notes[note_idx][note_interface.last_note + page_select * 16];
        if (c_note != 0) {
          uint8_t note = abs(c_note);
          DEBUG_DUMP(c_note);
          DEBUG_DUMP(note);
          note = note - 1;
          uint8_t oct = note / 12;
          note = note - 12 * oct;
          DEBUG_DUMP(note);
          DEBUG_DUMP(oct);
          if (c_note > 0) {
            oled_display.print(number_to_note.notes_upper[note]);
          } else {
            oled_display.print(number_to_note.notes_lower[note]);
          }

          oled_display.print(oct);
        }

        ++note_idx;
      }
    }
    oled_display.setFont(oldfont);
  }


  SeqPage::display();
  if (mcl_gui.show_encoder_value(&seq_param2) &&
        (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
        (!show_step_menu)) {
      mcl_gui.draw_microtiming(get_ext_speed(mcl_seq.ext_tracks[last_ext_track].speed), seq_param2.cur);
   }
  oled_display.display();
#endif
}
#endif

bool SeqExtStepPage::handleEvent(gui_event_t *event) {
  if (SeqPage::handleEvent(event)) {
    return true;
  }

#ifdef EXT_TRACKS
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);
    uint8_t track = event->source - 128;

    if (device == DEVICE_A4) {
      track -= 16;
    }

    if (mask == EVENT_BUTTON_PRESSED) {
      DEBUG_PRINTLN(track);
      if (device == DEVICE_MD) {

        if ((track + (page_select * 16)) >= active_track.length) {
          DEBUG_PRINTLN("setting to 0");
          DEBUG_PRINTLN(last_ext_track);
          DEBUG_PRINTLN(page_select);
          note_interface.notes[track] = 0;
          return true;
        }

        int8_t utiming =
            active_track.timing[(track + (page_select * 16))]; // upper
        uint8_t condition =
            active_track.conditional[(track + (page_select * 16))]; // lower
        seq_param1.cur = translate_to_knob_conditional(condition);
        // Micro
        if (utiming == 0) {
          utiming = mcl_seq.ext_tracks[last_ext_track].get_timing_mid();
        }
        seq_param2.cur = utiming;

        note_interface.last_note = track;
      }
    }
    if (mask == EVENT_BUTTON_RELEASED) {
      if (device == DEVICE_MD) {

        uint8_t utiming = (seq_param2.cur + 0);
        uint8_t condition = translate_to_step_conditional(seq_param1.cur);
        if ((track + (page_select * 16)) >= active_track.length) {
          return true;
        }

        //  timing = 3;
        // condition = 3;
        if (clock_diff(note_interface.note_hold, slowclock) < TRIG_HOLD_TIME) {
          for (uint8_t c = 0; c < NUM_EXT_NOTES; c++) {
            if (active_track.notes[c][track + page_select * 16] > 0) {
              MidiUart2.sendNoteOff(
                  last_ext_track,
                  abs(active_track.notes[c][track + page_select * 16]) - 1, 0);
            }
            active_track.notes[c][track + page_select * 16] = 0;
          }
          active_track.timing[(track + (page_select * 16))] = 0;
          active_track.conditional[(track + (page_select * 16))] = 0;
        }

        else {
          active_track.timing[(track + (page_select * 16))] = utiming; // upper
          active_track.conditional[(track + (page_select * 16))] =
              condition; // upper
        }
      }
      return true;
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    GUI.setPage(&seq_step_page);
    return true;
  }

#endif
  return false;
}

void SeqExtStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  // Step edit for ExtSeq
  // For each incoming note, check to see if note interface has any steps
  // selected For selected steps record notes.
#ifdef EXT_TRACKS
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  DEBUG_PRINT("note on midi2 ext, ");
  DEBUG_DUMP(channel);

  if (channel < mcl_seq.num_ext_tracks) {
    last_ext_track = channel;
    seq_extstep_page.config_encoders();

    if (MidiClock.state != 2) {
      mcl_seq.ext_tracks[channel].note_on(msg[1]);
    }

    for (uint8_t i = 0; i < 16; i++) {
      if (note_interface.notes[i] == 1) {
        mcl_seq.ext_tracks[channel].set_ext_track_step(
            seq_extstep_page.page_select * 16 + i, msg[1], msg[2]);
      }
    }
  }
#endif
}

void SeqExtStepMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
#ifdef EXT_TRACKS
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (channel < mcl_seq.num_ext_tracks && MidiClock.state != 2) {
    mcl_seq.ext_tracks[channel].note_off(msg[1]);
  }
#endif
}

void SeqExtStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);

  state = true;
}

void SeqExtStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  state = false;
}
