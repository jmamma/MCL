
/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDICLOCK_H__
#define MIDICLOCK_H__

#include "Callback.h"
#include "Vector.h"
#include "WProgram.h"
#include "midi-common.h"
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

  volatile uint8_t mod8_free_counter;
  volatile uint16_t div192_time;
  volatile uint8_t div192th_countdown;
  volatile uint16_t clock_last_time;

  volatile uint16_t last_diff_clock8;
  volatile uint16_t diff_clock8;
  volatile uint16_t last_clock8;

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

  bool reset_clock_phase = false;
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

  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> onMidiStartCallbacks;

  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> onMidiStartImmediateCallbacks;
  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> onMidiStopCallbacks;
  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> onMidiContinueCallbacks;

  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> on192Callbacks;
  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> on96Callbacks;
  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> on32Callbacks;
  CallbackVector1<ClockCallback, NUM_CLOCK_CALLBACKS, uint32_t> on16Callbacks;

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

  ALWAYS_INLINE() void init();

  volatile bool inCallback = false;

  ALWAYS_INLINE() void callCallbacks(bool isMidiEvent = false) {
    if (state != STARTED)
      return;

    if (inCallback) {
      DEBUG_PRINTLN(F("clock collision"));
      return;
    }

    inCallback = true;

    uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
    uint8_t _irqlock_tmp = SREG;
    MidiUartParent::handle_midi_lock = 1;

    sei();

    on192Callbacks.call(div192th_counter);

    if (isMidiEvent) {
      on96Callbacks.call(div96th_counter);

      if (mod6_counter == 0) {
        on16Callbacks.call(div16th_counter);
        on32Callbacks.call(div32th_counter);
      }
      if (mod6_counter == 3) {
        on32Callbacks.call(div32th_counter);
      }
    }

    inCallback = false;
    SREG = _irqlock_tmp;
    MidiUartParent::handle_midi_lock = _midi_lock_tmp;
  }

  ALWAYS_INLINE() void handleImmediateClock() {
    // if (clock > clock_last_time) {
    //  div192th_time = (clock - clock_last_time) / 2;
    //   DEBUG_PRINTLN( (clock - clock_last_time) / 2);

    // }
    clock_last_time = clock;
    div192th_countdown = 0;
    if (transmit_uart1) {
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

  /* in interrupt on 5000Hz internal timer timeout */
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
    // DEBUG_PRINTLN(diff_clock8);
    if (last_diff_clock8 != diff_clock8) {
      tempo = 100000.0f / diff_clock8;
      last_diff_clock8 = diff_clock8;
    }
  }

  float get_tempo() {
    calc_tempo();
    return tempo;
  }

  /* in interrupt, called on receiving MIDI_CLOCK
   *
   * MIDI_CLOCK is sent at 24PPQN, that is, 6 pulses per step, or 96 pulses per
   * bar.
   *
   * clock: incrementing at 5KHz.
   * mod8_free_counter: divides 24PPQN to 3PPQN, that is, 3/4 pulses per step
   * diff_clock8: measures clock ticks between two mod8.
   *  - because mod6 is div16th, mod8 is div12th.
   * div192_time: clock ticks for what divides 3PPQN time by 16, that is,
   * 48PPQN. div192th_countdown: incrementing at 5KHz, and triggers
   * increment192Counter.
   *
   * For example, assume BPM=120:
   * - 8 steps per second
   * - 48 midi clock pulses per second
   * - 6 clock_mod_8 pulses per second
   * For a 5KHz clock, it means diff_clock8 should be 833.333
   *
   * === calc_tempo ===
   * One minute @ 5KHz is 30K pulses
   * One beat is 4 steps, that is, 3 * div8 pulses.
   * BPM = 30K / 3 / diff_clock8 = 10K / diff_clock8
   *
   * === playing step-related counters ===
   * mod6_counter: divides 24PPQN to 4PPQN, that is, 1 pulse per step.
   * mod12_counter: divides 24PPQN to 2PPQN, that is, 1/2 pulses per step.
   * step_counter: a single step [1..4], reset at 5
   * div16th_counter: same speed as step_counter, only reset at init or
   * overflow. div32th_counter: 1/2 step. div96th_counter: 1/6 step, one
   * MIDI_CLOCK pulse. div192th_counter: 1/12 step, half MIDI_CLOCK pulse.
   */
  ALWAYS_INLINE() void incrementCounters() {
    mod8_free_counter++;
    if (reset_clock_phase) {
      mod8_free_counter = 0;
      last_clock8 = clock;
      reset_clock_phase = false;
    }
    if (mod8_free_counter == 8) {
      diff_clock8 = midi_clock_diff(last_clock8, clock);
      last_clock8 = clock;
      div192_time = diff_clock8 / 16;
      mod8_free_counter = 0;
    }
    if (state == STARTED) {
      div96th_counter++;
      mod6_counter++;
      mod12_counter++;
      div192th_counter++;
      if (mod6_counter == 6) {
        // one step
        step_counter++;
        mod6_counter = 0;
        mod12_counter = 0;
        div16th_counter++;
        div32th_counter++;
        // div32th counter should be at most 2x div16th_counter
        // on div16th overflow, also reset 32th, 96th and 192th.
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
    } else if (state == STARTING &&
               (mode == INTERNAL_MIDI || useImmediateClock)) {
      state = STARTED;
    }
  }

  void updateClockInterval();
  bool clock_less_than(uint16_t a, uint16_t b);
  bool clock_less_than(uint32_t a, uint32_t b);
  uint32_t clock_diff_div192(uint32_t old_clock, uint32_t new_clock);

  ALWAYS_INLINE() void handleImmediateMidiStart() {
    reset_clock_phase = true;

    if (transmit_uart1) {
      MidiUart.sendRaw(MIDI_START);
    }
    if (transmit_uart2) {
      MidiUart2.sendRaw(MIDI_START);
    }
    init();

    onMidiStartImmediateCallbacks.call(div96th_counter);
    state = STARTING;

    DEBUG_PRINTLN(F("START"));
  }

  ALWAYS_INLINE() void handleImmediateMidiStop() {
    state = PAUSED;
    if (transmit_uart1) {
      MidiUart.sendRaw(MIDI_STOP);
    }
    if (transmit_uart2) {
      MidiUart2.sendRaw(MIDI_STOP);
    }

    //  init();
  }

  ALWAYS_INLINE() void handleImmediateMidiContinue() {
    reset_clock_phase = true;
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
