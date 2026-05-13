#pragma once

#include "platform.h"
#include "MidiSetup.h"
#include "../Drivers/DeviceContext.h"
#include "../Drivers/MidiDeviceCapabilities.h"
#include <inttypes.h>

#ifdef PLATFORM_TBD
#include "GUI.h"
#endif

class MidiDevice;

class DeviceManager {
public:
  static constexpr uint8_t LOGICAL_SLOT_NONE = 255;

  DeviceManager();

  MidiDevice *device_for_port(uint8_t port) const;
  uint8_t logical_idx_for_port(uint8_t port) const;
  bool port_supports(uint8_t port, MidiDeviceCapability capability) const;
  bool port_is_elektron(uint8_t port) const;
  void attach_port(uint8_t port, MidiDevice *device,
                   uint8_t logical_idx = LOGICAL_SLOT_NONE);
  void detach_port(uint8_t port);
  void update_active_slots();
  MidiDevice *primary_device() const;
  MidiDevice *secondary_device() const;
  MidiDevice *slot_device(uint8_t slot) const;
  DeviceContext context_for_slot(uint8_t slot) const;

#ifdef PLATFORM_TBD
  static constexpr uint8_t UI_SLOT_PRIMARY = 0;
  static constexpr uint8_t UI_SLOT_SECONDARY = 1;
  static constexpr uint8_t UI_SLOT_NONE = 255;

  void ui_loop();
  bool handle_ui_event(gui_event_t *event);
  bool enter_ui(gui_event_t *event);
  bool enter_ui(MidiDevice *device, gui_event_t *event);
  bool enter_ui_slot(uint8_t slot, gui_event_t *event,
                     bool allow_toggle = true);
  bool enter_ui_slot_tap(uint8_t slot, gui_event_t *event);
  bool handle_ui_slot_button(uint8_t slot, gui_event_t *event,
                             bool allow_toggle = true);
  bool is_ui_slot_active(uint8_t slot) const;
  bool notify_active_ui_button(gui_event_t *event);
  bool is_ui_active() const;
  bool is_ui_collapsed() const;
  void exit_ui();
#endif

private:
  void set_device_for_port(uint8_t port, MidiDevice *device);

  MidiDevice *physical_[MIDI_PORT_COUNT] = {};
  uint8_t logical_idx_[MIDI_PORT_COUNT];
  MidiDevice *primary_ = nullptr;
  MidiDevice *secondary_ = nullptr;
#ifdef PLATFORM_TBD
  MidiDevice *active_ui_device_ = nullptr;
  uint8_t active_ui_slot_ = UI_SLOT_NONE;
#endif
};

extern DeviceManager device_manager;
