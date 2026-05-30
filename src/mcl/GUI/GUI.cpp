#include "GUI.h"
#include "Pages.h"
#include "MidiUart.h"
#include "global.h"
#include "oled.h"
#include "PlatformPanel.h"
#if defined(PLATFORM_WASM)
#include "MCLGUI.h"
#endif

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

void GuiClass::popPage(bool re_init) {
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
  if (re_init) {
    currentPage()->init();
  }
}

LightPage *GuiClass::currentPage() {
  LightPage *page = NULL;
  pageStack.peek(&page);
  return page;
}

bool GuiClass::handleTopEvent(gui_event_t *event) {
#ifdef MCL_HAS_EXTENDED_PANEL_INPUT
  // Extended panel input (TBD hardware or wasm host buttons) preempts the
  // active page so raw physical buttons can become MCL/MDX commands first.
  if (platform_panel_handleEvent(event)) return true;
#endif
#ifdef MCL_HAS_TBD_DRIVER
  // TBD overlays still get a first pass after the physical panel router.
  if (overlay && overlay->handleEvent(event)) return true;
#endif
  LightPage *page = currentPage();
  if (page != NULL) {
    if (page->handleEvent(event)) {
      return true;
    }
    return page->handleEncoderKeyControls(event);
  }
  return false;
}

#if defined(MCL_HAS_DESKTOP_MOUSE)
bool GuiClass::handleMouseEvent(mcl_mouse_event_t *event) {
  if (event == NULL) {
    return false;
  }
  wake_screen_saver();
#ifdef MCL_HAS_TBD_DRIVER
  if (overlay && overlay->handleEncoderMouseEvent(event)) return true;
  if (overlay && overlay->handleMouseEvent(event)) return true;
#endif
  LightPage *page = currentPage();
  if (page != NULL) {
    if (page->handleEncoderMouseEvent(event)) {
      return true;
    }
    return page->handleMouseEvent(event);
  }
  return false;
}

void GuiClass::queueVirtualButton(uint8_t button, bool pressed) {
  gui_event_t event = {};
  event.source = button;
  event.type = BUTTON;
  event.mask = pressed ? EVENT_BUTTON_PRESSED : EVENT_BUTTON_RELEASED;
  event.modifiers = 0;
  putEvent(&event);
}

void GuiClass::pollMouseEvents() {
  mcl_mouse_event_t event;
  uint8_t limit = 16;
  while (limit-- > 0 && mcl_platform_mouse_pop(&event)) {
    handleMouseEvent(&event);
  }
}
#endif

void GuiClass::wake_screen_saver() {
  if (screen_saver) { oled_display.wake(); screen_saver = false; }
}

void GuiClass::loop() {
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;

  while (!events.isEmpty()) {
    g_clock_minutes = 0;
    wake_screen_saver();
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

#if defined(MCL_HAS_DESKTOP_MOUSE)
  pollMouseEvents();
#endif

  for (uint8_t i = 0; i < tasks.size; i++) {
    if (tasks.arr[i] != NULL) {
      tasks.arr[i]->checkTask();
    }
  }

  MidiUartParent::handle_midi_lock = 0;
  LightPage *page = currentPage();
  bool overlay_captures_encoders = false;
#ifdef PLATFORM_TBD
  overlay_captures_encoders = overlayCapturesEncoders();
#endif
  if (page != NULL) {
    if (!overlay_captures_encoders) {
      MidiUartParent::handle_midi_lock = 1;
      page->update();
      MidiUartParent::handle_midi_lock = 0;
    }
    page->loop();
    if (!overlay_captures_encoders) {
      page->finalize();
    }
  }

#ifdef PLATFORM_TBD
  if (overlay_captures_encoders && overlay) {
    LightPage *active_overlay = overlay;
    MidiUartParent::handle_midi_lock = 1;
    active_overlay->update();
    MidiUartParent::handle_midi_lock = 0;
  }
  // Tick the overlay's loop after the active page so it can manage
  // its own state (LED palette, sub-page repaint, etc.).
  if (overlay) {
    LightPage *active_overlay = overlay;
    active_overlay->loop();
    if (overlay == active_overlay && overlay_captures_encoders) {
      active_overlay->finalize();
    }
  }
#endif

  if (use_screen_saver && g_clock_minutes >= SCREEN_SAVER_TIMEOUT && !screen_saver) {
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
#if defined(MCL_HAS_DESKTOP_MOUSE)
    page->clearPageEncoderHits();
#endif
    page->display();
  }

#ifdef PLATFORM_TBD
  // Overlay renders on top of the active page. Lifecycle is owned by
  // setOverlay / clearOverlay — single render hook here, no state.
  if (overlay) {
    oled_display.setFont();
#if defined(MCL_HAS_DESKTOP_MOUSE)
    overlay->clearPageEncoderHits();
#endif
    overlay->display();
  }
#endif

#ifdef DEBUGMODE
  if (g_fps == 0 && g_clock_fps == 0) {
    g_clock_fps = read_clock_ms();
  }
  g_fps++;
  if (clock_diff(g_clock_fps, read_clock_ms()) > 1000) {
    DEBUG_PRINT("FPS: ");
    DEBUG_PRINTLN(g_fps);
    g_fps = 0;
    g_clock_fps = read_clock_ms();
  }
#endif
#ifndef DEBUGMODE
#if defined(__AVR__)
  if (display_mirror) {
    mirror();
  }
#else
  if (display_mirror) {
    static uint16_t last_mirror_time = 0;
    uint16_t current_time = read_clock_ms();
    if (clock_diff(last_mirror_time, current_time) >= 40) { // 40ms = 25fps
      mirror();
      last_mirror_time = current_time;
    }
  }
#endif
#endif
#if defined(PLATFORM_WASM)
  mcl_gui.draw_async_infobox();
#endif
  oled_display.display();
}

GuiClass::GuiClass() {
  // Initialize any necessary state
}

#ifdef PLATFORM_TBD
void GuiClass::setOverlay(LightPage *p) {
  if (overlay == p) return;
  if (overlay) overlay->cleanup();
  overlay = p;
  if (overlay) {
    if (!overlay->isSetup) {
      overlay->setup();
      overlay->isSetup = true;
    }
    overlay->init();
  }
}

void GuiClass::clearOverlay() {
  if (!overlay) return;
  overlay->cleanup();
  overlay = nullptr;
}

bool GuiClass::overlayCapturesEncoders() const {
  if (!overlay) return false;
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (overlay->encoders[i]) return true;
  }
  return false;
}
#endif
