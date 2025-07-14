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

#define LONG_PRESS_REPEAT_TIME 80 // 40ms

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

  // State variables for timer-based long press
  int8_t   long_press_candidate;
  uint16_t last_repeat_clock;

public:
  EventManager() : ignoreMask(0), long_press_candidate(-1) {}

  void init() {
    eventBuffer.init();
    long_press_candidate = -1;
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
      if (BUTTON_PRESSED(i)) {
        if (!isIgnored(i)) {
          gui_event_t event;
          event.source = i;
          event.type = BUTTON;
          event.mask = EVENT_BUTTON_PRESSED;
          eventBuffer.putp(&event);

          //Assumes arrow keys.
          if (event.source >= Buttons.FUNC_BUTTON6 && event.source < Buttons.TRIG_BUTTON1) {
          // Set this button as the new candidate for repeating
            long_press_candidate = i;
            last_repeat_clock = g_clock_ms; // Initialize repeat clock
          }
        } else {
          clearIgnoreMask(i);
        }
      }

      if (BUTTON_RELEASED(i)) {
        if (!isIgnored(i)) {
          gui_event_t event;
          event.source = i;
          event.type = BUTTON;
          event.mask = EVENT_BUTTON_RELEASED;
          eventBuffer.putp(&event);
        } else {
          clearIgnoreMask(i);
        }

        // If the released button was our candidate, stop repeating
        if (long_press_candidate == i) {
          long_press_candidate = -1;
        }
      }
    }

    if (long_press_candidate != -1) {
      if (BUTTON_DOWN(long_press_candidate)) {
        if (clock_diff(last_repeat_clock, g_clock_ms) > LONG_PRESS_REPEAT_TIME) {
          gui_event_t event;
          event.source = (uint8_t)long_press_candidate;
          event.type = BUTTON;
          event.mask = EVENT_BUTTON_PRESSED; // Send a standard PRESS event
          eventBuffer.putp(&event);
          last_repeat_clock = g_clock_ms; // Update the timer for the next repeat
        }
      } else {
        // Button was released, clear the candidate just in case the RELEASED event was missed
        long_press_candidate = -1;
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
