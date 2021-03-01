#include "MegaComTask.h"
#include "Debug.h"
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#ifdef MEGACOMMAND

char _debug_strbuf[256];

namespace __debug_impl {
  void send_debug_msg_impl() {
    // assume locked, bank0
    megacom_task.debug(_debug_strbuf);
  }
} //namespace __debug_impl

#endif
