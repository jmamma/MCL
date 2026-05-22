/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "GUI.h"
#include "Pages.h"
#include "platform.h"
#include "DiagnosticPage.h"
#include "Encoders.h"
#include "KeyInterface.h"
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
  encoder_focus = ENCODER_FOCUS_NONE;
  encoder_key_control_mask = 0x0F;
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

  if (encoder_focus != ENCODER_FOCUS_NONE &&
      (encoder_focus >= GUI_NUM_ENCODERS || encoders[encoder_focus] == NULL ||
       !(encoder_key_control_mask & (1 << encoder_focus)) ||
       clock_diff(encoders_used_clock[encoder_focus], read_clock_ms()) >=
           SHOW_VALUE_TIMEOUT)) {
    resetEncoderFocus();
  }
}

void LightPage::init_encoders_used_clock(uint16_t timeout) {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] == NULL) {
      continue;
    }
    encoders[i]->old = encoders[i]->cur;
    ((LightPage *)this)->encoders_used_clock[i] =
        read_clock_ms() + timeout + 1;
  }
  resetEncoderFocus();
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

bool LightPage::selectEncoderFocus(int8_t start, int8_t step) {
  for (int8_t i = start; i >= 0 && i < GUI_NUM_ENCODERS; i += step) {
    if (encoders[i] != NULL && (encoder_key_control_mask & (1 << i))) {
      encoder_focus = i;
      return true;
    }
  }
  return false;
}

bool LightPage::handleEncoderKeyControls(gui_event_t *event) {
  if (!encoder_key_control_mask || !EVENT_CMD(event) ||
      event->mask != EVENT_BUTTON_PRESSED) {
    return false;
  }

  uint8_t key = event->source;
  int8_t move = 0;
  int8_t nudge = 0;

  if (key == MDX_KEY_NO && encoder_focus != ENCODER_FOCUS_NONE) {
    resetEncoderFocus();
    GUI.wake_screen_saver();
    g_clock_minutes = 0;
    g_clock_ticks = 0;
    return true;
  }

  switch (key) {
  case MDX_KEY_LEFT:
    move = -1;
    break;
  case MDX_KEY_RIGHT:
    move = 1;
    break;
  case MDX_KEY_UP:
    nudge = 1;
    break;
  case MDX_KEY_DOWN:
    nudge = -1;
    break;
  default:
    return false;
  }

  const bool func_down = key_interface.is_key_down(MDX_KEY_FUNC);
  const bool other_modifier_down =
      key_interface.is_key_down(MDX_KEY_YES) ||
      key_interface.is_key_down(MDX_KEY_NO) ||
      key_interface.is_key_down(MDX_KEY_PATSONG);
  const bool focus_active =
      encoder_focus < GUI_NUM_ENCODERS && encoders[encoder_focus] != NULL &&
      (encoder_key_control_mask & (1 << encoder_focus));

  if (move != 0 && focus_active && func_down && !other_modifier_down) {
    if (!moveEncoderFocusPage(move)) {
      return false;
    }
  } else if (func_down || other_modifier_down) {
    return false;
  } else if (encoder_focus >= GUI_NUM_ENCODERS || encoders[encoder_focus] == NULL ||
      !(encoder_key_control_mask & (1 << encoder_focus))) {
    encoder_focus = ENCODER_FOCUS_NONE;
    if (!selectEncoderFocus(0, 1)) {
      return false;
    }
  } else if (move != 0) {
    if (!selectEncoderFocus((int8_t)encoder_focus + move, move) &&
        moveEncoderFocusPage(move)) {
      resetEncoderFocus();
      selectEncoderFocus(move > 0 ? 0 : GUI_NUM_ENCODERS - 1,
                         move > 0 ? 1 : -1);
    }
  } else {
    Encoder *encoder = encoders[encoder_focus];
    encoder_t key_encoder = {};
    key_encoder.normal =
        nudge * (encoder->rot_res * ENCODER_RES_MULTIPLIER + 1);
    encoder->update(&key_encoder);
  }

  GUI.wake_screen_saver();
  g_clock_minutes = 0;
  g_clock_ticks = 0;
  if (encoder_focus != ENCODER_FOCUS_NONE) {
    encoders_used_clock[encoder_focus] = read_clock_ms();
  }
  return true;
}
