#include "MCLEncoder.h"
#include "DeviceManager.h"
#include "DevicePanelRef.h"
#include "DeviceParamResolver.h"
#include "PerfPageParent.h"
#include "ResourceManager.h"
#include "../Drivers/MidiDevice.h"
#include "MCLGUI.h"
#include "MCLStrings.h"

void PerfPageParent::setup() { DEBUG_PRINT_FN(); }

void PerfPageParent::init() {
  DEBUG_PRINT_FN();
  DevicePanelRef::set_primary_key_repeat(0);
  config_encoders();
  R.Clear();
  R.use_machine_param_names();
  R.use_icons_knob();
}

void PerfPageParent::cleanup() {
  DevicePanelRef::set_primary_key_repeat(1);
}

void PerfPageParent::draw_param(uint8_t knob, uint8_t dest, uint8_t param,
                                DeviceIdx device_idx) {

  char myName[4];
  mclstr_copy_progmem(myName, mclstr_dash_space, sizeof(myName));

  if (dest == 0) {
    if (param > 1) {
      strcpy_P(myName, mclstr_ler);
    }
  } else {
    bool labelled = device_idx != DeviceIdx::None
                        ? DeviceParamResolver::target_for_idx(device_idx, dest)
                              .param_label(param, myName, sizeof(myName))
                        : DeviceParamResolver::perf(dest)
                              .param_label(param, myName, sizeof(myName));
    if (!labelled) {
      mcl_gui.put_value_at(param, myName);
    }
  }
  mcl_gui.draw_knob(knob, mclstr_par, myName);
}

void PerfPageParent::draw_dest(uint8_t knob, uint8_t value, bool dest,
                               DeviceIdx device_idx) {
  char K[5];
  if (value == 0) {
    strcpy_P(K, mclstr_dash);
  } else {
    bool labelled = device_idx != DeviceIdx::None
                        ? DeviceParamResolver::target_for_idx(device_idx, value)
                              .target_label(K, sizeof(K))
                        : DeviceParamResolver::perf(value).target_label(
                              K, sizeof(K));
    if (!labelled) {
      uint8_t local_value = value;
      K[0] = device_idx == DeviceIdx::Secondary ? 'M' : 'T';
      if (device_idx == DeviceIdx::None) {
        uint8_t primary_count =
            DeviceParamResolver::target_count_for_idx(DeviceIdx::Primary);
        if (value > primary_count) {
          K[0] = 'M';
          local_value = value - primary_count;
        }
      }
      mcl_gui.put_value_at(local_value, K + 1);
    }
  }
  const char *label = dest ? mclstr_dest : mclstr_src;
  mcl_gui.draw_knob(knob, label, K);
}

bool PerfPageParent::handleEvent(gui_event_t *event) {
  /*
    if (EVENT_NOTE(event)) {
      uint8_t mask = event->mask;
      uint8_t port = event->port;
      auto device = device_manager.device_for_port(port);

      uint8_t track = event->source;
      uint8_t page_select = 0;
      uint8_t step = track + (page_select * 16);
      if (event->mask == EVENT_BUTTON_PRESSED) {
      }
    }
  */
  return false;
}
