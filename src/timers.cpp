#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "helpers.h"
#include "platform.h"

// We'll use PWM slice 0 for 1ms timer and slice 1 for 5kHz timer
const uint SLOW_TIMER_SLICE = 0;
const uint FAST_TIMER_SLICE = 1;

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
    pwm_config_set_clkdiv_int(&cfg_slow, 125);    // Use integer division for better stability
    pwm_config_set_wrap(&cfg_slow, 999);          // Count from 0-999 = 1000 steps
    pwm_init(SLOW_TIMER_SLICE, &cfg_slow, true);

    // For 5kHz:
    // 125,000,000 / 5000 = 25,000 total divisor
    // Let's use counter=200, so clock div = 125
    pwm_config cfg_fast = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&cfg_fast, 125);    // Same 1MHz base clock
    pwm_config_set_wrap(&cfg_fast, 199);          // Count from 0-199 = 200 steps
    pwm_init(FAST_TIMER_SLICE, &cfg_fast, true);

    /*
  // For 1kHz (1ms period)
  // We want the counter to overflow every 1ms
  // So if we use a divider of 125, counter needs to go to 1000
  pwm_config cfg_slow = pwm_get_default_config();
  pwm_config_set_clkdiv(&cfg_slow, 125.0f); // Divide 125MHz by 125 = 1MHz
  pwm_config_set_wrap(&cfg_slow, 1000);     // 1MHz / 1000 = 1kHz
  pwm_init(SLOW_TIMER_SLICE, &cfg_slow, true);

  // For 5kHz (200μs period)
  pwm_config cfg_fast = pwm_get_default_config();
  pwm_config_set_clkdiv(&cfg_fast, 125.0f); // Again get 1MHz
  pwm_config_set_wrap(&cfg_fast, 200);      // 1MHz / 200 = 5kHz
  pwm_init(FAST_TIMER_SLICE, &cfg_fast, true);
*/
  // Set up single shared interrupt handler for both PWM slices
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  // Enable interrupts for both slices
  pwm_set_irq_enabled(SLOW_TIMER_SLICE, true);
  pwm_set_irq_enabled(FAST_TIMER_SLICE, true);
}
