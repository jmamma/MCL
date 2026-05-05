#include "DeviceManager.h"

#ifdef PLATFORM_TBD
#include "MidiActivePeering.h"

namespace {

inline MidiDevice *active_dev1() {
  return midi_active_peering.dev1;
}

inline MidiDevice *active_dev2() {
  MidiDevice *dev1 = active_dev1();
  MidiDevice *dev2 = midi_active_peering.dev2;
  return dev2 != dev1 ? dev2 : nullptr;
}

} // namespace
#endif

DeviceManager device_manager;

#ifdef PLATFORM_TBD
void DeviceManager::ui_loop() {
  MidiDevice *dev1 = active_dev1();
  MidiDevice *dev2 = active_dev2();
  if (dev1) dev1->ui_loop();
  if (dev2) dev2->ui_loop();
}

bool DeviceManager::handle_ui_event(gui_event_t *event) {
  MidiDevice *dev1 = active_dev1();
  MidiDevice *dev2 = active_dev2();
  if (dev1 && dev1->handle_ui_event(event)) return true;
  if (dev2 && dev2->handle_ui_event(event)) return true;
  return false;
}

bool DeviceManager::is_ui_active() const {
  MidiDevice *dev1 = active_dev1();
  MidiDevice *dev2 = active_dev2();
  return (dev1 && dev1->is_ui_active()) ||
         (dev2 && dev2->is_ui_active());
}

void DeviceManager::mark_tr_consumed() {
  MidiDevice *dev1 = active_dev1();
  MidiDevice *dev2 = active_dev2();
  if (dev1) dev1->mark_tr_consumed();
  if (dev2) dev2->mark_tr_consumed();
}

void DeviceManager::mark_b_consumed() {
  MidiDevice *dev1 = active_dev1();
  MidiDevice *dev2 = active_dev2();
  if (dev1) dev1->mark_b_consumed();
  if (dev2) dev2->mark_b_consumed();
}
#endif
