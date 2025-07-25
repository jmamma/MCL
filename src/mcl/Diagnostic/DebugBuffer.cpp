#include "DebugBuffer.h"
#include "platform.h"

// Constructors
DebugBuffer::DebugBuffer(Stream *stream)
    : output_stream(stream), head(0), tail(0) {}

DebugBuffer::DebugBuffer() : output_stream(nullptr), head(0), tail(0) {}

// Private methods
uint32_t DebugBuffer::atomic_increment(volatile uint32_t &val) {
  uint32_t result;
  USE_LOCK();
  SET_LOCK();
  result = val;
  val = (val + 1) % BUFFER_SIZE;
  CLEAR_LOCK();
  return result;
}

bool DebugBuffer::get(char *str, size_t maxLen) {
  if (head == tail)
    return false;

  USE_LOCK();
  SET_LOCK();
  size_t i = 0;

  // Look for a complete line or fill buffer
  while (i < maxLen - 1 && tail != head) {
    str[i] = buffer[tail];
    bool isNewline = (str[i] == '\n');
    tail = (tail + 1) % BUFFER_SIZE;
    i++;

    // Stop at newline to keep messages together
    if (isNewline) {
      break;
    }
  }

  str[i] = '\0';
  CLEAR_LOCK();
  return (i > 0);
}

// Public buffer operations
bool DebugBuffer::put(const char *str) {
  size_t len = strlen(str);
  if (len >= BUFFER_SIZE)
    return false;

  // Check available space atomically
  USE_LOCK();
  SET_LOCK();
  size_t available = (tail + BUFFER_SIZE - head - 1) % BUFFER_SIZE;
  if (len > available) {
    CLEAR_LOCK();
    return false;
  }

  // Copy data
  for (size_t i = 0; i < len; i++) {
    buffer[(head + i) % BUFFER_SIZE] = str[i];
  }
  head = (head + len) % BUFFER_SIZE;
  CLEAR_LOCK();
  return true;
}

bool DebugBuffer::put(const __FlashStringHelper *fstr) {
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  size_t len = strlen_P(p);
  if (len >= BUFFER_SIZE)
    return false;

  USE_LOCK();
  SET_LOCK();
  size_t available = (tail + BUFFER_SIZE - head - 1) % BUFFER_SIZE;
  if (len > available) {
    CLEAR_LOCK();
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    buffer[(head + i) % BUFFER_SIZE] = pgm_read_byte(p + i);
  }
  head = (head + len) % BUFFER_SIZE;
  CLEAR_LOCK();
  return true;
}

void DebugBuffer::flush() {
  char out_buf[64];
  while (get(out_buf, sizeof(out_buf))) {
    output_stream->write(out_buf, strlen(out_buf));
  }
  output_stream->flush();
}

// Stream interface implementation
size_t DebugBuffer::write(uint8_t c) { return output_stream->write(c); }

size_t DebugBuffer::write(const uint8_t *buffer, size_t size) {
  return output_stream->write(buffer, size);
}
