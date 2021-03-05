#define IS_ISR_ROUTINE

#include "MegaComTask.h"
#include "MegaComFileServer.h"
#include "MegaComMidiServer.h"
#include "MegaComUIServer.h"

#ifdef MEGACOMMAND

ISR(USART0_RX_vect) {
  select_bank(0);
  if (UART_USB_CHECK_OVERRUN()) {
    setLed2();
  }
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

comchannel_t* ploopback;
void loopback_tx_available_cb() {
  while (!ploopback->tx_isempty_isr()) {
    ploopback->rx_isr(ploopback->tx_get_isr());
  }
}

void comchannel_t::init(int id, uint8_t *p_rxbuf, uint16_t sz_rxbuf,
                        uint8_t *p_txbuf, uint16_t sz_txbuf) {
  this->id = id;
  this->rx_state = 0;
  this->rx_buf.ptr = p_rxbuf;
  this->rx_buf.len = sz_rxbuf;
  this->tx_buf.ptr = p_txbuf;
  this->tx_buf.len = sz_txbuf;
  this->tx_available_callback = nullptr;
  memset(this->tx_status, CS_ACK, COMSERVER_MAX);
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
    if (!rx_len || rx_type >= COMSERVER_UNSUPPORTED) {
      rx_state = COMSTATE_CHECKSUM;
    } else if (rx_len >= COMCHANNEL_BUFSIZE) {
      rx_state = COMSTATE_SYNC;
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
    // for these data types, do not send back REQUEST_RESEND, just update tx
    // status.
    if (rx_type == COMSERVER_REQUEST_RESEND) {
      if (rx_chksum == data)
        tx_status[rx_len] = CS_RESEND;
    } else if (rx_type == COMSERVER_ACK) {
      if (rx_chksum == data)
        tx_status[rx_len] = CS_ACK;
    } else if (rx_type == COMSERVER_UNSUPPORTED) {
      if (rx_chksum == data)
        tx_status[rx_len] = CS_UNSUPPORTED;
    } else if (rx_chksum == data) { // incoming message, check chksum and
                                    // request resend if not match
      auto status = megacom_task.recv_msg_isr(id, rx_type, &rx_buf, rx_len);
      if (status == CS_ACK) {
        // ack it
        tx_begin(true, COMSERVER_ACK, rx_type);
        tx_end_isr();
      } else if (status == CS_UNSUPPORTED) {
        // reject it
        rx_buf.undo(rx_len);
        tx_begin(true, COMSERVER_UNSUPPORTED, rx_type);
        tx_end_isr();
      } else if (status == CS_REALTIME_MESSAGE) {
        // undo the buffer
        rx_buf.undo(rx_len);
        // ack it
        tx_begin(true, COMSERVER_ACK, rx_type);
        tx_end_isr();
      } else {
        goto request_resend;
        // request a resend, buy us some time to process the messages...
      }
    } else {
    request_resend:
      // wrong data in buffer, drain and request a resend
      rx_buf.undo(rx_len);
      tx_begin(true, COMSERVER_REQUEST_RESEND, rx_type);
      tx_end_isr();
    }
    break;
  }
}

comstatus_t comchannel_t::tx_checkstatus(uint8_t type) {
  if (type < COMSERVER_MAX) {
    comstatus_t status = tx_status[type];
    constexpr uint16_t timeout = 200; // ms
    if (status == CS_TX_ACTIVE) {
      uint16_t cur_time = read_slowclock();
      if (clock_diff(tx_timestamp[type], cur_time) >= timeout) {
        tx_status[type] = status = CS_TIMEOUT;
      }
    }
    return status;
  } else {
    return CS_REALTIME_MESSAGE;
  }
}

bool comchannel_t::tx_begin(bool isr, uint8_t type, uint16_t len) {
  if (tx_buf.len - tx_buf.size() < len) {
    return false;
  }

  if (!isr) {
    // wait until the previous tx is acked
    constexpr uint16_t timeout = 200; // ms
    uint16_t new_time = read_slowclock();
    uint16_t old_time = tx_timestamp[type];
    do {
      if (tx_status[type] != CS_TX_ACTIVE)
        break;
      new_time = read_slowclock();
    } while (clock_diff(old_time, new_time) < timeout);
    if (tx_status[type] == CS_TX_ACTIVE) {
      tx_status[type] = CS_TIMEOUT;
    }

    tx_irqlock = SREG;
    cli();
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

  tx_available_callback();

  if (!isr && type < COMSERVER_MAX) {
    tx_status[type] = CS_TX_ACTIVE;
    tx_timestamp[type] = read_slowclock();
  }
}

void comchannel_t::tx_data(uint8_t data) {
  tx_chksum += data;
  tx_buf.put_h_isr(data);
  tx_available_callback();
}

void comchannel_t::tx_end() {
  tx_buf.put_h_isr(tx_chksum);
  tx_available_callback();

  // release IRQ lock
  SREG = tx_irqlock;
}

// tx_end_isr doesn't check for ACK
void comchannel_t::tx_end_isr() {
  tx_buf.put_h_isr(tx_chksum);
  tx_available_callback();
}

void MegaComTask::init() {
  USE_LOCK();
  SET_LOCK();

  rx_msgs.len = NUM_COMMSG_SLOTS;
  rx_msgs.ptr = (commsg_t*)COMMSG_SLOTS_START;

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

  // COMSERVER_FILESERVER init
  servers[COMSERVER_FILESERVER] = &megacom_fileserver;

  // COMSERVER_EXTMIDI init
  servers[COMSERVER_EXTMIDI] = &megacom_midiserver;

  // COMSERVER_EXTUI init
  servers[COMSERVER_EXTUI] = &megacom_uiserver;

  // COMCHANNEL_UART_USB init
  channels[COMCHANNEL_UART_USB].tx_set_data_available_callback(
      uart0_tx_available_cb);

  uart_set_speed(SERIAL_SPEED, 0);
  UCSR0C = (3 << UCSZ00);
  /** enable receive, transmit and receive and transmit interrupts. **/
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

  // COMCHANNEL_LOOPBACK init
  ploopback = &channels[COMCHANNEL_LOOPBACK];
  channels[COMCHANNEL_LOOPBACK].tx_set_data_available_callback(
      loopback_tx_available_cb);

  CLEAR_LOCK();
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

  // 5ms time budget, but execute at least once
  auto start_time = read_slowclock();
  auto cur_time = start_time;

  while (clock_diff(start_time, cur_time) < 5) {

    // handle messages
    if (suspended_server != nullptr) {
      int state = suspended_server->resume(suspended_state);
      update_server_state(suspended_server, state);
    } else if (rx_msgs.size()) {
      // handle rx messages here
      auto cur_msg = rx_msgs.get();
      auto pserver = servers[cur_msg.type];
      pserver->msg = cur_msg;
      int state = pserver->run();
      update_server_state(pserver, state);
    } else {
      // time's wasting
      break;
    }
    cur_time = read_slowclock();
  }

  // TODO we can also implement comm. indicators in the GUI. Arrows for
  // uplink/downlink
}

ALWAYS_INLINE()
comstatus_t MegaComTask::recv_msg_isr(uint8_t channel, uint8_t type,
                                      combuf_t *pbuf, uint16_t len) {
  auto *server = servers[type];
  commsg_t msg{pbuf, len, channel, type};

  if (type >= COMSERVER_MAX || server == nullptr) {
    return CS_UNSUPPORTED;
  }

  if (server->is_realtime) {
    // clone the rb, skip the part ahead of the current message
    auto buf_clone = *pbuf;
    msg.pbuf = &buf_clone;
    buf_clone.skip(buf_clone.size() - len);
    server->msg = msg;
    server->run();
    return CS_REALTIME_MESSAGE;
  }

  if (rx_msgs.isFull_isr()) {
    return CS_BUFFER_FULL;
  }

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

bool MegaComTask::tx_begin_isr(uint8_t channel, uint8_t type, uint16_t len) {
  return channels[channel].tx_begin(true, type, len);
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

void MegaComTask::tx_vec(uint8_t channel, const char *vec, int len) {
  if (len) {
    channels[channel].tx_data(vec[0]);
  }
  for (int i = 1; i < len; ++i) {
    channels[channel].tx_data(vec[i]);
    // maintain steady clock cooperatively
#ifdef MEGACOMMAND
    if ((i & 0x3F) == 0) {
      if (TIMER1_CHECK_INT()) {
        TCNT1 = 0;
        clock++;
        TIMER1_CLEAR_INT()
      }
      if (TIMER3_CHECK_INT()) {
        TCNT2 = 0;
        slowclock++;
        TIMER3_CLEAR_INT()
      }
    }
#endif
  }
}

void MegaComTask::tx_end(uint8_t channel) { channels[channel].tx_end(); }

comstatus_t MegaComTask::tx_checkstatus(uint8_t channel, uint8_t type) {
  return channels[channel].tx_checkstatus(type);
}

void MegaComTask::tx_end_isr(uint8_t channel) {
  channels[channel].tx_end_isr();
}

void MegaComTask::debug(const char *pmsg) {
  USE_LOCK();
  SET_LOCK();

  int len = strlen(pmsg);

  tx_begin_isr(COMCHANNEL_UART_USB, COMSERVER_DEBUG, len + 1);
  tx_vec(COMCHANNEL_UART_USB, pmsg, len + 1);
  tx_end_isr(COMCHANNEL_UART_USB);
  CLEAR_LOCK();
}

uint8_t MegaComServer::msg_getch() {
  --msg.len;
  return msg.pbuf->get();
}

uint16_t MegaComServer::pending() { return msg.len; }

MegaComTask megacom_task(0);

#endif // MEGACOMMAND
