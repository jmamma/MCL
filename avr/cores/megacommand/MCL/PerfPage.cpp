#include "MCL_impl.h"
#include "ResourceManager.h"
#include "MCLMemory.h"

#define LEARN_MIN 1
#define LEARN_MAX 2
#define LEARN_OFF 0

void PerfPage::setup() {
  DEBUG_PRINT_FN();
  page_mode = PERF_DESTINATION;
  perf_encoders[0] = &perf_param1;
  perf_encoders[1] = &perf_param2;
  perf_encoders[2] = &perf_param3;
  perf_encoders[3] = &perf_param4;
}

void PerfPage::init() {
  DEBUG_PRINT_FN();
  PerfPageParent::init();
  trig_interface.on();
  MD.set_trigleds(0b0011001100110011, TRIGLED_OVERLAY);
}

void PerfPage::cleanup() { PerfPageParent::cleanup(); }

void PerfPage::config_encoder_range(uint8_t i) {
 ((PerfEncoder *)encoders[i])->max = NUM_MD_TRACKS + 4 + 16;
 ((PerfEncoder *)encoders[i + 1])->min = 0;


 DEBUG_PRINTLN(encoders[i]->cur);
 DEBUG_PRINTLN(encoders[i + 1]->cur);
  if (encoders[i]->cur >= 16 + 4) {
    ((PerfEncoder *)encoders[i + 1])->max = 127;
  }
  else if (encoders[i]->cur >= 16) {
    ((PerfEncoder *)encoders[i + 1])->max = 7;
  }
  else {
     ((PerfEncoder *)encoders[i + 1])->max = 23;
  }
}

void PerfPage::config_encoders() {
    encoders[1] = &fx_param2;
    encoders[2] = &fx_param3;
    encoders[3] = &fx_param4;

  if (page_mode < PERF_DESTINATION) {
    encoders[0] = &fx_param1;
    uint8_t c = page_mode;
    PerfParam *p = &perf_encoders[perf_id]->perf_data.params[c];

    encoders[0]->cur = p->dest;
    encoders[1]->cur = p->param;

    encoders[2]->cur = p->min;
    ((PerfEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = p->max;
    ((PerfEncoder *)encoders[3])->max = 127;

    config_encoder_range(0);
  }

  if (page_mode == PERF_DESTINATION) {
    encoders[0] = perf_encoders[perf_id];
    ((PerfEncoder *)encoders[0])->max = 127;
    config_encoder_range(1);

    PerfParam *p = &perf_encoders[perf_id]->perf_data.src_param;
    encoders[1]->cur = p->dest;
    encoders[2]->cur = p->param;

    encoders[3]->cur = p->min;
    ((PerfEncoder *)encoders[3])->max = 127;

  }
/*
      for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    encoders[i]->old = encoders[i]->cur;
    ((LightPage *)this)->encoders_used_clock[i] =
        slowclock - SHOW_VALUE_TIMEOUT - 1;
  }
*/
}
void PerfPage::update_params() {
  if (page_mode < PERF_DESTINATION) {
    config_encoder_range(0);

    if (encoders[0]->hasChanged() && encoders[0]->cur == 0) {
      encoders[1]->cur = 0;
    }
    uint8_t c = page_mode;
    PerfParam *p = &perf_encoders[perf_id]->perf_data.params[c];
    p->dest = encoders[0]->cur;
    p->param = encoders[1]->cur;
    p->min = encoders[2]->cur;
    p->max = encoders[3]->cur;
  }
  else {
    config_encoder_range(1);

    if (encoders[1]->hasChanged() && encoders[1]->cur == 0) {
      encoders[2]->cur = 0;
    }
     uint8_t c = page_mode;
     PerfParam *p = &perf_encoders[perf_id]->perf_data.src_param;
     p->dest = encoders[1]->cur;
     p->param = encoders[2]->cur;
     p->min = encoders[3]->cur;
  }

}

void PerfPage::loop() {
   update_params();
}

void PerfPage::display() {
  oled_display.clearDisplay();
  auto oldfont = oled_display.getFont();

  mcl_gui.draw_panel_number(perf_id);

  uint8_t x = mcl_gui.knob_x0 + 5;
  uint8_t y = 8;
  uint8_t lfo_height = 7;
  uint8_t width = 13;

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  mcl_gui.draw_knob_frame();

  const char *info1;
  const char *info2;

  if (page_mode < PERF_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);
    mcl_gui.draw_knob(2, encoders[2], "MIN");
    mcl_gui.draw_knob(3, encoders[3], "MAX");
    info2 = "PER>DST";
  }
  if (page_mode == PERF_DESTINATION) {
    char *str = "A ";
    str[1] = '1' + perf_id;
    mcl_gui.draw_knob(0, encoders[0], str);
    draw_dest(1, encoders[1]->cur);
    draw_param(2, encoders[1]->cur, encoders[2]->cur);
    mcl_gui.draw_knob(3, encoders[3], "MIN");
    info2 = "PER>MOD";
  }

  mcl_gui.draw_panel_labels(info1, info2);

  oled_display.display();
  oled_display.setFont(oldfont);
}

void PerfPage::learn_param(uint8_t dest, uint8_t param, uint8_t value) {
  //Intercept controller param.
  PerfParam *p = &perf_encoders[perf_id]->perf_data.src_param;
  if (dest + 1== p->dest && param == p->param) {
    perf_encoders[perf_id]->cur = value;
  }

  if (learn) {
    PerfData *d = &perf_encoders[perf_id]->perf_data;
    uint8_t n = d->add_param(dest, param, learn, value);
    page_mode = n;
    config_encoders();
  }

  //MIDI LEARN current mode;
  uint8_t a = page_mode == PERF_DESTINATION ? 1 : 0;

  if (encoders[a]->cur == 0 && encoders[a + 1]->cur > 0) {
      encoders[a]->cur = dest + 1;
      encoders[a + 1]->cur = param;
      encoders[a + 2]->cur = 0;
      update_params();
      config_encoders();
  }

}

void PerfPage::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks

  MD.parseCC(channel, param, &track, &track_param);
  if (track > 15) {
    return;
  }

  learn_param(track, track_param, value);

}

void PerfPage::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  learn_param(channel + 16 + 4, param, value);
}


void PerfPage::setup_callbacks() {
  if (midi_state) {
    return;
  }
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&PerfPage::onControlChangeCallback_Midi);
  Midi2.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&PerfPage::onControlChangeCallback_Midi2);

  midi_state = true;
}

void PerfPage::remove_callbacks() {
  if (!midi_state) {
    return;
  }

  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&PerfPage::onControlChangeCallback_Midi);
  Midi2.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&PerfPage::onControlChangeCallback_Midi2);

  midi_state = false;
}

void PerfPage::send_locks(uint8_t mode) {
  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];
  uint8_t params[24];
  memset(params, 255, sizeof(params));

  for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
    PerfParam *p = &perf_encoders[perf_id]->perf_data.params[n];
    uint8_t dest = p->dest;
    uint8_t param = p->param;

    if (param >= 24) { continue; }

    if (dest == last_md_track + 1) {
      if (mode == LEARN_MIN) {
        params[param] = p->min;
      }
      if (mode == LEARN_MAX) {
        params[param] = p->max;
      }
    }
  }
  MD.activate_encoder_interface(params);
}

bool PerfPage::handleEvent(gui_event_t *event) {
  if (PerfPageParent::handleEvent(event)) { return true; }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (event->mask == EVENT_BUTTON_PRESSED) {
         uint8_t id = track / 4;
         if (perf_id != id) {
           perf_id = track / 4;
           config_encoders();
         }
         uint8_t b = track - (perf_id) * 4;
         if (b == 0) {
            learn = LEARN_MIN;
         }
         if (b == 1) {
            learn = LEARN_MAX;
         }
         if (page_mode < PERF_DESTINATION) {
           send_locks(learn);
         }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
       if (note_interface.notes_all_off()){
         learn = LEARN_OFF;
         MD.deactivate_encoder_interface();
       }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    page_mode++;
    if (page_mode > PERF_DESTINATION) {
      page_mode = 0;
    }
    config_encoders();
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    learn = true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    learn = false;
  }


  return false;
}
