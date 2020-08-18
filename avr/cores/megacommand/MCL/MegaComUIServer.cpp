#include "MegaComUIServer.h"

MCUIServer megacom_uiserver;

int MCUIServer::run() {
  ui_server_command_t cmd = (ui_server_command_t)msg_getch();
  if (cmd == USC_INVALIDATE_VISUALS) {
    m_update = false;
    if (megacom_task.tx_checkstatus(COMCHANNEL_UART_USB, COMSERVER_EXTUI) == CS_TX_ACTIVE) {
      // previous tx still on-wire
      return -1;
    }

    if (!megacom_task.tx_begin(COMCHANNEL_UART_USB, COMSERVER_EXTUI, 513)) {
      // buffer full
      return -1;
    }
    megacom_task.tx_data(COMCHANNEL_UART_USB, USC_DRAW);
    megacom_task.tx_vec(COMCHANNEL_UART_USB, (char*)oled_display.getBuffer(), 512);
    megacom_task.tx_end(COMCHANNEL_UART_USB);
  } else {
    // TODO external events handling
    while(pending()) msg_getch();
  }

  return -1;
}

int MCUIServer::resume(int state) {
  return -1;
}

void MCUIServer::update() {
  if (!m_update) {
    m_update = true;
    megacom_task.tx_begin(COMCHANNEL_LOOPBACK, COMSERVER_EXTUI, 1);
    megacom_task.tx_data(COMCHANNEL_LOOPBACK, USC_INVALIDATE_VISUALS);
    megacom_task.tx_end(COMCHANNEL_LOOPBACK);
  }
}
