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
    DeviceParamTarget target =
        device_idx != DeviceIdx::None
            ? DeviceParamResolver::target_for_idx(device_idx, dest)
            : DeviceParamResolver::perf(dest).params;
    bool labelled = target.param_label(param, myName, sizeof(myName));
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
    DeviceParamTarget target =
        device_idx != DeviceIdx::None
            ? DeviceParamResolver::target_for_idx(device_idx, value)
            : DeviceParamResolver::perf(value).params;
    bool labelled = target.target_label(K, sizeof(K));
    if (!labelled) {
      uint8_t local_value = value;
      K[0] = device_idx == DeviceIdx::Secondary ? 'M' : 'T';
      if (device_idx == DeviceIdx::None) {
        DeviceIdx dest_device = DeviceIdx::None;
        uint8_t target = 0;
        if (DeviceParamResolver::perf_dest_to_target(value, &dest_device,
                                                     &target)) {
          K[0] = dest_device == DeviceIdx::Secondary ? 'M' : 'T';
          local_value = target + 1;
        }
      }
      mcl_gui.put_value_at(local_value, K + 1);
    }
  }
  const char *label = dest ? mclstr_dest : mclstr_src;
  mcl_gui.draw_knob(knob, label, K);
}
