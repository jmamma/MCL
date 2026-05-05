#pragma once

#include "platform.h"
#include <inttypes.h>

#ifdef PLATFORM_TBD
#include "GUI.h"
#endif

class MidiDevice;

class DeviceManager {
public:
  MidiDevice *device_for_port(uint8_t port) const;
  void set_device_for_port(uint8_t port, MidiDevice *device);
  void update_active_slots();
  MidiDevice *dev1() const;
  MidiDevice *dev2() const;

#ifdef PLATFORM_TBD
  void ui_loop();
  bool handle_ui_event(gui_event_t *event);
  bool enter_ui(gui_event_t *event);
  bool is_ui_active() const;
  void exit_ui();
#endif

private:
  MidiDevice *physical_[3] = {nullptr, nullptr, nullptr};
  MidiDevice *dev1_ = nullptr;
  MidiDevice *dev2_ = nullptr;
};

extern DeviceManager device_manager;
