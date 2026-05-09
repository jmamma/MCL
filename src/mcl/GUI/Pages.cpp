/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"
#include "Pages.h"
#include "platform.h"
#include "DiagnosticPage.h"
#include "Encoders.h"
#include "global.h"
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

LightPage::LightPage(Encoder *e1, Encoder *e2, Encoder *e3, Encoder *e4) {
  encoders[0] = e1;
  encoders[1] = e2;
  encoders[2] = e3;
  encoders[3] = e4;
  isSetup = false;
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
        GUI.wake_screen_saver();
        g_clock_minutes = 0;
        g_clock_ticks = 0;
        encoders_used_clock[i] = read_clock_ms();
      }
    }
  }
}

void LightPage::init_encoders_used_clock(uint16_t timeout) {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
      encoders[i]->old = encoders[i]->cur;
      ((LightPage *)this)->encoders_used_clock[i] =
          read_clock_ms() + timeout + 1;
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
