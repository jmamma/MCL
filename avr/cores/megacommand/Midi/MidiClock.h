
/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDICLOCK_H__
#define MIDICLOCK_H__

#include "Callback.hh"
#include "Vector.hh"
#include "WProgram.h"
#include "midi-common.hh"
#include <inttypes.h>

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_clock Midi Clock
 *
 * @{
 **/

class ClockCallback {};

typedef void (ClockCallback::*midi_clock_callback_ptr_t)(uint32_t count);

class MidiClockClass {
  /**
   * \addtogroup midi_clock
   *
   * @{
   **/
protected:
  float tempo;
public:
  volatile uint32_t div192th_counter_last;
  volatile uint32_t div192th_counter;

  volatile uint32_t div96th_counter_last;
  volatile uint32_t div96th_counter;
  volatile uint32_t div32th_counter;
  volatile uint16_t div16th_counter;

  volatile uint8_t mod12_counter;
  volatile uint8_t mod6_counter;
  volatile uint8_t mod3_counter;
  volatile uint8_t mod6_free_counter;

  volatile uint16_t clock_last_time;
  volatile uint16_t div192th_time;
  volatile uint16_t last_clock16;

  volatile uint16_t last_diff_clock16;
  volatile uint16_t diff_clock16;

  volatile uint8_t bar_counter;
  volatile uint8_t beat_counter;
  volatile uint8_t step_counter;

  volatile uint8_t inmod6_counter;

  volatile uint16_t interval;

  volatile uint16_t counter;
  volatile uint16_t rx_phase;

  volatile uint16_t rx_last_clock;
  volatile uint16_t rx_clock;
  volatile bool doUpdateClock;

  volatile bool useImmediateClock;

  volatile bool updateSmaller;
  uint16_t pll_x;
  // bool transmit;
  bool transmit_uart1;
  bool transmit_uart2;
  bool isInit;

  //    volatile uint16_t mcl_clock;
  //   volatile uint16_t mcl_countbool
  volatile enum {
    PAUSED = 0,
    STARTING = 1,
    STARTED = 2,
  } state;

#if defined(MIDIDUINO) || defined(HOST_MIDIDUINO)

  typedef enum {
    OFF = 0,
    INTERNAL_M,
    EXTERNAL_UART1,
    EXTERNAL_UART2
  } clock_mode_t;
#define INTERNAL_MIDI INTERNAL_M
#define EXTERNAL_MIDI EXTERNAL_UART1

#else
  typedef enum {
    OFF = 0,
    INTERNAL_MIDI,
    EXTERNAL_UART1,
    EXTERNAL_UART2
  } clock_mode_t;
  // arduino

#ifndef BOARD_ID
#define BOARD_ID 0x80
#endif

#endif

  clock_mode_t mode;

  MidiClockClass();

  CallbackVector1<ClockCallback, 8, uint32_t> onMidiStartCallbacks;

  CallbackVector1<ClockCallback, 8, uint32_t> onMidiStartImmediateCallbacks;
  CallbackVector1<ClockCallback, 8, uint32_t> onMidiStopCallbacks;
  CallbackVector1<ClockCallback, 8, uint32_t> onMidiContinueCallbacks;

  CallbackVector1<ClockCallback, 8, uint32_t> on192Callbacks;
  CallbackVector1<ClockCallback, 8, uint32_t> on96Callbacks;
  CallbackVector1<ClockCallback, 8, uint32_t> on32Callbacks;
  CallbackVector1<ClockCallback, 8, uint32_t> on16Callbacks;

  CallbackVector1<ClockCallback, 8, uint32_t> onClockCallbacks;
  void addOnMidiStartImmediateCallback(ClockCallback *obj,
                                       midi_clock_callback_ptr_t func) {
    onMidiStartImmediateCallbacks.add(obj, func);
  }
  void removeOnMidiStartImmediateCallback(ClockCallback *obj,
                                          midi_clock_callback_ptr_t func) {
    onMidiStartImmediateCallbacks.remove(obj, func);
  }
  void removeOnMidiStartImmediateCallback(ClockCallback *obj) {
    onMidiStartImmediateCallbacks.remove(obj);
  }

  void addOnMidiStartCallback(ClockCallback *obj,
                              midi_clock_callback_ptr_t func) {
    onMidiStartCallbacks.add(obj, func);
  }
  void removeOnMidiStartCallback(ClockCallback *obj,
                                 midi_clock_callback_ptr_t func) {
    onMidiStartCallbacks.remove(obj, func);
  }
  void removeOnMidiStartCallback(ClockCallback *obj) {
    onMidiStartCallbacks.remove(obj);
  }
  void addOnMidiStopCallback(ClockCallback *obj,
                             midi_clock_callback_ptr_t func) {
    onMidiStopCallbacks.add(obj, func);
  }
  void removeOnMidiStopCallback(ClockCallback *obj,
                                midi_clock_callback_ptr_t func) {
    onMidiStopCallbacks.remove(obj, func);
  }
  void removeOnMidiStopCallback(ClockCallback *obj) {
    onMidiStopCallbacks.remove(obj);
  }
  void addOnMidiContinueCallback(ClockCallback *obj,
                                 midi_clock_callback_ptr_t func) {
    onMidiContinueCallbacks.add(obj, func);
  }
  void removeOnMidiContinueCallback(ClockCallback *obj,
                                    midi_clock_callback_ptr_t func) {
    onMidiContinueCallbacks.remove(obj, func);
  }
  void removeOnMidiContinueCallback(ClockCallback *obj) {
    onMidiContinueCallbacks.remove(obj);
  }
  void addOn192Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on192Callbacks.add(obj, func);
  }
  void removeOn192Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on192Callbacks.remove(obj, func);
  }
  void removeOn192Callback(ClockCallback *obj) { on192Callbacks.remove(obj); }

  void addOn96Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on96Callbacks.add(obj, func);
  }
  void removeOn96Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on96Callbacks.remove(obj, func);
  }
  void removeOn96Callback(ClockCallback *obj) { on96Callbacks.remove(obj); }

  void addOn32Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on32Callbacks.add(obj, func);
  }
  void removeOn32Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on32Callbacks.remove(obj, func);
  }
  void removeOn32Callback(ClockCallback *obj) { on32Callbacks.remove(obj); }

  void addOn16Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on16Callbacks.add(obj, func);
  }
  void removeOn16Callback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    on16Callbacks.remove(obj, func);
  }
  void removeOn16Callback(ClockCallback *obj) { on16Callbacks.remove(obj); }

  void addOnClockCallback(ClockCallback *obj, midi_clock_callback_ptr_t func) {
    onClockCallbacks.add(obj, func);
  }
  void removeOnClockCallback(ClockCallback *obj,
                             midi_clock_callback_ptr_t func) {
    onClockCallbacks.remove(obj, func);
  }
  void removeOnClockCallback(ClockCallback *obj) {
    onClockCallbacks.remove(obj);
  }

  ALWAYS_INLINE() void init();
  ALWAYS_INLINE() void callCallbacks() {
    if (state != STARTED)
      return;

    // Moved MidiClock callbacks to Main Loop

    static bool inCallback = false;
    if (inCallback) {
      DEBUG_PRINTLN("clock collision");
      return;
    } else {
      inCallback = true;
    }

#ifndef HOST_MIDIDUINO
    sei();
#endif
    // HOST_MIDIDUINO/

    on192Callbacks.call(div192th_counter);

    if (mod6_counter == 0) {
      on16Callbacks.call(div16th_counter);
      on32Callbacks.call(div32th_counter);
      // mcl_16counter++;
    }
    if (mod6_counter == 3) {
      on32Callbacks.call(div32th_counter);
    }

    inCallback = false;
  }

 ALWAYS_INLINE() void handleImmediateClock() {
    // if (clock > clock_last_time) {
    //  div192th_time = (clock - clock_last_time) / 2;
    //   DEBUG_PRINTLN( (clock - clock_last_time) / 2);

    // }
    clock_last_time = clock;
    uint8_t _mod6_counter = mod6_counter;

    if (transmit_uart1) {
      //       MidiUart.putc(0xF8);
      MidiUart.m_putc_immediate(0xF8);
    }
    if (transmit_uart2) {
      MidiUart2.m_putc_immediate(0xF8);
    }
    incrementCounters();
    if ((step_counter == 1) && (state == STARTED)) {
      setLed();
    } else {
      clearLed();
    }

    // callCallbacks();
  }

  /* in interrupt on receiving 0xF8 */
 ALWAYS_INLINE() void handleClock() {

    if (useImmediateClock) {
      handleImmediateClock();
      return;
    }
  }
 ALWAYS_INLINE() void increment192Counter() {
    if (state == STARTED) {
      div192th_counter++;
      mod12_counter++;
    }
  }
  uint16_t midi_clock_diff(uint16_t old_clock, uint16_t new_clock) {
    if (new_clock >= old_clock)
      return new_clock - old_clock;
    else
      return new_clock + (0xFFFF - old_clock);
  }

  void calc_tempo() {
    if (last_diff_clock16 != diff_clock16) {
      tempo = ((float)75000 / ((float)diff_clock16));
      last_diff_clock16 = diff_clock16;
    }
  }

  float get_tempo() {
    calc_tempo();
    return tempo;
  }

  ALWAYS_INLINE() void MidiClockClass::incrementCounters() {
    mod6_free_counter++;
    if (mod6_free_counter == 6) {
      diff_clock16 = midi_clock_diff(last_clock16, clock);
      div192th_time = diff_clock16 * .08333;
      mod6_free_counter = 0;
      last_clock16 = clock;
    }
   if (state == STARTED) {
      div96th_counter++;
      mod6_counter++;
      mod12_counter++;
      mod3_counter++;
      div192th_counter++;
      if (mod3_counter == 3) {
        mod3_counter = 0;
      }
      if (mod6_counter == 6) {
        step_counter++;
        mod6_counter = 0;
        mod12_counter = 0;
        div16th_counter++;
        div32th_counter++;
        // div32th counter should be at most 2x div16th_counter
        if (div16th_counter == 0) {
          div32th_counter = 0;
          div96th_counter = 0;
          div192th_counter = 0;
        }
      } else if (mod6_counter == 3) {
        div32th_counter++;
      }
      if (step_counter == 5) {
        step_counter = 1;
        beat_counter++;
      }
      if (beat_counter == 5) {
        beat_counter = 1;
        bar_counter++;
      }
      if (bar_counter == 101) {
        bar_counter = 1;
      }
    }
      else if (state == STARTING && (mode == INTERNAL_MIDI || useImmediateClock)) {
      state = STARTED;
      callCallbacks();
    }
 
  }
  void updateClockInterval();
  bool clock_less_than(uint16_t a, uint16_t b);
  bool clock_less_than(uint32_t a, uint32_t b);
  uint32_t clock_diff_div192(uint32_t old_clock, uint32_t new_clock);

  ALWAYS_INLINE() void MidiClockClass::handleImmediateMidiStart() {
    if (transmit_uart1) {
      MidiUart.sendRaw(MIDI_START);
    }
    if (transmit_uart2) {
      MidiUart2.sendRaw(MIDI_START);
    }
    init();

    onMidiStartImmediateCallbacks.call(div96th_counter);
    state = STARTING;

    DEBUG_PRINTLN("START");
  }

  ALWAYS_INLINE() void MidiClockClass::handleImmediateMidiStop() {
    state = PAUSED;
    if (transmit_uart1) {
      MidiUart.sendRaw(MIDI_STOP);
    }
    if (transmit_uart2) {
      MidiUart2.sendRaw(MIDI_STOP);
    }

    //  init();
  }

  ALWAYS_INLINE() void MidiClockClass::handleImmediateMidiContinue() {
    if (transmit_uart1) {
      MidiUart.sendRaw(MIDI_CONTINUE);
    }
    if (transmit_uart2) {
      MidiUart2.sendRaw(MIDI_CONTINUE);
    }
    state = STARTED;

    isInit = false;
    //  init();
  }

  void handleMidiStart();
  void handleMidiContinue();
  void handleMidiStop();
  void handleTimerInt();
  void handleSongPositionPtr(uint8_t *msg);
  void setSongPositionPtr(uint16_t pos);

  void start();
  void stop();
  void pause();
  void setTempo(uint16_t tempo);
  bool getBlinkHint(bool onbeat);

  bool isStarted() { return state == STARTED; }

  /* @} */
};

extern MidiClockClass MidiClock;

/* @} @} */

#endif /* MIDICLOCK_H__ */
