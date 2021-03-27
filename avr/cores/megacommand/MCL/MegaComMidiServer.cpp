#define IS_ISR_ROUTINE
#include "MegaComMidiServer.h"

// IN ISR
int MCMidiServer::run(){
  uint8_t port = msg_getch();
  MidiUartParent* device;
  if (port == 0) {
    if (!Midi.ext_in) return -1;
    device = &MidiUart;
  } else {
    if (!Midi2.ext_in) return -1;
    device = &MidiUart2;
  }

  while(msg.len) {
    uint8_t data = msg_getch();
    device->m_putc(data);
  }
  return -1;
}

void MCMidiServer::send_isr(uint8_t port, uint8_t data) {
  // TODO hard coded
  if (!megacom_task.tx_begin_isr(COMCHANNEL_UART_USB, COMSERVER_EXTMIDI, 2)) {
    setLed2();
    return;
  }
  megacom_task.tx_data(COMCHANNEL_UART_USB, port);
  megacom_task.tx_data(COMCHANNEL_UART_USB, data);
  megacom_task.tx_end_isr(COMCHANNEL_UART_USB);
}

void MCMidiServer::send(uint8_t port, uint8_t data) {
  USE_LOCK();
  SET_LOCK();
  send_isr(port, data);
  CLEAR_LOCK();
}

MCMidiServer megacom_midiserver;
