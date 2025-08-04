#include "MCLSeq.h"
#include "MidiClock.h"
#include "global.h"
#include "helpers.h"
#include "platform.h"
#include "helpers.h"

ISR(TIMER1_COMPA_vect) {
  select_bank(BANK0);

  g_clock_fast++;
  MidiClock.div192th_countdown++;
  if (MidiClock.state == 2) {
    if (MidiClock.div192th_countdown >= MidiClock.div192_time) {
      if (MidiClock.div192th_counter != MidiClock.div192th_counter_last) {
        MidiClock.increment192Counter();
        MidiClock.div192th_countdown = 0;
        MidiClock.div192th_counter_last = MidiClock.div192th_counter;
        if (MidiClock.inCallback) { return; }
        MidiClock.inCallback = true;
        uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
        MidiUartParent::handle_midi_lock = 1;
        sei();
        mcl_seq.seq();
        MidiUartParent::handle_midi_lock = _midi_lock_tmp;
        MidiClock.inCallback = false;
      }
    }
  }

  if (!MidiUartParent::handle_midi_lock)  {
   uint8_t _irqlock_tmp = SREG;
   MidiUartParent::handle_midi_lock = 1;
   sei();
   handleIncomingMidi();
   SREG = _irqlock_tmp;
   MidiUartParent::handle_midi_lock = 0;
  }
}

ISR(TIMER3_COMPA_vect) {
  select_bank(BANK0);

  g_clock_ms++;
  g_clock_ticks++;

  if (g_clock_ticks == 60000) {
    g_clock_ticks = 0;
    g_clock_minutes++;
  }

  MidiUart.tickActiveSense();
  MidiUart2.tickActiveSense();
  MidiUartUSB.tickActiveSense();

  sei();
  GUI_hardware.poll();
}

void setup_timers() {
  TCCR1A = _BV(WGM10);              //  | _BV(COM1A1) | _BV(COM1B1);
  TCCR1B |= _BV(CS10) | _BV(WGM12); // every cycle

  // http://www.arduinoslovakia.eu/application/timer-calculator
  // Microcontroller: ATmega2560
  // Created: 2017-10-28T08:18:15.310Z

  // Clear registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // 5000 Hz (16000000/((49+1)*64))
  OCR1A = 49;
  // CTC
  TCCR1B |= (1 << WGM12);
  // Prescaler 64
  TCCR1B |= (1 << CS11) | (1 << CS10);
// Output Compare Match A Interrupt Enable
#ifdef MEGACOMMAND
  TIMSK1 |= (1 << OCIE1A);
#else
  TIMSK |= (1 << OCIE1A);
#endif
  // TCCR2A = _BV(WGM20) | _BV(WGM21) | _BV(CS20) | _BV(CS21); // ) | _BV(CS21);
  // // | _BV(COM21);

  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;
  // 1000 Hz (16000000/((249+1)*64))
  OCR3A = 249;
  // CTC
  TCCR3B |= (1 << WGM32);
  // Prescaler 64
  TCCR3B |= (1 << CS31) | (1 << CS30);
// Output Compare Match A Interrupt Enable
#ifdef MEGACOMMAND
  TIMSK3 |= (1 << OCIE3A);
#else
  ETIMSK |= (1 << OCIE3A);
#endif
}

void setup_irqs() {
}


