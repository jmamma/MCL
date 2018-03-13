/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"

#define MAX_EVENTS 32
#define MAX_BUTTONS 8
volatile CRingBuffer<gui_event_t, MAX_EVENTS> EventRB;

void pollEventGUI() {
  for (int i = 0; i < MAX_BUTTONS; i++) {
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
