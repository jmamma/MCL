#include "GUI.h"
#include "MidiUart.h"
#include "Pages.h"
#include "Sketch.h"
#include "WProgram.h"

#define SCREEN_SAVER_TIME 5

#if defined(MIDIDUINO_USE_GUI) || defined(HOST_MIDIDUINO)

Sketch _defaultSketch((char *)"DFT");

/************************************************/
GuiClass::GuiClass() { setSketch(&_defaultSketch); }

void GuiClass::setSketch(Sketch *_sketch) {
  if (sketch != NULL) {
    sketch->hide();
  }
  sketch = _sketch;
  if (sketch != NULL) {
    sketch->show();
  }
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

void loop();

void GuiClass::loop() {

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;

 MidiUartParent::handle_midi_lock = 1;
  while (!EventRB.isEmpty()) {
    clock_minutes = 0;
    oled_display.screen_saver = false;
    gui_event_t event;
    EventRB.getp(&event);
    bool ret = false;
    if (sketch != NULL) {
      ret = sketch->handleTopEvent(&event);
      if (ret)
        continue;
    }

    for (uint8_t i = 0; i < eventHandlers.size; i++) {
      if (eventHandlers.arr[i] != NULL) {
        ret = eventHandlers.arr[i](&event);
        if (ret)
          continue;
      }
    }

  }

  for (uint8_t i = 0; i < tasks.size; i++) {
    if (tasks.arr[i] != NULL) {
      tasks.arr[i]->checkTask();
    }
  }

  MidiUartParent::handle_midi_lock = 0;
  if (sketch != NULL) {
    PageParent *page = sketch->currentPage();
    if (page != NULL) {

      MidiUartParent::handle_midi_lock = 1;
      page->update();
      MidiUartParent::handle_midi_lock = 0;
      page->loop();
      page->finalize();
    }
  }

  if (use_screen_saver && clock_minutes >= SCREEN_SAVER_TIME) {
    oled_display.screen_saver = true;
  }
  MidiUartParent::handle_midi_lock = 0;

  display();

  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void GuiClass::display_lcd() {}

#ifdef DEBUGMODE
uint8_t fps = 0;
uint16_t fps_clock = 0;
#endif

void GuiClass::display() {
  PageParent *page = NULL;
  if (sketch != NULL) {
    page = sketch->currentPage();
    if (page != NULL) {
      oled_display.setFont();
      page->display();
    }
  }

#ifdef DEBUGMODE
  if (fps == 0 && fps_clock == 0) {
    fps_clock = slowclock;
  }
  fps++;
  if (clock_diff(fps_clock, slowclock) > 1000) {
    DEBUG_PRINT("FPS: ");
    DEBUG_PRINTLN(fps);
    fps = 0;
    fps_clock = slowclock;
  }
#endif
#ifdef UART_USB
#ifndef DEBUGMODE
  if (display_mirror) {
    // 7bit encode
    MidiUartParent::handle_midi_lock = 1;
    uint8_t change_mode_msg[] = {0xF0, 0x7D, 0x4D, 0x43, 0x4C, 0x40};
    MidiUartUSB.m_putc(change_mode_msg, sizeof(change_mode_msg));

    uint8_t buf[8];

    uint16_t n = 0;

    while (n < 512) {
      buf[0] = 0;
      for (uint8_t c = 0; c < 7; c++) {

        buf[c + 1] = 0;
        if (n + c < 512) {
          buf[c + 1] |= oled_display.getBuffer(n + c) & 0x7F;
        }
        uint8_t msb = oled_display.getBuffer(n + c) >> 7;
        buf[0] |= msb << c;
      }
      MidiUartUSB.m_putc(buf, sizeof(buf));
      n = n + 7;
    }
    MidiUartUSB.m_putc(0xF7);
    MidiUartParent::handle_midi_lock = 0;
  }
#endif
#endif
  oled_display.display();
}

char hex2c(uint8_t hex) {
  if (hex < 10) {
    return hex + '0';
  } else {
    return hex - 10 + 'a';
  }
}

GuiClass GUI;

#endif
