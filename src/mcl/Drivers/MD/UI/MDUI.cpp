#ifdef PLATFORM_TBD

#include "MDUI.h"
#include "../MD.h"

MDUI::MDUI(MDClass &md) : panel_(md) {}

void MDUI::loop() {
  sps_mode.poll_encoders();
  sps_mode.poll_page_overlay();
}

bool MDUI::handle_event(gui_event_t *event) {
  return panel_.handle_event(event);
}

bool MDUI::enter(gui_event_t *event) {
  (void)event;
  sps_mode.enable();
  return true;
}

bool MDUI::is_active() const {
  return sps_mode.is_active();
}

void MDUI::exit() {
  sps_mode.disable();
}

void MDUI::handle_ui_slot_button(bool pressed) {
  sps_mode.handle_ui_slot_button(pressed);
}

#endif // PLATFORM_TBD
