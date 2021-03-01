#pragma once

#ifdef DEBUGMODE

extern char _debug_strbuf[];

namespace __debug_impl
{
  inline void format_impl() {
    // template recursion termination
  }

  inline void format(const __FlashStringHelper*& pmsg) {
    strcat_P(_debug_strbuf, (const char*)pmsg);
  }

  inline void format(const char* pmsg) {
    strcat(_debug_strbuf, pmsg);
  }

  inline void format(const int8_t &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%d", val);
    strcat(_debug_strbuf, buf);
  }

  inline void format(const uint8_t &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%u", val);
    strcat(_debug_strbuf, buf);
  }

  inline void format(const int16_t &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%d", val);
    strcat(_debug_strbuf, buf);
  }

  inline void format(const uint16_t &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%u", val);
    strcat(_debug_strbuf, buf);
  }

  inline void format(const int32_t &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%ld", val);
    strcat(_debug_strbuf, buf);
  }

  inline void format(const uint32_t &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%lu", val);
    strcat(_debug_strbuf, buf);
  }

  inline void format(const float &val) {
    char buf[16];
    snprintf(buf, sizeof(buf)-1, "%f", val);
    strcat(_debug_strbuf, buf);
  }

  void send_debug_msg_impl();

  template<typename T1, typename ...Args>
    inline void format_impl(T1 arg1, Args... args) {
      // assume bank0
      // assume correct _debug_strbuf and append content to it
      format(arg1);
      format_impl(args...);
  }
}

template<typename T1, typename ...Args>
inline void debug_msg(T1 arg1, Args... args) {

  USE_LOCK();
  SET_LOCK();

  _debug_strbuf[0] = '\0';
  __debug_impl::format_impl(arg1, args...);
  __debug_impl::send_debug_msg_impl();

  CLEAR_LOCK();
}


#define DEBUG_PRINT(...)  debug_msg(__VA_ARGS__)
#define DEBUG_PRINTLN(...)  debug_msg(__VA_ARGS__)
#define DEBUG_DUMP(x)  debug_msg(#x, " = ", x)
// __PRETTY_FUNCTION__ is a gcc extension
#define DEBUG_PRINT_FN(...) debug_msg("func_call: ", __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define _d(x) #x, " = ", x

#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_DUMP(x)
#define DEBUG_PRINT_FN(...)
#define _d(x) 
#endif
