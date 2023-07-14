/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"
#include "Pages.h"
#include "WProgram.h"
#include "DiagnosticPage.h"
#include "Encoders.h"
/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_pages GUI Pages
 *
 * @{
 *
 * \file
 * GUI Pages
 **/

uint16_t LightPage::encoders_used_clock[4];

void Page::update() {}

void PageContainer::pushPage(LightPage* page) {
  if (currentPage() == page) {
    DEBUG_PRINTLN(F("can't push twice"));
    // can't push the same page twice in a row
    return;
  }
  DEBUG_PRINTLN(F("Pushing page"));
  page->parent = this;
  if (!page->isSetup) {
    page->setup();
    page->isSetup = true;
  }

  pageStack.push(page);
  page->init();
  page->show();
#ifdef ENABLE_DIAG_LOGGING
  // deactivate diagnostic page on pushPage
  diag_page.deactivate();
#endif
}

void LightPage::update() {
  encoder_t _encoders[GUI_NUM_ENCODERS];

  USE_LOCK();
  SET_LOCK();
  memcpy(_encoders, Encoders.encoders, sizeof(_encoders));
  Encoders.clearEncoders();
  CLEAR_LOCK();

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL) {
      encoders[i]->update(_encoders + i);
      if (encoders[i]->hasChanged()) {
#ifdef OLED_DISPLAY
        oled_display.screen_saver = false;
#endif
        clock_minutes = 0;
        minuteclock = 0;
        encoders_used_clock[i] = slowclock;
      }
    }
  }
}

void LightPage::init_encoders_used_clock(uint16_t timeout) {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
      encoders[i]->old = encoders[i]->cur;
      ((LightPage *)this)->encoders_used_clock[i] =
          slowclock + timeout + 1;
  }
}

void LightPage::clear() {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL)
      encoders[i]->clear();
  }
}

void LightPage::finalize() {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL)
      encoders[i]->checkHandle();
  }
}

