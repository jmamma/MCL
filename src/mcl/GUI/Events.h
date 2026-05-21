#pragma once

#include <inttypes.h>
#include "RingBuffer.h"
#include "helpers.h"
#include "platform.h"
#include "MCLFeatureConfig.h"
#include "GUI_hardware.h"

// Event type definitions
#define EVENT_BUTTON_PRESSED  _BV(0)
#define EVENT_BUTTON_RELEASED _BV(1)
#define EVENT_BUTTON_LONG_PRESS _BV(2)

#define MAX_BUTTONS GUI_NUM_BUTTONS
#define MAX_EVENTS 32

#if GUI_NUM_BUTTONS > 8
typedef uint64_t event_ignore_mask_t;
#else
typedef uint8_t event_ignore_mask_t;
#endif
static_assert(GUI_NUM_BUTTONS <= sizeof(event_ignore_mask_t) * 8,
              "event_ignore_mask_t cannot represent all GUI buttons");

#define EVENT_PRESSED(event, button) ((event)->mask & EVENT_BUTTON_PRESSED && (event)->source == button)
#define EVENT_RELEASED(event, button) ((event)->mask & EVENT_BUTTON_RELEASED && (event)->source == button)
#define EVENT_BUTTON(event) ((event->type == BUTTON))
#define EVENT_CMD(event) ((event->type == CMD))
#define EVENT_NOTE(event) ((event->type == NOTE))

/* --- Key Repeat Definitions --- */
#define ARROW_KEY_START_ID         Buttons.FUNC_BUTTON6
#define NUM_ARROW_KEYS             4
#define LONG_PRESS_INITIAL_DELAY_MS 180
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
  volatile event_ignore_mask_t ignoreMask = 0;
  volatile CRingBuffer<gui_event_t, MAX_EVENTS> eventBuffer;

  static event_ignore_mask_t buttonMask(uint8_t button) {
    return ((event_ignore_mask_t)1) << button;
  }

  // --- Generic/default implementation ---
  void pollEvents_() {
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
      bool pressed = BUTTON_PRESSED(i);
      bool released = BUTTON_RELEASED(i);

      if (pressed || released) {
        bool ignored = isIgnored(i);
        if (!ignored) {
          gui_event_t event;
          event.source = i;
          event.type = BUTTON;
          event.mask = pressed ? EVENT_BUTTON_PRESSED : EVENT_BUTTON_RELEASED;
          eventBuffer.putp(&event);
        } else {
          clearIgnoreMask(i);
        }
      }
    }
  }
#if defined(MCL_HAS_EXTENDED_PANEL_INPUT)
  // --- State variables needed for extended panel arrow repeat ---
  uint16_t long_press_start_time[NUM_ARROW_KEYS];
  uint16_t last_repeat_clock[NUM_ARROW_KEYS];
  bool is_repeating[NUM_ARROW_KEYS];
  void pollEventsExtendedPanel() {
    // Poll for standard button presses and releases
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
      bool pressed = BUTTON_PRESSED(i);
      bool released = BUTTON_RELEASED(i);

      if (pressed || released) {
        bool ignored = isIgnored(i);
        if (!ignored) {
          gui_event_t event;
          event.source = i;
          event.type = BUTTON;
          event.mask = pressed ? EVENT_BUTTON_PRESSED : EVENT_BUTTON_RELEASED;
          eventBuffer.putp(&event);
        } else {
          clearIgnoreMask(i);
        }

        // Handle arrow key repeat tracking
        if (i >= ARROW_KEY_START_ID &&
            i < (ARROW_KEY_START_ID + NUM_ARROW_KEYS)) {
          uint8_t arrow_key_index = i - ARROW_KEY_START_ID;

          if (pressed && !ignored) {
            // Start tracking for repeats on press
            long_press_start_time[arrow_key_index] = read_clock_ms();
            is_repeating[arrow_key_index] = false;
          } else if (released) {
            // Stop tracking on release
            long_press_start_time[arrow_key_index] = 0;
            is_repeating[arrow_key_index] = false;
          }
        }
      }
    }
    // --- Key repeat generation logic ---
    uint16_t current_time = read_clock_ms();
    for (uint8_t i = 0; i < NUM_ARROW_KEYS; i++) {
      if (long_press_start_time[i] == 0)
        continue;

      uint8_t button_id = ARROW_KEY_START_ID + i;
      if (BUTTON_DOWN(button_id)) {
        bool should_send_event = false;

        if (is_repeating[i]) {
          // Already repeating - check repeat rate
          if (clock_diff(last_repeat_clock[i], current_time) >
              LONG_PRESS_REPEAT_RATE_MS) {
            should_send_event = true;
          }
        } else {
          // Not yet repeating - check initial delay
          if (clock_diff(long_press_start_time[i], current_time) >
              LONG_PRESS_INITIAL_DELAY_MS) {
            should_send_event = true;
            is_repeating[i] = true;
          }
        }

        if (should_send_event) {
          gui_event_t event;
          event.source = button_id;
          event.type = BUTTON;
          event.mask = EVENT_BUTTON_PRESSED;
          eventBuffer.putp(&event);
          last_repeat_clock[i] = current_time;
        }
      }
    }
  }
#endif
public:
  EventManager() = default;

  void init() {
    eventBuffer.init();
#if defined(MCL_HAS_EXTENDED_PANEL_INPUT)
    for (int i = 0; i < NUM_ARROW_KEYS; i++) {
      long_press_start_time[i] = 0;
      is_repeating[i] = false;
      last_repeat_clock[i] = 0;
    }
#endif
  }

  void setIgnoreMask(uint8_t button) {
    if (button < MAX_BUTTONS) {
      ignoreMask |= buttonMask(button);
    }
  }

  void clearIgnoreMask(uint8_t button) {
    if (button < MAX_BUTTONS) {
      ignoreMask &= ~buttonMask(button);
    }
  }

  bool isIgnored(uint8_t button) {
    return (button < MAX_BUTTONS) && (ignoreMask & buttonMask(button));
  }

  // --- Public dispatcher function ---
  // Calls the correct underlying implementation at compile time.
  void pollEvents() {
#if defined(MCL_HAS_EXTENDED_PANEL_INPUT)
    pollEventsExtendedPanel();
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
