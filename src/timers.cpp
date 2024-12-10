#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "helpers.h"

// We'll use PWM slice 0 for 1ms timer and slice 1 for 5kHz timer
const uint SLOW_TIMER_SLICE = 0;
const uint FAST_TIMER_SLICE = 1;

const uint32_t FAST_TIMER_HZ = 5000;  // 5kHz
const uint32_t SLOW_TIMER_HZ = 1000;  // 1kHz (1ms)

// Separate ISR for 1ms timer
void __not_in_flash_func(pwm_wrap_isr_0)() {
    if (pwm_get_irq_status_mask() & (1u << SLOW_TIMER_SLICE)) {
        pwm_clear_irq(SLOW_TIMER_SLICE);
        g_ms_ticks++;
    }
}

// Separate ISR for 5kHz timer
void __not_in_flash_func(pwm_wrap_isr_1)() {
    if (pwm_get_irq_status_mask() & (1u << FAST_TIMER_SLICE)) {
        pwm_clear_irq(FAST_TIMER_SLICE);
        g_fast_ticks++;
    }
}

void setup_timers() {
    // Get system clock frequency
    uint32_t sys_clock = clock_get_hz(clk_sys);

    // Setup 1ms timer (1kHz) on slice 0
    pwm_config cfg_slow = pwm_get_default_config();
    float div_slow = (float)sys_clock / SLOW_TIMER_HZ;
    pwm_config_set_clkdiv(&cfg_slow, div_slow);
    pwm_config_set_wrap(&cfg_slow, 1);
    pwm_init(SLOW_TIMER_SLICE, &cfg_slow, true);

    // Setup 5kHz timer on slice 1
    pwm_config cfg_fast = pwm_get_default_config();
    float div_fast = (float)sys_clock / FAST_TIMER_HZ;
    pwm_config_set_clkdiv(&cfg_fast, div_fast);
    pwm_config_set_wrap(&cfg_fast, 1);
    pwm_init(FAST_TIMER_SLICE, &cfg_fast, true);

    // Set up interrupt handlers - each PWM slice gets its own IRQ handler
    pwm_set_irq_enabled(SLOW_TIMER_SLICE, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_isr_0);

    pwm_set_irq_enabled(FAST_TIMER_SLICE, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP + 1, pwm_wrap_isr_1);

    // Enable both interrupts
    irq_set_enabled(PWM_IRQ_WRAP, true);
    irq_set_enabled(PWM_IRQ_WRAP + 1, true);
}
