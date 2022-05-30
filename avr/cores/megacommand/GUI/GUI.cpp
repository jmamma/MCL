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
#ifdef UART_USB
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
