#include "GUI.h"
#include "MidiUart.h"
#include "WProgram.h"
#include "Sketch.h"
#include "Pages.h"

#define SCREEN_SAVER_TIME 5

#if defined(MIDIDUINO_USE_GUI) || defined(HOST_MIDIDUINO)

Sketch _defaultSketch((char *)"DFT");

/************************************************/
GuiClass::GuiClass() {
  curLine = LINE1;
  for (uint8_t i = 0; i < 16; i++) {
    lines[0].data[i] = ' ';
    lines[1].data[i] = ' ';
  }
  lines[0].changed = false;
  lines[1].changed = false;
  setSketch(&_defaultSketch);
}

void GuiClass::setSketch(Sketch *_sketch) {
  if (sketch != NULL) {
    sketch->hide();
  }
  sketch = _sketch;
  if (sketch != NULL) {
    sketch->show();
  }
  if (currentPage() != NULL)
    currentPage()->redisplayPage();
}

void GuiClass::setPage(LightPage *page) {
  if (sketch != NULL)
    sketch->setPage(page);
}

void GuiClass::pushPage(LightPage *page) {
  if (sketch != NULL)
    sketch->pushPage(page);
}

void GuiClass::popPage(LightPage *page) {
  if (sketch != NULL)
    sketch->popPage(page);
}

void GuiClass::popPage() {
  if (sketch != NULL)
    sketch->popPage();
}

LightPage *GuiClass::currentPage() {
  if (sketch != NULL)
    return sketch->currentPage();
  else
    return NULL;
}

void GuiClass::redisplay() {
  if (sketch != NULL) {
    PageParent *page = sketch->currentPage();
    if (page != NULL)
      page->redisplay = true;
  }
}

void loop();

void GuiClass::loop() {

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;

  while (!EventRB.isEmpty()) {
    MidiUartParent::handle_midi_lock = 1;
    clock_minutes = 0;
    minuteclock = 0;
    oled_display.screen_saver = false;
    gui_event_t event;
    EventRB.getp(&event);
    for (int i = 0; i < eventHandlers.size; i++) {
      if (eventHandlers.arr[i] != NULL) {
        bool ret = eventHandlers.arr[i](&event);
        if (ret) {
          continue;
        }
      }
    }

    if (sketch != NULL) {
      bool ret = sketch->handleTopEvent(&event);
      if (ret)
        continue;
    }
  }


  MidiUartParent::handle_midi_lock = 1;
  for (int i = 0; i < tasks.size; i++) {
    if (tasks.arr[i] != NULL) {
      tasks.arr[i]->checkTask();
    }
  }

  MidiUartParent::handle_midi_lock = 0;
  if (sketch != NULL) {
    PageParent *page = sketch->currentPage();
    if (page != NULL) {
      page->update();
      page->loop();
    }
  }

  if (sketch != NULL) {
    sketch->loop();
  }
#ifndef HOST_MIDIDUINO
  ::loop();
#endif
  if (use_screen_saver && clock_minutes >= SCREEN_SAVER_TIME) {
#ifdef OLED_DISPLAY
    oled_display.screen_saver = true;
#endif
  }
  MidiUartParent::handle_midi_lock = 0;

  display();

  if (sketch != NULL) {
    PageParent *page = sketch->currentPage();
    if (page != NULL) {
      page->finalize();
    }
  }
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;

}

void GuiClass::display_lcd() {
  PageParent *page = NULL;
  if (sketch != NULL) {
    page = sketch->currentPage();
  }

  for (uint8_t i = 0; i < 2; i++) {
    if (lines[i].flashActive) {
      uint16_t clock = read_slowclock();
      uint16_t diff = clock_diff(lines[i].flashTimer, clock);
      if (diff > lines[i].duration) {
        lines[i].changed = true;
        lines[i].flashActive = false;
      }
      if (lines[i].flashChanged) {
        for (int j = 0; j < 16; j++) {
          if (lines[i].flash[j] == 0) {
            lines[i].flash[j] = ' ';
          }
        }
        LCD.goLine(i);
        LCD.puts(lines[i].flash);
        lines[i].flashChanged = false;
      }
    }

    if (lines[i].changed && !lines[i].flashActive) {
      for (int j = 0; j < 16; j++) {
        if (lines[i].data[j] == 0) {
          lines[i].data[j] = ' ';
        }
      }
      if (page->classic_display) {
        LCD.goLine(i);
        LCD.puts(lines[i].data);
        lines[i].changed = false;
      }
    }
  }
  GUI.setLine(GUI.LINE1);
  GUI.clearFlashLine();
  GUI.setLine(GUI.LINE2);
  GUI.clearFlashLine();


}

void GuiClass::display() {
  PageParent *page = NULL;
  if (sketch != NULL) {
    page = sketch->currentPage();
    if (page != NULL) {
      page->display();
      page->redisplay = false;
    }
  }
  display_lcd();
#ifdef UART_USB
#ifdef OLED_DISPLAY
#ifndef DEBUGMODE
  if (display_mirror) {
    // 7bit encode
    while (!UART_USB_CHECK_EMPTY_BUFFER())
      ;
    UART_USB_WRITE_CHAR(0);

    //  Serial.write(0);

    uint8_t buf[8];

    uint16_t n = 0;

    while (n < 512) {
      buf[0] = 0x80;
      for (uint8_t c = 0; c < 7; c++) {

        buf[c + 1] = 0x80;
        if (n + c < 512) {
          buf[c + 1] |= oled_display.getBuffer(n + c);
        }
        uint8_t msb = oled_display.getBuffer(n + c) >> 7;
        buf[0] |= msb << c;
      }
      for (uint8_t c = 0; c < 8; c++) {
        while (!UART_USB_CHECK_EMPTY_BUFFER())
          ;
        UART_USB_WRITE_CHAR(buf[c]);
      }
      // Serial.write(buf, 8);

      n = n + 7;
    }
  }
#endif
  if (page->classic_display) {
    oled_display.display();
  }
#endif
#endif
}

char hex2c(uint8_t hex) {
  if (hex < 10) {
    return hex + '0';
  } else {
    return hex - 10 + 'a';
  }
}

void GuiClass::put_value(uint8_t idx, uint8_t value) {
  put_value_at(idx << 2, value);
}

void GuiClass::put_value(uint8_t idx, int value) {
  put_value_at(idx << 2, value);
}

void GuiClass::put_value16(uint8_t idx, uint16_t value) {
  put_value16_at(idx << 2, value);
}

void GuiClass::put_valuex(uint8_t idx, uint8_t value) {
  put_valuex_at(idx << 1, value);
}

void GuiClass::put_value_at(uint8_t idx, uint8_t value) {
  put_value_at(idx, value, lines[curLine].data);
  lines[curLine].changed = true;
}
void GuiClass::put_value_at(uint8_t idx, uint8_t value, char *data) {
  data[idx] = value / 100 + '0';
  data[idx + 1] = (value % 100) / 10 + '0';
  data[idx + 2] = (value % 10) + '0';
  data[idx + 3] = ' ';
}

void GuiClass::put_value_at2(uint8_t idx, uint8_t value) {
  put_value_at2(idx, value, lines[curLine].data);  
  lines[curLine].changed = true;
}
void GuiClass::put_value_at2(uint8_t idx, uint8_t value, char *data) {
  data[idx] = (value % 100) / 10 + '0';
  data[idx + 1] = (value % 10) + '0';
}

void GuiClass::put_value_at1(uint8_t idx, uint8_t value) {
  put_value_at1(idx, value, lines[curLine].data);
}

void GuiClass::put_value_at1(uint8_t idx, uint8_t value, char *data) {
  data[idx] = (value % 10) + '0';
  lines[curLine].changed = true;
}

void GuiClass::put_value_at(uint8_t idx, int value) {
  put_value_at(idx, value, lines[curLine].data); 
  lines[curLine].changed = true;
}

void GuiClass::put_value_at(uint8_t idx, int value, char *data) {
  data[idx] = (value % 1000) / 100 + '0';
  data[idx + 1] = (value % 100) / 10 + '0';
  data[idx + 2] = (value % 10) + '0';
  data[idx + 3] = ' ';
}

void GuiClass::put_value16_at(uint8_t idx, uint16_t value) {
  put_value16_at(idx, value, lines[curLine].data);
  lines[curLine].changed = true; 
}

void GuiClass::put_value16_at(uint8_t idx, uint16_t value, char *data) {
  data[idx] = hex2c(value >> 12 & 0xF);
  data[idx + 1] = hex2c(value >> 8 & 0xF);
  data[idx + 2] = hex2c(value >> 4 & 0xF);
  data[idx + 3] = hex2c(value >> 0 & 0xF);
}

void GuiClass::put_valuex_at(uint8_t idx, uint8_t value) {
  put_valuex_at(idx, value, lines[curLine].data);
}

void GuiClass::put_valuex_at(uint8_t idx, uint8_t value, char *data) {
  lines[curLine].changed = true;
  data[idx] = hex2c(value >> 4 & 0xF);
  data[idx + 1] = hex2c(value >> 0 & 0xF);
}

void GuiClass::put_string(uint8_t idx, const char *str) {
  put_string_at(idx << 2, str);
}

void GuiClass::put_string_fill(uint8_t idx, const char *str) {
  put_string_at_fill(idx << 2, str);
}

void GuiClass::put_p_string(uint8_t idx, PGM_P str) {
  put_p_string_at(idx << 2, str);
}

void GuiClass::put_p_string_fill(uint8_t idx, PGM_P str) {
  put_p_string_at_fill(idx << 2, str);
}

void GuiClass::put_string_at_len(uint8_t idx, const char *str, uint8_t len) {
  char *data = lines[curLine].data;
  m_strncpy(data + idx, str, len);
  lines[curLine].changed = true;
}

void GuiClass::put_string_at_not(uint8_t idx, const char *str) {
  char *data = lines[curLine].data;
  m_strncpy(data + idx, str, m_strlen(str) - 1);
  lines[curLine].changed = true;
}

void GuiClass::put_string_at(uint8_t idx, const char *str) {
  char *data = lines[curLine].data;
  m_strncpy(data + idx, str, sizeof(lines[0].data) - idx);
  lines[curLine].changed = true;
}
void GuiClass::put_string_at_noterminator(uint8_t idx, const char *str) {
  char *data = lines[curLine].data;
  m_strncpy(data + idx, str, sizeof(lines[0].data) - idx - 2);
  lines[curLine].changed = true;
}

void GuiClass::put_p_string_at(uint8_t idx, PGM_P str) {
  char *data = lines[curLine].data;
  m_strncpy_p(data + idx, str, sizeof(lines[0].data) - idx);
  lines[curLine].changed = true;
}

void GuiClass::put_string_at_fill(uint8_t idx, const char *str) {
  char *data = lines[curLine].data;
  m_strncpy_fill(data + idx, str, sizeof(lines[0].data) - idx);
  lines[curLine].changed = true;
}

void GuiClass::put_p_string_at_fill(uint8_t idx, PGM_P str) {
  char *data = lines[curLine].data;
  m_strncpy_p_fill(data + idx, str, sizeof(lines[0].data) - idx);
  lines[curLine].changed = true;
}

void GuiClass::put_string_fill(const char *str) { put_string_at_fill(0, str); }

void GuiClass::put_p_string_fill(PGM_P str) { put_p_string_at_fill(0, str); }

void GuiClass::put_string(const char *str) { put_string_at(0, str); }

void GuiClass::put_p_string(PGM_P str) { put_p_string_at(0, str); }

void GuiClass::printf(const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  put_string(buf);
  va_end(lp);
}

void GuiClass::printf_fill(const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  put_string_fill(buf);
  va_end(lp);
}

void GuiClass::printf_at(uint8_t idx, const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  put_string_at(idx, buf);
  va_end(lp);
}

void GuiClass::printf_at_fill(uint8_t idx, const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  put_string_at_fill(idx, buf);
  va_end(lp);
}

void GuiClass::flash_printf(const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  flash_string(buf);
  va_end(lp);
}

void GuiClass::flash_printf_fill(const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  flash_string_fill(buf);
  va_end(lp);
}

void GuiClass::flash_printf_at(uint8_t idx, const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  flash_string_at(idx, buf);
  va_end(lp);
}

void GuiClass::flash_printf_at_fill(uint8_t idx, const char *fmt, ...) {
  va_list lp;
  va_start(lp, fmt);

  char buf[17];
  m_vsnprintf(buf, sizeof(buf), fmt, lp);
  flash_string_at_fill(idx, buf);
  va_end(lp);
}

void GuiClass::clearLines() {
  for (uint8_t a = 0; a < 2; a++) {
    for (uint8_t i = 0; i < sizeof(lines[0].data); i++) {
      lines[a].data[i] = ' ';
    }
    lines[a].changed = true;
  }
}

void GuiClass::clearLine() {
  for (uint8_t i = 0; i < sizeof(lines[0].data); i++)
    lines[curLine].data[i] = ' ';
  lines[curLine].changed = true;
}

void GuiClass::flash(uint16_t duration) {
  lines[curLine].flashChanged = lines[curLine].flashActive = true;
  lines[curLine].duration = duration;
  lines[curLine].flashTimer = read_slowclock();
}

void GuiClass::clearFlashLine() {
  for (uint8_t i = 0; i < sizeof(lines[0].data); i++)
    lines[curLine].flash[i] = ' ';
}

void GuiClass::clearFlash(uint16_t duration) {
  for (uint8_t i = 0; i < sizeof(lines[0].data); i++)
    lines[curLine].flash[i] = ' ';
  flash(duration);
}

void GuiClass::flash_put_value(uint8_t idx, uint8_t value, uint16_t duration) {
  flash_put_value_at(idx << 2, value, duration);
}

void GuiClass::flash_put_value16(uint8_t idx, uint16_t value,
                                 uint16_t duration) {
  flash_put_value16_at(idx << 2, value, duration);
}

void GuiClass::flash_put_valuex(uint8_t idx, uint8_t value, uint16_t duration) {
  flash_put_valuex_at(idx << 1, value, duration);
}

void GuiClass::flash_put_value_at(uint8_t idx, uint8_t value,
                                  uint16_t duration) {
  char *data = lines[curLine].flash;
  data[idx] = value / 100 + '0';
  data[idx + 1] = (value % 100) / 10 + '0';
  data[idx + 2] = (value % 10) + '0';
  data[idx + 3] = ' ';
  flash(duration);
}

void GuiClass::flash_put_value16_at(uint8_t idx, uint16_t value,
                                    uint16_t duration) {
  char *data = lines[curLine].flash;
  data[idx] = hex2c(value >> 12 & 0xF);
  data[idx + 1] = hex2c(value >> 8 & 0xF);
  data[idx + 2] = hex2c(value >> 4 & 0xF);
  data[idx + 3] = hex2c(value >> 0 & 0xF);
  flash(duration);
}

void GuiClass::flash_put_valuex_at(uint8_t idx, uint8_t value,
                                   uint16_t duration) {
  char *data = lines[curLine].flash;
  data[idx] = hex2c(value >> 4 & 0xF);
  data[idx + 1] = hex2c(value >> 0 & 0xF);
  flash(duration);
}

void GuiClass::flash_string_at(uint8_t idx, const char *str,
                               uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_string_at_fill(uint8_t idx, const char *str,
                                    uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy_fill(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_p_string_at(uint8_t idx, PGM_P str, uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy_p(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_p_string_at_fill(uint8_t idx, PGM_P str,
                                      uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy_p_fill(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_string(const char *str, uint16_t duration) {
  flash_string_at(0, str, duration);
}

void GuiClass::flash_p_string(PGM_P str, uint16_t duration) {
  flash_p_string_at(0, str, duration);
}

void GuiClass::flash_string_fill(const char *str, uint16_t duration) {
  flash_string_at_fill(0, str, duration);
}

void GuiClass::flash_p_string_fill(PGM_P str, uint16_t duration) {
  flash_p_string_at_fill(0, str, duration);
}

void GuiClass::flash_string_clear(const char *str, uint16_t duration) {
  setLine(LINE1);
  flash_string_fill(str, duration);
  setLine(LINE2);
  clearFlash(duration);
}

void GuiClass::flash_p_string_clear(const char *str, uint16_t duration) {
  setLine(LINE1);
  flash_p_string_fill(str, duration);
  setLine(LINE2);
  clearFlash(duration);
}

void GuiClass::flash_strings_fill(const char *str1, const char *str2,
                                  uint16_t duration) {
  setLine(LINE1);
  flash_string_fill(str1, duration);
  setLine(LINE2);
  flash_string_fill(str2, duration);
}

void GuiClass::flash_p_strings_fill(PGM_P str1, PGM_P str2, uint16_t duration) {
  setLine(LINE1);
  flash_p_string_fill(str1, duration);
  setLine(LINE2);
  flash_p_string_fill(str2, duration);
}

GuiClass GUI;

#endif
