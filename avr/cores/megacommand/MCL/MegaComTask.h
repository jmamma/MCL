#pragma once

#include "MCL.h"
#include "Task.hh"

#ifdef MEGACOMMAND

class MegaComServer {
public:
  // set by the task, a local copy
  commsg_t msg;
  uint8_t get();
  uint16_t pending();
  virtual int run() = 0;
  virtual int resume(int state) = 0;
};

// A coroutine that listens to MegaCom frames and dispatches the received frames to the handlers
// The desiderata is to make it non-blocking, and interleaved with other tasks, at the cost of
// having throttled bandwidth and lower priority.
class MegaComTask: public Task {
private:
  CRingBuffer<commsg_t, 0, uint8_t> rx_msgs;
  comchannel_t channels[COMCHANNEL_MAX];
  MegaComServer* servers[COMSERVER_MAX];
  MegaComServer* suspended_server;
  int suspended_state;
public:
  MegaComTask(uint16_t interval) : Task(interval) {}
  void init();
  void update_server_state(MegaComServer* pserver, int state);
  ALWAYS_INLINE() comstatus_t recv_msg_isr(uint8_t channel, uint8_t type, combuf_t* pbuf, uint16_t len);
  ALWAYS_INLINE() void rx_isr(uint8_t channel, uint8_t data);
  ALWAYS_INLINE() uint8_t tx_get_isr(uint8_t channel);
  ALWAYS_INLINE() bool tx_isempty_isr(uint8_t channel);
  bool tx_begin(uint8_t channel, uint8_t type, uint16_t len);
  void tx_data(uint8_t channel, uint8_t data);
  void tx_word(uint8_t channel, uint16_t data);
  void tx_dword(uint8_t channel, uint32_t data);
  void tx_vec(uint8_t channel, char* vec, int len);
  comstatus_t tx_end(uint8_t channel);
  virtual void run();
  virtual void destroy() {}
};

extern MegaComTask megacom_task;
#endif // MEGACOMMAND
