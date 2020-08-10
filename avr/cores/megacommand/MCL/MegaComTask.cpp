#define IS_ISR_ROUTINE

#include "MegaComTask.h"

#ifdef MEGACOMMAND

ISR(USART0_RX_vect) {
  uint8_t data = UART_USB_READ_CHAR();
  megacom_task.rx_isr(COMCHANNEL_UART_USB, data);
}

ISR(USART0_UDRE_vect) {
  select_bank(0);
  if (!megacom_task.tx_isempty_isr(COMCHANNEL_UART_USB)) {
    UART_USB_WRITE_CHAR(megacom_task.tx_get_isr(COMCHANNEL_UART_USB));
  }
  if (megacom_task.tx_isempty_isr(COMCHANNEL_UART_USB)) {
    UART_USB_CLEAR_ISR_TX_BIT();
  }
}

void uart0_tx_available_cb() {
  UART_USB_SET_ISR_TX_BIT();
}

void comchannel_t::init(int id, uint8_t* p_rxbuf, uint16_t sz_rxbuf, uint8_t* p_txbuf, uint16_t sz_txbuf) {
  this->id = id;
  this->rx_state = 0;
  this->rx_buf.ptr = p_rxbuf;
  this->rx_buf.len = sz_rxbuf;
  this->tx_buf.ptr = p_txbuf;
  this->tx_buf.len = sz_txbuf;
  this->tx_active = false;
}

void comchannel_t::tx_set_data_available_callback(void(*cb)()) {
  this->tx_available_callback = cb;
}

uint8_t comchannel_t::tx_get_isr() {
  return tx_buf.get_h_isr();
}

bool comchannel_t::tx_isempty_isr() {
  return tx_buf.isEmpty_isr();
}

void comchannel_t::rx_isr(uint8_t data) {
  switch(rx_state) {
    case COMSTATE_SYNC:
      if (data == 0x00) {
        rx_state = COMSTATE_TYPE;
        rx_chksum = 0;
      }
      break;
    case COMSTATE_TYPE:
      rx_type = data;
      rx_state = COMSTATE_LEN1;
      rx_chksum += data;
      break;
    case COMSTATE_LEN1:
      rx_len = data;
      rx_state = COMSTATE_LEN2;
      rx_chksum += data;
      break;
    case COMSTATE_LEN2:
      rx_len = (rx_len << 8) + data;
      rx_pending = rx_len;
      if(!rx_len) { rx_state = COMSTATE_CHECKSUM; } 
      else { rx_state = COMSTATE_DATA; }
      rx_chksum += data;
      break;
    case COMSTATE_DATA:
      rx_chksum += data;
      rx_buf.put_h_isr(data);
      if(--rx_pending == 0) { rx_state = COMSTATE_CHECKSUM; }
      break;
    case COMSTATE_CHECKSUM:
      rx_state = COMSTATE_SYNC;
      if(rx_chksum == data) { megacom_task.recv_msg_isr(id, rx_type, &rx_buf, rx_len); }
      // wrong data in buffer, have to drain it
      else { megacom_task.recv_msg_isr(id, COMSERVER_DRAIN_INVALID, &rx_buf, rx_len); }
      break;
  }
}

bool comchannel_t::tx_begin(uint8_t type, uint16_t len) {
  if (tx_active) return false;
  tx_active = true;
  tx_chksum = 0;

  // SYNC
  tx_buf.put_h(0x00);
  // TYPE
  tx_chksum += type;
  tx_buf.put_h(type);
  // LEN1
  uint8_t l = (len & 0xFF00) >> 8;
  tx_chksum += l;
  tx_buf.put_h(l);
  // LEN2
  l = len & 0x00FF;
  tx_chksum += l;
  tx_buf.put_h(l);

  if (tx_available_callback) tx_available_callback();

  return true;
}

void comchannel_t::tx_data(uint8_t data) {
  tx_chksum += data;
  tx_buf.put_h(data);
  if (tx_available_callback) tx_available_callback();
}

void comchannel_t::tx_end() {
  tx_buf.put_h(tx_chksum);
  if (tx_available_callback) tx_available_callback();
  tx_active = false;
}

void MegaComTask::init() {
  rx_msgs.len = NUM_COMMSG_SLOTS;
  rx_msgs.ptr = COMMSG_SLOTS_START;

  for(int i=0; i<COMCHANNEL_MAX; ++i) {
    channels[i].init(
        i, 
        (uint8_t*)(COMCHANNEL_BUFFER_START + i * 2 * COMCHANNEL_BUFSIZE), COMCHANNEL_BUFSIZE,
        (uint8_t*)(COMCHANNEL_BUFFER_START + (i * 2 + 1) * COMCHANNEL_BUFSIZE), COMCHANNEL_BUFSIZE);
  }

  // COMCHANNEL_UART_USB init

  uart_set_speed(SERIAL_SPEED, 0);
  UCSR0C = (3 << UCSZ00);
  /** enable receive, transmit and receive and transmit interrupts. **/
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

  channels[COMCHANNEL_UART_USB].tx_set_data_available_callback(uart0_tx_available_cb);
}

void MegaComTask::run() {
  // TODO resume waiting handler
  if (rx_msgs.size()) {
    // handle rx messages here
    commsg_t msg = rx_msgs.get_h();
  }
  // TODO uart_usb is async I/O so we don't worry about tx here. but there could be sync I/O that needs some manual driving...
  // TODO we can also implement comm. indicators in the GUI. Arrows for uplink/downlink
}

ALWAYS_INLINE() void MegaComTask::recv_msg_isr(uint8_t channel, uint8_t type, combuf_t* pbuf, uint16_t len) {
  commsg_t msg { pbuf, len, channel, type };
  rx_msgs.put_h_isr(msg);
}

ALWAYS_INLINE() void MegaComTask::rx_isr(uint8_t channel, uint8_t data) {
  channels[channel].rx_isr(data);
}

ALWAYS_INLINE() uint8_t MegaComTask::tx_get_isr(uint8_t channel) {
  return channels[channel].tx_get_isr();
}

ALWAYS_INLINE() bool MegaComTask::tx_isempty_isr(uint8_t channel) {
  return channels[channel].tx_isempty_isr();
}

MegaComTask megacom_task(0);

#endif //MEGACOMMAND
