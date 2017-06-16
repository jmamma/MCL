#include "WProgram.h"
#include "GUI.h"

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
  if (sketch !=NULL) {
    sketch->hide();
  }
  sketch = _sketch;
  if (sketch !=NULL) {
    sketch->show();
  }
  if (currentPage() != NULL)
    currentPage()->redisplayPage();
}  

void GuiClass::setPage(Page *page) {
  if (sketch != NULL)
    sketch->setPage(page);
}

void GuiClass::pushPage(Page *page) {
  if (sketch != NULL)
    sketch->pushPage(page);
}

void GuiClass::popPage(Page *page) {
  if (sketch != NULL)
    sketch->popPage(page);
}

void GuiClass::popPage() {
  if (sketch != NULL)
    sketch->popPage();
}

Page *GuiClass::currentPage() {
  if (sketch != NULL)
    return sketch->currentPage();
  else
    return NULL;
}


void GuiClass::redisplay() {
  if (sketch != NULL) {
    Page *page = sketch->currentPage();
    if (page != NULL)
      page->redisplay = true;
  }
}

void loop();

void GuiClass::loop() {
  for (int i = 0; i < tasks.size; i++) {
    if (tasks.arr[i] != NULL) {
      tasks.arr[i]->checkTask();
    }
  }

  while (!EventRB.isEmpty()) {
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

  if (sketch != NULL) {
    Page *page = sketch->currentPage();
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

  display();

  if (sketch != NULL) {
    Page *page = sketch->currentPage();
    if (page != NULL) {
      page->finalize();
    }
  }
}

void GuiClass::display() {
  if (sketch != NULL) {
    Page *page = sketch->currentPage();
    if (page != NULL) {
      page->display();
      page->redisplay = false;
    }
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
#ifdef HOST_MIDIDUINO
				printf("%s\n", lines[i].flash);
#else
				LCD.goLine(i);
				LCD.puts(lines[i].flash);
#endif
				lines[i].flashChanged = false;
      }
    }

    if (lines[i].changed && !lines[i].flashActive) {
      for (int j = 0; j < 16; j++) {
				if (lines[i].data[j] == 0) {
					lines[i].data[j] = ' ';
				}
      }
#ifdef HOST_MIDIDUINO
			printf("%s\n", lines[i].data);
#else
      LCD.goLine(i);
      LCD.puts(lines[i].data);
#endif
      lines[i].changed = false;
    }
  }
  GUI.setLine(GUI.LINE1);
  GUI.clearFlashLine();
  GUI.setLine(GUI.LINE2);
  GUI.clearFlashLine();
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
  char *data = lines[curLine].data;
  lines[curLine].changed = true;
  data[idx] = value / 100 + '0';
  data[idx+1] = (value % 100) / 10 + '0';
  data[idx+2] = (value % 10) + '0';
  data[idx+3] = ' ';
}

void GuiClass::put_value_at2(uint8_t idx, uint8_t value) {
  char *data = lines[curLine].data;
  lines[curLine].changed = true;
  data[idx] = (value % 100) / 10 + '0';
  data[idx+1] = (value % 10) + '0';
}


void GuiClass::put_value_at(uint8_t idx, int value) {
  char *data = lines[curLine].data;
  lines[curLine].changed = true;
  data[idx] = (value % 1000) / 100 + '0';
  data[idx+1] = (value % 100) / 10 + '0';
  data[idx+2] = (value % 10) + '0';
  data[idx+3] = ' ';
}

void GuiClass::put_value16_at(uint8_t idx, uint16_t value) {
  char *data = lines[curLine].data;
  lines[curLine].changed = true;
  data[idx]   = hex2c(value >> 12 & 0xF);
  data[idx+1] = hex2c(value >> 8 & 0xF);
  data[idx+2] = hex2c(value >> 4 & 0xF);
  data[idx+3] = hex2c(value >> 0 & 0xF);
}

void GuiClass::put_valuex_at(uint8_t idx, uint8_t value) {
  char *data = lines[curLine].data;
  lines[curLine].changed = true;
  data[idx]   = hex2c(value >> 4 & 0xF);
  data[idx+1] = hex2c(value >> 0 & 0xF);
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


void GuiClass::put_string_fill(const char *str) {
  put_string_at_fill(0, str);
}

void GuiClass::put_p_string_fill(PGM_P str) {
  put_p_string_at_fill(0, str);
}

void GuiClass::put_string(const char *str) {
  put_string_at(0, str);
}

void GuiClass::put_p_string(PGM_P str) {
  put_p_string_at(0, str);
}

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

void GuiClass::flash_put_value16(uint8_t idx, uint16_t value, uint16_t duration) {
  flash_put_value16_at(idx << 2, value, duration);
}

void GuiClass::flash_put_valuex(uint8_t idx, uint8_t value, uint16_t duration) {
  flash_put_valuex_at(idx << 1, value, duration);
}

void GuiClass::flash_put_value_at(uint8_t idx, uint8_t value, uint16_t duration) {
  char *data = lines[curLine].flash;
  data[idx] = value / 100 + '0';
  data[idx+1] = (value % 100) / 10 + '0';
  data[idx+2] = (value % 10) + '0';
  data[idx+3] = ' ';
  flash(duration);
}

void GuiClass::flash_put_value16_at(uint8_t idx, uint16_t value, uint16_t duration) {
  char *data = lines[curLine].flash;
  data[idx]   = hex2c(value >> 12 & 0xF);
  data[idx+1] = hex2c(value >> 8 & 0xF);
  data[idx+2] = hex2c(value >> 4 & 0xF);
  data[idx+3] = hex2c(value >> 0 & 0xF);
  flash(duration);
}

void GuiClass::flash_put_valuex_at(uint8_t idx, uint8_t value, uint16_t duration) {
  char *data = lines[curLine].flash;
  data[idx]   = hex2c(value >> 4 & 0xF);
  data[idx+1] = hex2c(value >> 0 & 0xF);
  flash(duration);
}

void GuiClass::flash_string_at(uint8_t idx, const char *str, uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_string_at_fill(uint8_t idx, const char *str, uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy_fill(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_p_string_at(uint8_t idx, PGM_P str, uint16_t duration) {
  char *data = lines[curLine].flash;
  m_strncpy_p(data + idx, str, sizeof(lines[0].flash) - idx);
  flash(duration);
}

void GuiClass::flash_p_string_at_fill(uint8_t idx, PGM_P str, uint16_t duration) {
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

void GuiClass::flash_strings_fill(const char *str1, const char *str2, uint16_t duration) {
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
