/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"
#include "Pages.h"
#include "WProgram.h"

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

void Page::update() {}

void PageParent::redisplayPage() {
  if (displaymode == DISPLAY_TEXT_MODE0) {
    GUI.setLine(GUI.LINE1);
    GUI.clearLine();
    GUI.setLine(GUI.LINE2);
    GUI.clearLine();
    redisplay = true;
  } else {
  }
}

uint16_t LightPage::encoders_used_clock[GUI_NUM_ENCODERS];

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
        GUI.screen_saver = false;
        clock_minutes = 0;
        minuteclock = 0;
        encoders_used_clock[i] = slowclock;
        redisplay = true;
      }
    }
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

