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
#define EVENT_BUTTON(event) ((event->type == BUTTON))
#define EVENT_CMD(event) ((event->type == CMD))
#define EVENT_NOTE(event) ((event->type == NOTE))

/* --- Key Repeat Definitions --- */
#define ARROW_KEY_START_ID         Buttons.FUNC_BUTTON6
#define NUM_ARROW_KEYS             4
#define LONG_PRESS_INITIAL_DELAY_MS 160
#define LONG_PRESS_REPEAT_RATE_MS   60

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

  // --- Generic/default implementation ---
  void pollEvents_() {
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
      if (BUTTON_PRESSED(i)) {
        if (!isIgnored(i)) {
          gui_event_t event;
          event.source = i;
          event.type = BUTTON;
          event.mask = EVENT_BUTTON_PRESSED;
          eventBuffer.putp(&event);
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
      }
    }
  }

#if defined(PLATFORM_TBD)
  // --- State variables needed only for the TBD platform ---
  uint16_t long_press_start_time[NUM_ARROW_KEYS];
  uint16_t last_repeat_clock[NUM_ARROW_KEYS];
  bool     is_repeating[NUM_ARROW_KEYS];
  
  // --- Platform-specific implementation for TBD with key repeats ---
  void pollEventsTBD() {
    // Poll for standard button presses and releases
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
      if (BUTTON_PRESSED(i)) {
        if (!isIgnored(i)) {
          gui_event_t event;
          event.source = i;
          event.type = BUTTON;
          event.mask = EVENT_BUTTON_PRESSED;
          eventBuffer.putp(&event);

          // If it's an arrow key, start tracking it for repeats.
          if (i >= ARROW_KEY_START_ID && i < (ARROW_KEY_START_ID + NUM_ARROW_KEYS)) {
            uint8_t arrow_key_index = i - ARROW_KEY_START_ID;
            long_press_start_time[arrow_key_index] = read_clock_ms();
            is_repeating[arrow_key_index] = false;
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

        // If it was an arrow key, stop tracking it for repeats.
        if (i >= ARROW_KEY_START_ID && i < (ARROW_KEY_START_ID + NUM_ARROW_KEYS)) {
          uint8_t arrow_key_index = i - ARROW_KEY_START_ID;
          long_press_start_time[arrow_key_index] = 0;
          is_repeating[arrow_key_index] = false;
        }
      }
    }
    
    // --- Key repeat generation logic ---
    uint16_t current_time = read_clock_ms();
    for (uint8_t i = 0; i < NUM_ARROW_KEYS; i++) {
      if (long_press_start_time[i] == 0) continue; 

      uint8_t button_id = ARROW_KEY_START_ID + i;
      if (BUTTON_DOWN(button_id)) {
        if (is_repeating[i]) {
          if (clock_diff(last_repeat_clock[i], current_time) > LONG_PRESS_REPEAT_RATE_MS) {
            gui_event_t event;
            event.source = button_id; event.type = BUTTON; event.mask = EVENT_BUTTON_PRESSED;
            eventBuffer.putp(&event);
            last_repeat_clock[i] = current_time;
          }
        } else {
          if (clock_diff(long_press_start_time[i], current_time) > LONG_PRESS_INITIAL_DELAY_MS) {
            gui_event_t event;
            event.source = button_id; event.type = BUTTON; event.mask = EVENT_BUTTON_PRESSED;
            eventBuffer.putp(&event);
            is_repeating[i] = true;
            last_repeat_clock[i] = current_time;
          }
        }
      } else {
        long_press_start_time[i] = 0;
        is_repeating[i] = false;
      }
    }
  }
#endif
public:
  EventManager() : ignoreMask(0) {}

  void init() {
    eventBuffer.init();
#if defined(PLATFORM_TBD)
    for (int i = 0; i < NUM_ARROW_KEYS; i++) {
      long_press_start_time[i] = 0;
      is_repeating[i] = false;
      last_repeat_clock[i] = 0;
    }
#endif
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

  // --- Public dispatcher function ---
  // Calls the correct underlying implementation at compile time.
  void pollEvents() {
#if defined(PLATFORM_TBD)
    pollEventsTBD();
#else
    pollEvents_();
#endif
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
