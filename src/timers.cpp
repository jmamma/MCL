#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "helpers.h"
#include "platform.h"
#include "MidiClock.h"
#include "global.h"

// We'll use PWM slice 4 for 1ms timer and slice 5 for 5kHz timer
const uint SLOW_TIMER_SLICE = 4;
const uint FAST_TIMER_SLICE = 5;

const uint32_t FAST_TIMER_HZ = 5000; // 5kHz
const uint32_t SLOW_TIMER_HZ = 1000; // 1kHz (1ms)

// Shared ISR that handles both timers
void __not_in_flash_func(pwm_wrap_handler)() {
  // Check which slice triggered the interrupt
  uint32_t mask = pwm_get_irq_status_mask();

  if (mask & (1u << SLOW_TIMER_SLICE)) {
    pwm_clear_irq(SLOW_TIMER_SLICE);
    g_ms_ticks++;
  }

  if (mask & (1u << FAST_TIMER_SLICE)) {
    pwm_clear_irq(FAST_TIMER_SLICE);
    g_fast_ticks++;

    MidiClock.div192th_countdown++;
    if (MidiClock.state == MidiClockClass::STARTED) {
      if (MidiClock.div192th_countdown >= MidiClock.div192_time) {
        if (MidiClock.div192th_counter != MidiClock.div192th_counter_last) {
          MidiClock.increment192Counter();
          MidiClock.div192th_countdown = 0;
          MidiClock.div192th_counter_last = MidiClock.div192th_counter;
          if (MidiClock.inCallback) {
            return;
          }
          MidiClock.inCallback = true;
          uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
          MidiUartParent::handle_midi_lock = 1;
          //mcl_seq.seq(); ... to do
          MidiUartParent::handle_midi_lock = _midi_lock_tmp;
          MidiClock.inCallback = false;
        }
      }
    }

    if (!MidiUartParent::handle_midi_lock) {
      MidiUartParent::handle_midi_lock = 1;
      handleIncomingMidi();
      MidiUartParent::handle_midi_lock = 0;
    }
  }
}

void setup_timers() {
  // Get system clock frequency
  uint32_t sys_clock = clock_get_hz(clk_sys);
  DEBUG_PRINTLN("System clock: ");
  DEBUG_PRINTLN(sys_clock);
  // For 1kHz (1ms) timer:
  // Want exactly 1000Hz from 125MHz
  // 125,000,000 / 1000 = 125,000 total divisor needed
  // Let's use counter=1000, so clock div = 125
  pwm_config cfg_slow = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg_slow,
                            125); // Use integer division for better stability
  pwm_config_set_wrap(&cfg_slow, 999); // Count from 0-999 = 1000 steps
  pwm_init(SLOW_TIMER_SLICE, &cfg_slow, true);

  // For 5kHz:
  // 125,000,000 / 5000 = 25,000 total divisor
  // Let's use counter=200, so clock div = 125
  pwm_config cfg_fast = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg_fast, 125); // Same 1MHz base clock
  pwm_config_set_wrap(&cfg_fast, 199);       // Count from 0-199 = 200 steps
  pwm_init(FAST_TIMER_SLICE, &cfg_fast, true);

  // Set up single shared interrupt handler for both PWM slices
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  // Enable interrupts for both slices
  pwm_set_irq_enabled(SLOW_TIMER_SLICE, true);
  pwm_set_irq_enabled(FAST_TIMER_SLICE, true);
}
