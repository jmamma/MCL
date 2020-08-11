#define IS_ISR_ROUTINE

#include "MegaComTask.h"
#include "MegaComFileServer.h"

#ifdef MEGACOMMAND

ISR(USART0_RX_vect) {
  select_bank(0);
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

void uart0_tx_available_cb() { UART_USB_SET_ISR_TX_BIT(); }

void comchannel_t::init(int id, uint8_t *p_rxbuf, uint16_t sz_rxbuf,
                        uint8_t *p_txbuf, uint16_t sz_txbuf) {
  this->id = id;
  this->rx_state = 0;
  this->rx_buf.ptr = p_rxbuf;
  this->rx_buf.len = sz_rxbuf;
  this->tx_buf.ptr = p_txbuf;
  this->tx_buf.len = sz_txbuf;
  this->tx_active = false;
}

void comchannel_t::tx_set_data_available_callback(void (*cb)()) {
  this->tx_available_callback = cb;
}

uint8_t comchannel_t::tx_get_isr() { return tx_buf.get_h_isr(); }

bool comchannel_t::tx_isempty_isr() { return tx_buf.isEmpty_isr(); }

void comchannel_t::rx_isr(uint8_t data) {
  switch (rx_state) {
  case COMSTATE_SYNC:
    if (data == COMSYNC_TOKEN) {
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
    if (!rx_len) {
      rx_state = COMSTATE_CHECKSUM;
    } else {
      rx_state = COMSTATE_DATA;
    }
    rx_chksum += data;
    break;
  case COMSTATE_DATA:
    rx_chksum += data;
    // TODO check for overflow?
    rx_buf.put_h_isr(data);
    if (--rx_pending == 0) {
      rx_state = COMSTATE_CHECKSUM;
    }
    break;
  case COMSTATE_CHECKSUM:
    rx_state = COMSTATE_SYNC;
    if (rx_chksum == data) {
      if (rx_type == COMSERVER_REQUEST_RESEND) {
        // should be 0
        rx_buf.skip(rx_len);
        tx_status = CS_RESEND;
      } else if (rx_type == COMSERVER_ACK) { 
        // should be 0
        rx_buf.skip(rx_len);
        tx_status = CS_ACK;
      } else if (rx_type == COMSERVER_UNSUPPORTED) {
        // should be 0
        rx_buf.skip(rx_len);
        tx_status = CS_UNSUPPORTED;
      } else {
        auto status = megacom_task.recv_msg_isr(id, rx_type, &rx_buf, rx_len);
        if (status == CS_ACK) {
          // ack it
          tx_begin(true, COMSERVER_ACK, 0);
          tx_end_isr();
        } else if (status == CS_UNSUPPORTED) {
          // reject it
          tx_begin(true, COMSERVER_UNSUPPORTED, 1);
          tx_data_isr(rx_type);
          tx_end_isr();
        } else {
          goto request_resend;
          // request a resend, buy us some time to process the messages...
        }
      }
    }
    else {
request_resend:
      // wrong data in buffer, drain and request a resend
      rx_buf.skip(rx_len);
      // must be true here because we just unlocked rx state
      tx_begin(true, COMSERVER_REQUEST_RESEND, 0);
      tx_end_isr();
    }
    break;
  }
}

bool comchannel_t::tx_begin(bool isr, uint8_t type, uint16_t len) {
  // begin tx if and only if there isn't another tx or rx message on wire
  if (tx_active || rx_state != COMSTATE_SYNC)
    return false;

  USE_LOCK();
  if (!isr) {
    SET_LOCK();
  }

  tx_chksum = 0;

  // SYNC
  tx_buf.put_h_isr(COMSYNC_TOKEN);
  // TYPE
  tx_chksum += type;
  tx_buf.put_h_isr(type);
  // LEN1
  uint8_t l = (len & 0xFF00) >> 8;
  tx_chksum += l;
  tx_buf.put_h_isr(l);
  // LEN2
  l = len & 0x00FF;
  tx_chksum += l;
  tx_buf.put_h_isr(l);

  if (tx_available_callback)
    tx_available_callback();

  // the other end will not start a tx until we finish sending this one

  tx_active = true;
  tx_status = CS_TIMEOUT;

  if (!isr) {
    CLEAR_LOCK();
  }

  return true;
}

void comchannel_t::tx_data(uint8_t data) {
  tx_chksum += data;
  tx_buf.put_h(data);
  if (tx_available_callback)
    tx_available_callback();
}

void comchannel_t::tx_data_isr(uint8_t data) {
  tx_chksum += data;
  tx_buf.put_h_isr(data);
  if (tx_available_callback)
    tx_available_callback();
}

// tx_end check for ACK
comstatus_t comchannel_t::tx_end() {
  // free tx state first so that once we send out the checksum, the rx isr
  // can listen to ACK/RESEND
  tx_active = false;
  tx_buf.put_h(tx_chksum);
  if (tx_available_callback)
    tx_available_callback();

  constexpr uint16_t timeout = 200; //ms

  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;
  do {
    if (tx_status != CS_TIMEOUT) break;
    current_clock = read_slowclock();
    handleIncomingMidi();
  } while (clock_diff(start_clock, current_clock) < timeout);

  return tx_status;
}

// tx_end_isr doesn't check for ACK
void comchannel_t::tx_end_isr() {
  tx_buf.put_h_isr(tx_chksum);
  if (tx_available_callback)
    tx_available_callback();
  tx_active = false;
}

void MegaComTask::init() {
  rx_msgs.len = NUM_COMMSG_SLOTS;
  rx_msgs.ptr = COMMSG_SLOTS_START;

  for (int i = 0; i < COMCHANNEL_MAX; ++i) {
    channels[i].init(
        i, (uint8_t *)(COMCHANNEL_BUFFER_START + i * 2 * COMCHANNEL_BUFSIZE),
        COMCHANNEL_BUFSIZE,
        (uint8_t *)(COMCHANNEL_BUFFER_START + (i * 2 + 1) * COMCHANNEL_BUFSIZE),
        COMCHANNEL_BUFSIZE);
  }

  for (int i = 0; i < COMSERVER_MAX; ++i) {
    servers[i] = nullptr;
  }

  suspended_server = nullptr;
  suspended_state = 0;

  // COMCHANNEL_UART_USB init
  {
    USE_LOCK();
    SET_LOCK();
    uart_set_speed(SERIAL_SPEED, 0);
    UCSR0C = (3 << UCSZ00);
    /** enable receive, transmit and receive and transmit interrupts. **/
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
    CLEAR_LOCK();
  }

  channels[COMCHANNEL_UART_USB].tx_set_data_available_callback(
      uart0_tx_available_cb);

  // COMSERVER_FILESERVER init
  servers[COMSERVER_FILESERVER] = &megacom_fileserver;
}

void MegaComTask::update_server_state(MegaComServer *pserver, int state) {
  if (state == -1) {
    suspended_server = nullptr;
  } else {
    suspended_server = pserver;
    suspended_state = state;
  }
}

void MegaComTask::run() {
  if (suspended_server != nullptr) {
    int state = suspended_server->resume(suspended_state);
    update_server_state(suspended_server, state);
  } else if (rx_msgs.size()) {
    // handle rx messages here
    auto cur_msg = rx_msgs.get_h();
    auto pserver = servers[cur_msg.type];
    pserver->msg = cur_msg;
    int state = pserver->run();
    update_server_state(pserver, state);
  }
  // TODO uart_usb is async I/O so we don't worry about tx here. but there could
  // be sync I/O that needs some manual driving...
  // TODO we can also implement comm. indicators in the GUI. Arrows for
  // uplink/downlink
}

ALWAYS_INLINE()
comstatus_t MegaComTask::recv_msg_isr(uint8_t channel, uint8_t type, combuf_t *pbuf,
                               uint16_t len) {
  if (rx_msgs.isFull_isr()) {
    pbuf->skip(len);
    return CS_RESEND;
  }
  if (type >= COMSERVER_MAX || servers[type] == nullptr){
    // unsupported message
    pbuf->skip(len);
    return CS_UNSUPPORTED;
  }
  commsg_t msg{pbuf, len, channel, type};
  rx_msgs.put_h_isr(msg);
  return CS_ACK;
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

bool MegaComTask::tx_begin(uint8_t channel, uint8_t type, uint16_t len) {
  return channels[channel].tx_begin(false, type, len);
}

void MegaComTask::tx_data(uint8_t channel, uint8_t data) {
  channels[channel].tx_data(data);
}

void MegaComTask::tx_word(uint8_t channel, uint16_t data) {
  channels[channel].tx_data(data >> 8);
  channels[channel].tx_data(data & 0xFF);
}

void MegaComTask::tx_dword(uint8_t channel, uint32_t data) {
  channels[channel].tx_data(data >> 24);
  channels[channel].tx_data((data >> 16) & 0xFF);
  channels[channel].tx_data((data >> 8) & 0xFF);
  channels[channel].tx_data(data & 0xFF);
}

void MegaComTask::tx_vec(uint8_t channel, char* vec, int len) {
  USE_LOCK();
  SET_LOCK();
  for(int i=0;i<len;++i) {
    channels[channel].tx_data_isr(vec[i]);
  }
  CLEAR_LOCK();
}

comstatus_t MegaComTask::tx_end(uint8_t channel) { return channels[channel].tx_end(); }

uint8_t MegaComServer::msg_getch() {
  --msg.len;
  return msg.pbuf->get_h();
}

uint16_t MegaComServer::pending() {
  return msg.len;
}

MegaComTask megacom_task(0);

#endif // MEGACOMMAND
