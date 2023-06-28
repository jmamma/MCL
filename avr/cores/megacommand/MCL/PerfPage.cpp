#include "MCL_impl.h"
#include "ResourceManager.h"

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
  oled_display.clearDisplay();
  oled_display.setFont();
  config_encoders();
  R.Clear();
  R.use_machine_param_names();
  setup_callbacks();
}

void PerfPage::cleanup() { remove_callbacks(); }

void PerfPage::config_encoders() {
  if (page_mode < PERF_DESTINATION) {
    encoders[0] = &fx_param1;
    encoders[1] = &fx_param2;
    encoders[2] = &fx_param3;
    encoders[3] = &fx_param4;

    uint8_t c = page_mode;
    PerfParam *p = &perf_encoders[perf_id]->perf_data.params[c];

    encoders[0]->cur = p->dest;
    ((PerfEncoder *)encoders[0])->max = NUM_MD_TRACKS + 4 + 16;

    encoders[1]->cur = p->param;
    ((PerfEncoder *)encoders[1])->max = 23;

    encoders[2]->cur = p->min;
    ((PerfEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = p->max;
    ((PerfEncoder *)encoders[3])->max = 127;
    if (encoders[0]->cur < NUM_MD_TRACKS + 4) {
      ((PerfEncoder *)encoders[1])->max = 7;
    }

    if (encoders[0]->cur < NUM_MD_TRACKS + 4 + 16) {
      ((PerfEncoder *)encoders[1])->max = 127;
    }
    else {
    ((PerfEncoder *)encoders[1])->max = 23;
    }
  }

  if (page_mode == PERF_DESTINATION) {
    encoders[0] = perf_encoders[0];
    encoders[1] = perf_encoders[1];
    encoders[2] = perf_encoders[2];
    encoders[3] = perf_encoders[3];
  }
  //  loop();

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    encoders[i]->old = encoders[i]->cur;
    ((LightPage *)this)->encoders_used_clock[i] =
        slowclock - SHOW_VALUE_TIMEOUT - 1;
  }
}
void PerfPage::loop() {

  if (page_mode < PERF_DESTINATION) {
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

    if (encoders[0]->cur < NUM_MD_TRACKS + 4) {
      ((PerfEncoder *)encoders[1])->max = 7;
    }

    if (encoders[0]->cur < NUM_MD_TRACKS + 4 + 16) {
      ((PerfEncoder *)encoders[1])->max = 127;
    }
    else {
    ((PerfEncoder *)encoders[1])->max = 23;
    }
}

void PerfPage::draw_param(uint8_t knob, uint8_t dest, uint8_t param) {

  char myName[4] = "-- ";

  const char *modelname = NULL;
  if (dest == 0) {
    if (param > 1) {
      strcpy(myName, "LER");
    }
  } else {
    if (dest < 17) {
      modelname = model_param_name(MD.kit.get_model(dest - 1), param);
    } else if (dest < 20) {
      modelname = fx_param_name(MD_FX_ECHO + dest - 17, param);
    } else {
      mcl_gui.put_value_at(param, myName);
    }
    if (modelname != NULL) {
      strncpy(myName, modelname, 4);
    }
  }
  mcl_gui.draw_knob(knob, "PAR", myName);
}

void PerfPage::draw_dest(uint8_t knob, uint8_t value) {
  char K[4];
  if (value > 20) {
    strcpy(K, "MI ");
    K[2] = '0' + value - 20 + 1;
  } else {
    switch (value) {
    case 0:
      strcpy(K, "--");
      break;
    case 17:
      strcpy(K, "ECH");
      break;
    case 18:
      strcpy(K, "REV");
      break;
    case 19:
      strcpy(K, "EQ");
      break;
    case 20:
      strcpy(K, "DYN");
      break;
    default:
      //  K[0] = 'T';
      mcl_gui.put_value_at(value, K);
      break;
    }
  }
  mcl_gui.draw_knob(knob, "DEST", K);
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
    mcl_gui.draw_knob(0, encoders[0], "A");
    mcl_gui.draw_knob(1, encoders[1], "B");
    mcl_gui.draw_knob(2, encoders[2], "C");
    mcl_gui.draw_knob(3, encoders[3], "D");
    info2 = "PER>MOD";
  }

  mcl_gui.draw_panel_labels(info1, info2);

  oled_display.display();
  oled_display.setFont(oldfont);
}

void PerfPage::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks
  uint8_t param_true = 0;

  MD.parseCC(channel, param, &track, &track_param);
  if (track > 15) {
    return;
  }
  if (learn) {
    encoders[0]->cur = track + 1;
    encoders[1]->cur = track_param;
    if (value < encoders[2]->cur) {
      encoders[2]->cur = value;
    }
    if (value > encoders[3]->cur) {
      encoders[3]->cur = value;
    }
  }
  if (page_mode < PERF_DESTINATION) {
    if (encoders[0]->cur == 0 && encoders[1]->cur > 1) {
      encoders[0]->cur = track + 1;
      encoders[1]->cur = track_param;
    }
  }
}

void PerfPage::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  if (learn) {
    encoders[0]->cur = channel + 1 + 16 + 4;
    encoders[1]->cur = param;
    ((PerfEncoder *)encoders[1])->max = 127;
    if (value < encoders[2]->cur) {
      encoders[2]->cur = value;
    }
    if (value > encoders[3]->cur) {
      encoders[3]->cur = value;
    }
  }
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

bool PerfPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    uint8_t page_select = 0;
    uint8_t step = track + (page_select * 16);
    if (event->mask == EVENT_BUTTON_PRESSED) {
    }
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  /*  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
        EVENT_PRESSED(event, Buttons.ENCODER2) ||
        EVENT_PRESSED(event, Buttons.ENCODER3) ||
        EVENT_PRESSED(event, Buttons.ENCODER4)) {
      mcl.setPage(GRID_PAGE);
    }
  */
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

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mcl.setPage(PAGE_SELECT_PAGE);
    return true;
  }

  return false;
}
