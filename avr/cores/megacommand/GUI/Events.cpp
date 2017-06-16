/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"

volatile CRingBuffer<gui_event_t, 8> EventRB;

void pollEventGUI() {
  for (int i = 0; i < 8; i++) {
    gui_event_t event;
    event.source = i;
    if (BUTTON_PRESSED(i)) {
      event.mask = EVENT_BUTTON_PRESSED;
      EventRB.putp(&event);
    }
    if (BUTTON_RELEASED(i)) {
      event.mask = EVENT_BUTTON_RELEASED;
      EventRB.putp(&event);
    }
  }
}
