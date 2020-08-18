#include "MegaComUIServer.h"

MCUIServer megacom_uiserver;

int MCUIServer::run() {
  // TODO external events handling
  while(pending()) msg_getch();
  return -1;
}

int MCUIServer::resume(int state) {
  return -1;
}

void MCUIServer::update() {
  if (!m_update) {
    m_update = true;
  }

  if (megacom_task.tx_checkstatus(COMCHANNEL_UART_USB, COMSERVER_EXTUI) == CS_TX_ACTIVE) {
    // previous tx still on-wire
    return;
  }

  if (!megacom_task.tx_begin(COMCHANNEL_UART_USB, COMSERVER_EXTUI, 513)) {
    // buffer full
    return;
  }
  megacom_task.tx_data(COMCHANNEL_UART_USB, USC_DRAW);
  megacom_task.tx_vec(COMCHANNEL_UART_USB, (char*)oled_display.getBuffer(), 512);
  megacom_task.tx_end(COMCHANNEL_UART_USB);
}
