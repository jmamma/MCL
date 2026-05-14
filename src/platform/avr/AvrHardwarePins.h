#pragma once

#include "hardware.h"

#include <avr/io.h>
#include <stdint.h>

#define MCL_OLED_RESET_PIN 38
#define MCL_OLED_CS_PIN 42
#define MCL_OLED_DC_PIN 44

namespace mcl_avr_pins {

static constexpr uint8_t sd_cs_pin = SPI1_SS_PIN;
static constexpr uint8_t oled_reset_pin = MCL_OLED_RESET_PIN;
static constexpr uint8_t oled_cs_pin = MCL_OLED_CS_PIN;
static constexpr uint8_t oled_dc_pin = MCL_OLED_DC_PIN;

inline void sd_cs_output() {
#ifdef MEGACOMMAND
  DDRB |= _BV(PB0);
#else
  DDRE |= _BV(PE7);
#endif
}

inline void sd_cs_write(bool level) {
#ifdef MEGACOMMAND
  if (level) {
    PORTB |= _BV(PB0);
  } else {
    PORTB &= ~_BV(PB0);
  }
#else
  if (level) {
    PORTE |= _BV(PE7);
  } else {
    PORTE &= ~_BV(PE7);
  }
#endif
}

inline void oled_pins_output() {
  DDRD |= _BV(PD7);
  DDRL |= _BV(PL7) | _BV(PL5);
}

inline void oled_cs_high() { PORTL |= _BV(PL7); }
inline void oled_cs_low() { PORTL &= ~_BV(PL7); }
inline void oled_dc_high() { PORTL |= _BV(PL5); }
inline void oled_dc_low() { PORTL &= ~_BV(PL5); }
inline void oled_reset_high() { PORTD |= _BV(PD7); }
inline void oled_reset_low() { PORTD &= ~_BV(PD7); }

} // namespace mcl_avr_pins
