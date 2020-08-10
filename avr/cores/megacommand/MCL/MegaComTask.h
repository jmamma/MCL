#pragma once

#include "MCL.h"
#include "Task.hh"

#ifdef MEGACOMMAND

// A coroutine that listens to MegaCom frames and dispatches the received frames to the handlers
// The desiderata is to make it non-blocking, and interleaved with other tasks, at the cost of
// having throttled bandwidth and lower priority.
class MegaComTask: public Task {
private:
  CRingBuffer<commsg_t, 0, uint8_t> rx_msgs;
  comchannel_t channels[COMCHANNEL_MAX];
public:
  MegaComTask(uint16_t interval) : Task(interval) {}
  void init();
  ALWAYS_INLINE() void recv_msg_isr(uint8_t channel, uint8_t type, combuf_t* pbuf, uint16_t len);
  ALWAYS_INLINE() void rx_isr(uint8_t channel, uint8_t data);
  ALWAYS_INLINE() uint8_t tx_get_isr(uint8_t channel);
  ALWAYS_INLINE() bool tx_isempty_isr(uint8_t channel);
  // TODO void send_msg(uint8_t channel, uint8_t type, combuf_t* pbuf, uint16_t len);
  virtual void run();
  virtual void destroy() {}
};

extern MegaComTask megacom_task;
#endif // MEGACOMMAND
