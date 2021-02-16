#include "MCL_impl.h"

uint8_t TurboLight::lookup_speed(uint8_t speed) {
  switch (speed) {
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

void TurboLight::send_header(uint8_t cmd, MidiUartClass *MidiUart_) {
  for (uint8_t x = 0 ; x  < 6; x++) {
    MidiUart_->m_putc_immediate(turbomidi_sysex_header[x]);

  }

  MidiUart_->m_putc_immediate(cmd);
}

void TurboLight::set_speed(uint8_t speed, uint8_t port) {
  MidiUartClass *MidiUart_;

  if (port == 1) {
    MidiUart_ = (MidiUartClass*) &MidiUart;
  }
  else {
    MidiUart_ = (MidiUartClass*) &MidiUart2;
  }

  if (MidiUart_->speed == tmSpeeds[speed]) {
    return;
  }

  //USE_LOCK();
  // SET_LOCK();

  send_header(0x20, MidiUart_);
  MidiUart_->m_putc_immediate(speed);

  MidiUart_->m_putc_immediate(0xF7);
  #ifdef MEGACOMMAND
  if (port == 1) {
    while (!IS_BIT_SET8(UCSR1A, UDRE1));
  }
  else {
    while (!IS_BIT_SET8(UCSR2A, UDRE2));
  }
  #else
  if (port == 1) {
    while (!IS_BIT_SET8(UCSR0A, UDRE1));
  }
  else {
    while (!IS_BIT_SET8(UCSR1A, UDRE1));
  }

  #endif
  for (uint8_t n = 0; n < 16; n++) {
  MidiUart_->m_putc_immediate(0x00);
  }
  MidiUart.set_speed(tmSpeeds[speed ], port);
  //delay(50);
  if (speed <= 1) {
  MidiUart_->activeSenseEnabled = false;
  }
  else {
  MidiUart_->setActiveSenseTimer(150);
  }
  //  MidiUart_->m_putc_immediate(0xF8);
  //MidiUart_->m_putc_immediate(0xFE);

  // MidiUart_->sendActiveSenseTimer = 10;
  //   CLEAR_LOCK();

}



TurboLight turbo_light;
