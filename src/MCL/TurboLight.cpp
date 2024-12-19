#include "TurboLight.h"

uint8_t TurboLight::lookup_speed(uint8_t speed) {
  switch (speed) {
    default:
    case 0:
      return 1;

    case 1:
      return 2;

    case 2:
      return 4;

    case 3:
      return 7;
  }
}

void TurboLight::send_header(uint8_t cmd, MidiUartClass *uart) {
  for (uint8_t x = 0 ; x  < 6; x++) {
    uart->m_putc_immediate(turbomidi_sysex_header[x]);

  }

  uart->m_putc_immediate(cmd);
}

void TurboLight::set_speed(uint8_t speed, MidiUartClass *uart) {
  if (uart->speed == tmSpeeds[speed]) {
    DEBUG_PRINTLN(F("same speed"));
    return;
  }
  DEBUG_PRINTLN(F("turbo time"));
  send_header(0x20, uart);
  uart->m_putc_immediate(speed);

  uart->m_putc_immediate(0xF7);

  while (!uart->check_empty_tx());

  for (uint8_t n = 0; n < 16; n++) {
    uart->m_putc_immediate(0x00);
  }
  uart->set_speed(tmSpeeds[speed ]);
  //delay(50);
  if (speed <= 1) {
    uart->activeSenseEnabled = false;
  }
  else {
    uart->setActiveSenseTimer(150);
  }

}



TurboLight turbo_light;
