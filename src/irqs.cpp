#include "MCLSeq.h"
#include "MidiClock.h"
#include "global.h"
#include "helpers.h"
#include "platform.h"
#include "tusb.h" // Add at top of file if not already there

// We'll use PWM slice 4 for 1ms timer and slice 5 for 5kHz timer
const uint SLOW_TIMER_SLICE = 4;
const uint FAST_TIMER_SLICE = 5;

const uint32_t FAST_TIMER_HZ = 5000; // 5kHz
const uint32_t SLOW_TIMER_HZ = 1000; // 1kHz (1ms)

void __not_in_flash_func(softirq1_handler)() {
  CLEAR_SW_IRQ1();

  MidiClock.inCallback = true;
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;
  mcl_seq.seq();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
  MidiClock.inCallback = false;
}

void __not_in_flash_func(softirq2_handler)() {
  CLEAR_SW_IRQ2();
  handleIncomingMidi();
}

void __not_in_flash_func(softirq3_handler)() {
  CLEAR_SW_IRQ3();
  GUI_hardware.poll();
}

void __not_in_flash_func(timer1_handler)() {
  LOCK();
  g_clock_ms++;
  g_clock_ticks++;

  if (g_clock_ticks == 60000) {
    g_clock_ticks = 0;
    g_clock_minutes++;
  }

  MidiUart.tickActiveSense();
  MidiUart2.tickActiveSense();

  TRIGGER_SW_IRQ3();
  CLEAR_LOCK();
}

void __not_in_flash_func(timer2_handler)() {
  LOCK();
  g_clock_fast++;

  MidiClock.div192th_countdown++;
  if (MidiClock.state == MidiClockClass::STARTED) {
    if (MidiClock.div192th_countdown >= MidiClock.div192_time) {
      if (MidiClock.div192th_counter != MidiClock.div192th_counter_last) {
        MidiClock.increment192Counter();
        MidiClock.div192th_countdown = 0;
        MidiClock.div192th_counter_last = MidiClock.div192th_counter;
        if (!MidiClock.inCallback) {
          TRIGGER_SW_IRQ1();
        }
      }
    }
  }
  if (!MidiUartParent::handle_midi_lock) {
    TRIGGER_SW_IRQ2();
  }
  CLEAR_LOCK();
}

// Shared ISR that handles both timers
void __not_in_flash_func(pwm_wrap_handler)() {
  // Check which slice triggered the interrupt
  uint32_t mask = pwm_get_irq_status_mask();

  if (mask & (1u << SLOW_TIMER_SLICE)) {
    pwm_clear_irq(SLOW_TIMER_SLICE);
    timer1_handler();
  }

  if (mask & (1u << FAST_TIMER_SLICE)) {
    pwm_clear_irq(FAST_TIMER_SLICE);
    timer2_handler();
  }
}

void setup_timers() {
  // Get system clock frequency
  uint32_t sys_clock = clock_get_hz(clk_sys);
  DEBUG_PRINTLN("System clock: ");
  DEBUG_PRINTLN(sys_clock);

  // For 1kHz (1ms) timer:
  const uint32_t total_div_slow = F_CPU / SLOW_TIMER_HZ;
  const uint16_t wrap_slow = 999; // Count from 0-999 = 1000 steps
  const uint16_t clkdiv_slow = total_div_slow / (wrap_slow + 1);

  pwm_config cfg_slow = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg_slow, clkdiv_slow);
  pwm_config_set_wrap(&cfg_slow, wrap_slow);
  pwm_init(SLOW_TIMER_SLICE, &cfg_slow, true);

  // For 5kHz timer:
  const uint32_t total_div_fast = F_CPU / FAST_TIMER_HZ;
  const uint16_t wrap_fast = 199; // Count from 0-199 = 200 steps
  const uint16_t clkdiv_fast = total_div_fast / (wrap_fast + 1);

  pwm_config cfg_fast = pwm_get_default_config();
  pwm_config_set_clkdiv_int(&cfg_fast, clkdiv_fast);
  pwm_config_set_wrap(&cfg_fast, wrap_fast);
  pwm_init(FAST_TIMER_SLICE, &cfg_fast, true);

  irq_set_priority(PWM_IRQ_WRAP, 0x10);
  // Set up single shared interrupt handler for both PWM slices
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  // Enable interrupts for both slices
  pwm_set_irq_enabled(SLOW_TIMER_SLICE, true);
  pwm_set_irq_enabled(FAST_TIMER_SLICE, true);
}

void setup_software_interrupts() {
  // Claim three consecutive user IRQs
  SW_IRQ1 = user_irq_claim_unused(true);
  SW_IRQ2 = user_irq_claim_unused(true);
  SW_IRQ3 = user_irq_claim_unused(true);

  // Priority hierarchy:
  // PWM   (0x10) - Highest
  // UART  (0x20) - Middle
  // SW Interrupts (0x30) - LowestM

  irq_set_priority(SW_IRQ1, 0x30);
  irq_set_priority(SW_IRQ2, 0x30);
  irq_set_priority(SW_IRQ3, 0x30);

  irq_set_exclusive_handler(SW_IRQ1, softirq1_handler);
  irq_set_exclusive_handler(SW_IRQ2, softirq2_handler);
  irq_set_exclusive_handler(SW_IRQ3, softirq3_handler);
  // Enable the interrupts in NVIC
  irq_set_enabled(SW_IRQ1, true);
  irq_set_enabled(SW_IRQ2, true);
  irq_set_enabled(SW_IRQ3, true);
}

void setup_irqs() {
  setup_timers();
  setup_software_interrupts();
}

uint8_t SW_IRQ1;
uint8_t SW_IRQ2;
uint8_t SW_IRQ3;
