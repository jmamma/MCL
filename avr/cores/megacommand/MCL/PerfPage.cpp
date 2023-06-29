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

    PerfParam *p = &perf_encoders[perf_id]->perf_data.src_param;
    encoders[1]->cur = p->dest;
    encoders[2]->cur = p->param;
    config_encoder_range(1);
    encoders[3]->cur = p->min;
    ((PerfEncoder *)encoders[3])->max = 127;

  }

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    encoders[i]->old = encoders[i]->cur;
    ((LightPage *)this)->encoders_used_clock[i] = slowclock;
  }
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

  const char *info2 = "PERFORM";
  const char *info1;

  if (page_mode < PERF_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);
    mcl_gui.draw_knob(2, encoders[2], "MIN");
    mcl_gui.draw_knob(3, encoders[3], "MAX");
    info1 = "PARAM 1";
    mcl_gui.put_value_at(page_mode + 1, info1 + 6);
  }
  if (page_mode == PERF_DESTINATION) {
    char *str = "A ";
    str[1] = '1' + perf_id;
    mcl_gui.draw_knob(0, encoders[0], str);
    draw_dest(1, encoders[1]->cur);
    draw_param(2, encoders[1]->cur, encoders[2]->cur);
    mcl_gui.draw_knob(3, encoders[3], "MIN");
    info1 = "CONTROL";
  }

  mcl_gui.draw_panel_labels(info1, info2);

  oled_display.display();
  oled_display.setFont(oldfont);
}

void PerfPage::learn_param(uint8_t dest, uint8_t param, uint8_t value) {
  //Intercept controller param.
  PerfParam *p = &perf_encoders[perf_id]->perf_data.src_param;
  if (dest + 1== p->dest && param == p->param) {
    //Controller param, start value;
    uint8_t min = p->min;
    uint8_t max = 127;
    if (value >= min) {
       uint8_t cur = value - min;
       int8_t range = max - min;
       uint8_t val = ((float) cur / (float) range) * 127.0f;
       perf_encoders[perf_id]->cur = val;
       perf_encoders[perf_id]->send_params();
       if (mcl.currentPage() == PERF_PAGE_0) { update_params(); }
    }
  }
  if (mcl.currentPage() == PERF_PAGE_0) {

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
         send_locks(learn);
         old_mode = page_mode;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
       if (note_interface.notes_all_off()){
         learn = LEARN_OFF;
         MD.deactivate_encoder_interface();
         page_mode = old_mode;
         config_encoders();
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
