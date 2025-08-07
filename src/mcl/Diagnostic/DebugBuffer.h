#pragma once

#include <Arduino.h>

#ifndef __AVR__
 #define DEBUG_BUFFER_SIZE 4096
#else
 #define DEBUG_BUFFER_SIZE 128
#endif

class DebugBuffer : public Stream {
private:
  static constexpr size_t BUFFER_SIZE = DEBUG_BUFFER_SIZE;
  char buffer[BUFFER_SIZE];
  volatile uint32_t head;
  volatile uint32_t tail;
  Stream* output_stream;
  // Private methods
  uint32_t atomic_increment(volatile uint32_t &val);
  bool get(char *str, size_t maxLen);

public:
  DebugBuffer(Stream *stream)
    : head(0), tail(0), output_stream(stream) {}

  // Buffer operations
  bool put(const char *str);
  bool put(const __FlashStringHelper *fstr);
  bool put(const String &str) { return put(str.c_str()); }
  bool put(const StringSumHelper &str) { return put(String(str).c_str()); }
  // Stream interface implementation
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int available() override { return 0; }
  int read() override { return 0; }
  int peek() override { return 0; }
  void transmit();
  void flush() { return; }
};
