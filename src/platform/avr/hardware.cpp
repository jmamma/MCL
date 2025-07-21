#include "MidiUart.h"
#include "global.h"

void change_usb_mode(uint8_t mode) {
  uint8_t change_mode_msg[] = {0xF0, 0x7D, 0x4D, 0x43, 0x4C, 0x01, mode, 0xF7};
  MidiUartUSB.m_putc(change_mode_msg, sizeof(change_mode_msg));
  delay(200);
  if (mode == USB_SERIAL) {
     MidiUartUSB.mode = UART_SERIAL; MidiUartUSB.set_speed(SERIAL_SPEED);
  }
}
