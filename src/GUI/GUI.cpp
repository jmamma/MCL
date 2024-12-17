#include "GUI.h"
#include "Pages.h"
#include "MidiUart.h"
#include "global.h"
#include "oled.h"

void GuiClass::setPage(LightPage *page) {
  if (currentPage() != NULL) {
    currentPage()->cleanup();
  }
  pageStack.reset();
  pushPage(page);
}

void GuiClass::pushPage(LightPage *page) {
  if (currentPage() == page) {
    DEBUG_PRINTLN(F("can't push twice"));
    return;
  }
  DEBUG_PRINTLN(F("Pushing page"));
  if (!page->isSetup) {
    page->setup();
    page->isSetup = true;
  }

  pageStack.push(page);
  page->init();
  page->show();
  page->init_encoders_used_clock();
#ifdef ENABLE_DIAG_LOGGING
  diag_page.deactivate();
#endif
}

void GuiClass::popPage(LightPage *page) {
  if (currentPage() == page) {
    popPage();
  }
}

void GuiClass::popPage() {
  LightPage *lastpage = currentPage();
  if (lastpage != NULL) {
    lastpage->cleanup();
  }
  LightPage *page = nullptr;
  if (!pageStack.pop(&page)) { goto reset; }
  if (page != nullptr) {
    page->hide();
  }
  page = currentPage();
  if (page == nullptr) {
    reset:
    if (lastpage == NULL) { return; }
    pageStack.reset();
    pushPage(lastpage);
  }
  currentPage()->init();
}

LightPage *GuiClass::currentPage() {
  LightPage *page = NULL;
  pageStack.peek(&page);
  return page;
}

bool GuiClass::handleTopEvent(gui_event_t *event) {
  LightPage *page = currentPage();
  if (page != NULL) {
    return page->handleEvent(event);
  }
  return false;
}

void GuiClass::loop() {
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;

  while (!events.isEmpty()) {
    g_clock_minutes = 0;
    if (screen_saver) { oled_display.wake(); screen_saver = false; }
    gui_event_t event;
    events.getEvent(&event);

    bool ret = handleTopEvent(&event);
    if (!ret) {
      for (uint8_t i = 0; i < eventHandlers.size; i++) {
        if (eventHandlers.arr[i] != NULL) {
          ret = eventHandlers.arr[i](&event);
          if (ret) break;
        }
      }
    }
  }

  for (uint8_t i = 0; i < tasks.size; i++) {
    if (tasks.arr[i] != NULL) {
      tasks.arr[i]->checkTask();
    }
  }

  MidiUartParent::handle_midi_lock = 0;
  PageParent *page = currentPage();
  if (page != NULL) {
    MidiUartParent::handle_midi_lock = 1;
    page->update();
    MidiUartParent::handle_midi_lock = 0;
    page->loop();
    page->finalize();
  }

  if (use_screen_saver && g_clock_minutes >= SCREEN_SAVER_TIMEOUT) {
    screen_saver = true;
    oled_display.sleep();
  }
  MidiUartParent::handle_midi_lock = 0;

  display();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void GuiClass::mirror() {
    // 7bit encode
    MidiUartParent::handle_midi_lock = 1;
    uint8_t change_mode_msg[] = {0xF0, 0x7D, 0x4D, 0x43, 0x4C, 0x40};
    MidiUartUSB.m_putc(change_mode_msg, sizeof(change_mode_msg));

    uint8_t buf[8];
    uint8_t *ptr = oled_display.getBuffer();
    uint16_t n = 0;

    while (n < 512) {
      buf[0] = 0;
      for (uint8_t c = 0; c < 7; c++) {

        buf[c + 1] = 0;
        if (n + c < 512) {
          buf[c + 1] |= ptr[n + c] & 0x7F;
        }
        uint8_t msb = ptr[n + c] >> 7;
        buf[0] |= msb << c;
      }
      MidiUartUSB.m_putc(buf, sizeof(buf));
      n = n + 7;
    }
    MidiUartUSB.m_putc(0xF7);
    MidiUartParent::handle_midi_lock = 0;

}

void GuiClass::display() {
  LightPage *page = currentPage();
  if (page != NULL) {
    oled_display.setFont();
    page->display();
  }

#ifdef DEBUGMODE
  if (g_fps == 0 && g_clock_fps == 0) {
    g_clock_fps = g_clock_ms;
  }
  g_fps++;
  if (clock_diff(g_clock_fps, g_clock_ms) > 1000) {
    DEBUG_PRINT("FPS: ");
    DEBUG_PRINTLN(g_fps);
    g_fps = 0;
    g_clock_fps = g_clock_ms;
  }
#endif
#ifdef UART_USB
#ifndef DEBUGMODE
  if (display_mirror) {
    mirror();
  }
#endif
#endif
  oled_display.display();
}

GuiClass::GuiClass() {
  // Initialize any necessary state
}
