/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include <inttypes.h>
#include "RingBuffer.h"
#include "helpers.h"
#include "platform.h"
#include "GUI_hardware.h"

// Event type definitions
#define EVENT_BUTTON_PRESSED  _BV(0)
#define EVENT_BUTTON_RELEASED _BV(1)
#define EVENT_BUTTON_LONG_PRESS _BV(2)

#define MAX_BUTTONS GUI_NUM_BUTTONS
#define MAX_EVENTS 32

#define EVENT_PRESSED(event, button) ((event)->mask & EVENT_BUTTON_PRESSED && (event)->source == button)
#define EVENT_RELEASED(event, button) ((event)->mask & EVENT_BUTTON_RELEASED && (event)->source == button)
#define EVENT_CMD(event) ((event->type == CMD))

#define LONG_PRESS_REPEAT_TIME 40 // 40ms

enum EventType {
  BUTTON,
  CMD,
  NOTE,
};

typedef struct gui_event_s {
  uint8_t mask;
  uint8_t source;
  uint8_t port;
  EventType type;
} gui_event_t;

class EventManager {
private:
  volatile uint8_t ignoreMask;
  volatile CRingBuffer<gui_event_t, MAX_EVENTS> eventBuffer;
  uint16_t last_repeat_clock; // Added for long press repeat

public:
  EventManager() : ignoreMask(0) {}


  void init() {
    eventBuffer.init();
  }
  void setIgnoreMask(uint8_t button) {
    if (button < MAX_BUTTONS) {
      ignoreMask |= _BV(button);
    }
  }

  void clearIgnoreMask(uint8_t button) {
    if (button < MAX_BUTTONS) {
      ignoreMask &= ~_BV(button);
    }
  }

  bool isIgnored(uint8_t button) {
    return (button < MAX_BUTTONS) && (ignoreMask & _BV(button));
  }

  void pollEvents() {
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
      gui_event_t event;
      event.source = i;
      event.type = BUTTON;
      if (BUTTON_PRESSED(i)) {
        if (!isIgnored(i)) {
          event.mask = EVENT_BUTTON_PRESSED;
          eventBuffer.putp(&event);
        } else {
          clearIgnoreMask(i);
        }
      }
      if (BUTTON_RELEASED(i)) {
        if (!isIgnored(i)) {
          event.mask = EVENT_BUTTON_RELEASED;
          eventBuffer.putp(&event);
        } else {
          clearIgnoreMask(i);
        }
      }
      if (BUTTON_LONG_CLICKED(i)) { // Check for long press
        if (clock_diff(last_repeat_clock, g_clock_ms) > LONG_PRESS_REPEAT_TIME) {
          event.mask = EVENT_BUTTON_PRESSED;
          eventBuffer.putp(&event);
          last_repeat_clock = g_clock_ms;
        }
      }
    }
  }


  bool isEmpty() {
    return eventBuffer.isEmpty();
  }

  void putEvent(gui_event_t* event) {
    eventBuffer.putp(event);
  }

  void getEvent(gui_event_t* event) {
    eventBuffer.getp(event);
    return;
  }

};
