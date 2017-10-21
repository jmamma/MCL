/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"
#include "MidiUartParent.hh"

void MidiUartParent::setSpeed(uint32_t _speed) {
#ifdef TX_IRQ
  // empty TX buffer before switching speed
  while (!txRb.isEmpty())
    ;   
#endif
  speed = _speed;
  uint32_t cpu = (F_CPU / 16);
  cpu /= _speed;
  cpu--;

  //uint32_t cpu = (F_CPU / 16);
  //cpu /= speed;
  //cpu--;
//UBRR0H = ((cpu >> 8));
  if (uart_port == 1) {
  UBRR1H = ((cpu >> 8) & 0xFF);
  UBRR1L = (cpu & 0xFF);
  }
  if (uart_port == 2) {
  UBRR2H = ((cpu >> 8) & 0xFF);
  UBRR2L = (cpu & 0xFF); 
  }
}

void MidiUartParent::sendString(const char *data, uint16_t cnt) {
	sendCommandByte(0xF0);
	uint8_t _data[4] = { MIDIDUINO_SYSEX_VENDOR_1, MIDIDUINO_SYSEX_VENDOR_2,
											 MIDIDUINO_SYSEX_VENDOR_3, CMD_STRING };
	puts(_data, 4);
	puts((uint8_t *)data, cnt);
	sendCommandByte(0xF7);
}


