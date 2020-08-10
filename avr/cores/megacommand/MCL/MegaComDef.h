#pragma once

typedef RingBuffer<0, uint16_t> combuf_t;

#include "RingBuffer.h"

#define COMCHANNEL_BUFSIZE 1024

#define COMSTATE_SYNC     0
#define COMSTATE_TYPE     1
#define COMSTATE_LEN1     2
#define COMSTATE_LEN2     3
#define COMSTATE_DATA     4
#define COMSTATE_CHECKSUM 5

enum comchannel_id_t {
  COMCHANNEL_UART_USB,
  // COMCHANNEL_EXPANSION_L,
  // COMCHANNEL_EXPANSION_R,
  COMCHANNEL_MAX
};

enum comserver_id_t {
  COMSERVER_EXTUI, // button input, ext screen etc.
  COMSERVER_FILESERVER, // simple FTP-ish protocol
  COMSERVER_EXTMIDI, // note in, CV out etc.
  COMSERVER_MAX,
  COMSERVER_DRAIN_INVALID = 0xFF
};

class comchannel_t {
private:
  uint8_t id;

  combuf_t rx_buf;
  uint8_t rx_state;
  uint8_t rx_type;
  uint16_t rx_len;
  uint16_t rx_pending;
  uint8_t rx_chksum;

  combuf_t tx_buf;
  uint8_t tx_chksum;
  bool tx_active;
  void (*tx_available_callback)();

public:
  void init(int id, uint8_t* p_rxbuf, uint16_t sz_rxbuf, uint8_t* p_txbuf, uint16_t sz_txbuf);
  void rx_isr(uint8_t data);
  uint8_t tx_get_isr();
  bool tx_isempty_isr();
  bool tx_begin(uint8_t type, uint16_t len);
  void tx_data(uint8_t data);
  void tx_end();
  void tx_set_data_available_callback(void(*cb)());
};

class commsg_t {
public:
  commsg_t() { }
  commsg_t(combuf_t* _pbuf, uint16_t _len, uint8_t _channel, uint8_t _type): pbuf(_pbuf), len(_len), channel(_channel), type(_type) {}
  commsg_t(const volatile commsg_t& that): commsg_t(that.pbuf, that.len, that.channel, that.type) {}
  commsg_t(const commsg_t& that): commsg_t(that.pbuf, that.len, that.channel, that.type) {}
  // commsg_t& operator=(commsg_t msg){
  //   auto ret = commsg_t(msg.pbuf, msg.len, msg.channel, msg.type);
  //   return ret;
  // }
  combuf_t* pbuf;
  uint16_t len;
  uint8_t channel;
  uint8_t type;
};

