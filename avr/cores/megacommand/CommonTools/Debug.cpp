#include "MegaComTask.h"
#include "Debug.h"
#include <string.h>
#include <stdio.h>

#ifdef MEGACOMMAND

char _debug_strbuf[256];

namespace __debug_impl {

void format_impl() {
  // template recursion termination
}

void format(const char* pmsg) {
  strcat(_debug_strbuf, pmsg);
}

void format(const int8_t &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%d", val);
  strcat(_debug_strbuf, buf);
}

void format(const uint8_t &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%u", val);
  strcat(_debug_strbuf, buf);
}

void format(const int16_t &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%d", val);
  strcat(_debug_strbuf, buf);
}

void format(const uint16_t &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%u", val);
  strcat(_debug_strbuf, buf);
}

void format(const int32_t &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%ld", val);
  strcat(_debug_strbuf, buf);
}

void format(const uint32_t &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%lu", val);
  strcat(_debug_strbuf, buf);
}

void format(const float &val) {
  char buf[16];
  snprintf(buf, sizeof(buf)-1, "%f", val);
  strcat(_debug_strbuf, buf);
}

void send_debug_msg_impl() {
  // assume locked, bank0
  megacom_task.debug(_debug_strbuf);
}

} //namespace __debug_impl

#endif
