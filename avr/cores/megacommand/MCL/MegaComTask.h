#pragma once

#include "MCL.h"
#include "Task.hh"

#ifdef MEGACOMMAND

class MegaComServer {
public:
  MegaComServer(bool rtserver): is_realtime(rtserver) {}
  // set by the task, a local copy
  commsg_t msg;
  // initialized by the server
  bool is_realtime;
  uint8_t msg_getch();
  uint16_t pending();
  virtual int run() = 0;
  virtual int resume(int state) = 0;
};

// A coroutine that listens to MegaCom frames and dispatches the received frames to the handlers
// The desiderata is to make it non-blocking, and interleaved with other tasks, at the cost of
// having throttled bandwidth and lower priority.
// There are two types of messages. REALTIME and ASYNC.
// ASYNC RX: Upon frame receive, put it into the message queue, send back ACK, and process it by the task
// ASYNC TX: Initiated in non-ISR context, queued into send buffer.
// REALTIME RX: Upon frame receive, handled in the ISR directly, then send back ACK.
// REALTIME TX: Initiated in ISR context.
// Several implications:
//   - It means REALTIME messages are full-duplex but ASYNC messages are half-duplex (ping-pong).
//   - REALTIME servers are STATELESS
class MegaComTask: public Task {
private:
  CRingBuffer<commsg_t, 0, uint8_t> rx_msgs;
  comchannel_t channels[COMCHANNEL_MAX];
  MegaComServer* servers[COMSERVER_MAX];
  MegaComServer* suspended_server;
  int suspended_state;

private:
public:
  MegaComTask(uint16_t interval) : Task(interval) {}
  void init();
  void update_server_state(MegaComServer* pserver, int state);
  ALWAYS_INLINE() comstatus_t recv_msg_isr(uint8_t channel, uint8_t type, combuf_t* pbuf, uint16_t len);
  ALWAYS_INLINE() void rx_isr(uint8_t channel, uint8_t data);
  ALWAYS_INLINE() uint8_t tx_get_isr(uint8_t channel);
  ALWAYS_INLINE() bool tx_isempty_isr(uint8_t channel);
  void tx_begin(uint8_t channel, uint8_t type, uint16_t len);
  void tx_data(uint8_t channel, uint8_t data);
  void tx_word(uint8_t channel, uint16_t data);
  void tx_dword(uint8_t channel, uint32_t data);
  void tx_vec(uint8_t channel, char* vec, int len);
  void tx_begin_isr(uint8_t channel, uint8_t type, uint16_t len);
  comstatus_t tx_end(uint8_t channel);
  void tx_end_isr(uint8_t channel);

  void debug(char* pmsg);

  virtual void run();
  virtual void destroy() {}
};

extern MegaComTask megacom_task;
#endif // MEGACOMMAND
