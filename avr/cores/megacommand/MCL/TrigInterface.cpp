#include "MCL_impl.h"

/*
TrigInterfaceTask trig_interface_task;

void TrigInterfaceTask::run() {
  uint8_t key = 255;

  if (trig_interface.is_key_down(MDX_KEY_LEFT)) {
    key = MDX_KEY_LEFT;
  }
  if (trig_interface.is_key_down(MDX_KEY_RIGHT)) {
    key = MDX_KEY_RIGHT;
  }
  if (trig_interface.is_key_down(MDX_KEY_UP)) {
    key = MDX_KEY_UP;
  }
  if (trig_interface.is_key_down(MDX_KEY_DOWN)) {
    key = MDX_KEY_DOWN;
  }

  if (key == 255) { 
    GUI.removeTask(&trig_interface_task);     
    return; 
  }

  gui_event_t event;
  event.source = key + 64; // EVENT_CMD
  event.mask = EVENT_BUTTON_PRESSED;
  event.port = UART1_PORT;
  EventRB.putp(&event);
 
}
*/

void TrigInterface::start() {}

void TrigInterface::send_md_leds(TrigLEDMode mode) {
  uint16_t led_mask = 0;
  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      SET_BIT16(led_mask, i);
    }
  }
  MD.set_trigleds(led_mask, mode);
}

void TrigInterface::enable_listener() {
  cmd_key_state = 0;
  sysex->addSysexListener(this);
}

void TrigInterface::disable_listener() { sysex->removeSysexListener(this); }

bool TrigInterface::on(bool clear_states) {
  note_interface.init_notes();
  if (clear_states) { cmd_key_state = 0; }
  note_interface.note_proceed = true;
  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  state = true;
  DEBUG_PRINTLN(F("activating trig interface"));
  MD.activate_trig_interface();
  return true;
}

bool TrigInterface::off() {
  note_interface.note_proceed = false;
  DEBUG_PRINTLN(F("deactiviating trig interface"));
  if (!state) {
    return false;
  }
  state = false;
  if (!MD.connected) {
    return false;
  }
  MD.deactivate_trig_interface();
  return true;
}

void TrigInterface::end() {}

void TrigInterface::end_immediate() {
  // if (!state) {
  //  return;
  //}
  if (sysex->getByte(0) != ids[0]) {
    return;
  }
  if (sysex->getByte(1) != ids[1]) {
    return;
  }

  uint8_t key = sysex->getByte(2);
  bool key_release = false;

  if (key >= 0x40) {
    key_release = true;
    key -= 0x40;
  }
  if (key_release) {
    CLEAR_BIT64(cmd_key_state, key);
  } else {
    SET_BIT64(cmd_key_state, key);
  }

  if (IS_BIT_SET64(ignore_next_mask, key)) {
    CLEAR_BIT64(ignore_next_mask, key);
    return;
  }

  if (key < 16) {
    if (key_release) {
      note_interface.note_off_event(key, UART1_PORT);
    } else {
      note_interface.note_on_event(key, UART1_PORT);
    }
    return;
  }
  gui_event_t event;
  event.source = key + 64; // EVENT_CMD
  event.mask = key_release ? EVENT_BUTTON_RELEASED : EVENT_BUTTON_PRESSED;
  event.port = UART1_PORT;
  EventRB.putp(&event);

  return;
}

TrigInterface trig_interface;
