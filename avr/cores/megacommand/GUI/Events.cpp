/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"

#define MAX_BUTTONS 8

CRingBuffer<gui_event_t, MAX_EVENTS> EventRB;
volatile uint8_t event_ignore_next_mask;

void pollEventGUI() {
  for (int i = 0; i < MAX_BUTTONS; i++) {
    gui_event_t event;
    event.source = i;
    if (BUTTON_PRESSED(i)) {

      if (!IS_BIT_SET(event_ignore_next_mask, i)) {
        event.mask = EVENT_BUTTON_PRESSED;
        EventRB.putp(&event);
      } else {

        CLEAR_BIT(event_ignore_next_mask, i);
      }
    }
    if (BUTTON_RELEASED(i)) {

      if (!IS_BIT_SET(event_ignore_next_mask, i)) {
        event.mask = EVENT_BUTTON_RELEASED;
        EventRB.putp(&event);

      } else {
        CLEAR_BIT(event_ignore_next_mask, i);
      }
    }
  }
}
