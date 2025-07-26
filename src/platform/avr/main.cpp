#define IS_ISR_ROUTINE

#include <Arduino.h>
#include <wiring_private.h>
#include "irqs.h"
#include "hardware.h"
#include "platform.h"

//#include <Events.h>
extern "C" {
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
}


void my_init_ram(void) __attribute__((naked)) __attribute__((used))
__attribute__((section(".init3")));

void(* hardwareReset) (void) = 0;

void my_init_ram(void) {
// Set PL6 as output

#ifdef MEGACOMMAND
  DDRL |= _BV(PL6);
  PORTL &= ~(_BV(PL6));
#else
  DDRB |= _BV(PB0);
  PORTB &= ~(_BV(PB0));
#endif

  //External SRAM Hardware Enable
  XMCRA |= _BV(SRE);

  //Leds

  DDRE |= _BV(PE4) | _BV(PE5);
  //SRAM tests

  /* Still not working?
  while (!test_sram_banks()) {
     setLed();
     setLed2();
  }
  */
}
uint8_t tcnt2;

void init(void) {
  /** Disable watchdog. **/
  wdt_disable();
  //  wdt_enable(WDTO_15MS);

  // Configure Port C as 8 channels of output. disable pullup resistors.
  DDRC = 0xFF;
  PORTC = 0x00;

  // Activate level shfiter.

  SET_BIT(DDRD, PD4);
  SET_BIT(PORTD, PD4);

  //For MC SMD. Level shifter 1 + 2 enable.
  //PL4 == MEGA2560 level shifter enable
  //PL3 == Atmega16/32 level shifter enable
  //Only one should be active at any time.

  DDRL |= _BV(PL4) | _BV(PL3);

  LOCAL_SPI_ENABLE();

  setup_timers();

}

void setup();
void loop();

int main(void) {


  //PK0 PK1 PK2 are MegaCMD control lines to the Atmega32u2
  //Not available on the DIY version.
  PORTK = 0x00;
  PORTK |= _BV(PK0) | _BV(PK1) | _BV(PK2); //enable pullup or set high

  DDRK = 0x00;
  DDRK |= _BV(PK1) | _BV(PK0); //set output

  init();

  sei();
  delay(100);

// Set SD card select HIGH before initialising OLED.
#ifdef MEGACOMMAND
  PORTB |= (1 << PB0);
#else
  PORTE |= (1 << PE7);
#endif
  setup();
  for (;;) {
    loop();
  }
  return 0;
}

