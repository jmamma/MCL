#pragma once
class DebugBuffer {
  static constexpr size_t BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  volatile uint32_t head __attribute__((aligned(4))) = 0;
  volatile uint32_t tail __attribute__((aligned(4))) = 0;
  char temp_buf[64];

  // Atomic increment with wrap-around
  uint32_t atomic_increment(volatile uint32_t &val) {
    uint32_t result;
    uint32_t save = save_and_disable_interrupts();
    result = val;
    val = (val + 1) % BUFFER_SIZE;
    restore_interrupts(save);
    return result;
  }

public:
  bool put(const char *str) {
    size_t len = strlen(str);
    if (len >= BUFFER_SIZE)
      return false;

    // Check available space atomically
    uint32_t save = save_and_disable_interrupts();
    size_t available = (tail + BUFFER_SIZE - head - 1) % BUFFER_SIZE;
    if (len > available) {
      restore_interrupts(save);
      return false;
    }

    // Copy data
    for (size_t i = 0; i < len; i++) {
      buffer[(head + i) % BUFFER_SIZE] = str[i];
    }
    head = (head + len) % BUFFER_SIZE;
    restore_interrupts(save);
    return true;
  }
  bool put(const __FlashStringHelper *fstr) {
    PGM_P p = reinterpret_cast<PGM_P>(fstr);
    size_t len = strlen_P(p);
    if (len >= BUFFER_SIZE)
      return false;

    uint32_t save = save_and_disable_interrupts();
    size_t available = (tail + BUFFER_SIZE - head - 1) % BUFFER_SIZE;
    if (len > available) {
      restore_interrupts(save);
      return false;
    }

    for (size_t i = 0; i < len; i++) {
      buffer[(head + i) % BUFFER_SIZE] = pgm_read_byte(p + i);
    }
    head = (head + len) % BUFFER_SIZE;
    restore_interrupts(save);
    return true;
  }
  bool put(const String &str) { return put(str.c_str()); }

  bool put(const StringSumHelper &str) { return put(String(str).c_str()); }
void flush() {
    char out_buf[64];
    size_t pos = 0;
    bool last_was_newline = false;

    while (get(out_buf, sizeof(out_buf))) {
        // No trimming or skipping of whitespace
        Serial.write(out_buf, strlen(out_buf));
    }
}
private:
    bool get(char* str, size_t maxLen) {
        if (head == tail)
            return false;

        uint32_t save = save_and_disable_interrupts();
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
        restore_interrupts(save);
        return (i > 0);
    }
};
