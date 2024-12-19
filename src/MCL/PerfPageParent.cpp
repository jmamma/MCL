#include "MCLEncoder.h"
#include "PerfPageParent.h"
#include "ResourceManager.h"
#include "MD.h"
#include "MCLGUI.h"

void PerfPageParent::setup() { DEBUG_PRINT_FN(); }

void PerfPageParent::init() {
  DEBUG_PRINT_FN();
  MD.set_key_repeat(0);
  config_encoders();
  R.Clear();
  R.use_machine_param_names();
  R.use_icons_knob();
  setup_callbacks();
}

void PerfPageParent::cleanup() { MD.set_key_repeat(1); remove_callbacks(); }

void PerfPageParent::loop() {}

void PerfPageParent::draw_param(uint8_t knob, uint8_t dest, uint8_t param) {

  char myName[4] = "-- ";

  const char *modelname = NULL;
  if (dest == 0) {
    if (param > 1) {
      strcpy(myName, "LER");
    }
  } else {
    dest = dest - 1;
    if (dest >= NUM_MD_TRACKS + 4) {
      mcl_gui.put_value_at(param, myName);
    } else if (dest >= NUM_MD_TRACKS) {
      modelname = fx_param_name(MD_FX_ECHO + dest - 16, param);
    } else {
      modelname = model_param_name(MD.kit.get_model(dest), param);
    }

    if (modelname != NULL) {
      strncpy(myName, modelname, 4);
    }
  }
  mcl_gui.draw_knob(knob, "PAR", myName);
}

void PerfPageParent::draw_dest(uint8_t knob, uint8_t value, bool dest) {
  char K[4];
  K[0] = value > 20 ? 'M' : 'T';
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
    if (value > 20) {
      value -= 20;
    }
    mcl_gui.put_value_at(value, K + 1);
    break;
  }
  const char *str1 = "DEST";
  if (!dest) { str1 = "SRC"; }
  mcl_gui.draw_knob(knob, str1, K);
}

void PerfPageParent::display() {}

bool PerfPageParent::handleEvent(gui_event_t *event) {
  /*
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
  */
  return false;
}
