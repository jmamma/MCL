#pragma once

#include <Arduino.h>

class DebugBuffer : public Stream {
private:
  static constexpr size_t BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  volatile uint32_t head __attribute__((aligned(4)));
  volatile uint32_t tail __attribute__((aligned(4)));
  char temp_buf[64];
  Stream* output_stream;
  
  // Private methods
  uint32_t atomic_increment(volatile uint32_t &val);
  bool get(char *str, size_t maxLen);

public:
  // Constructors
  explicit DebugBuffer(Stream* stream);
  DebugBuffer();
  
  // Buffer operations
  bool put(const char *str);
  bool put(const __FlashStringHelper *fstr);
  bool put(const String &str) { return put(str.c_str()); }
  bool put(const StringSumHelper &str) { return put(String(str).c_str()); }
  void flush();
  
  // Stream interface implementation
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int available() override { return 0; }
  int read() override { return 0; }
  int peek() override { return 0; }
};
