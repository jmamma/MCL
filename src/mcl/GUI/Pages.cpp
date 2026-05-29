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
#if defined(MCL_HAS_DESKTOP_MOUSE)
  mouse_encoder_focus = ENCODER_FOCUS_NONE;
  mouse_encoder_drag_origin_y = 0;
  mouse_encoder_drag_last_ticks = 0;
  mouse_encoder_press_buttons = 0;
  mouse_encoder_was_dragged = false;
  clearPageEncoderHits();
#endif
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
#if defined(MCL_HAS_DESKTOP_MOUSE)
  resetMouseEncoderDrag();
#endif
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

  if (func_down && !other_modifier_down) {
    if (!focus_active) {
      encoder_focus = ENCODER_FOCUS_NONE;
      int8_t start = move < 0 ? GUI_NUM_ENCODERS - 1 : 0;
      int8_t step = move < 0 ? -1 : 1;
      if (!selectEncoderFocus(start, step)) {
        return false;
      }
    }
    if (move != 0) {
      moveEncoderFocusPage(move);
      if (encoder_focus >= GUI_NUM_ENCODERS || encoders[encoder_focus] == NULL ||
          !(encoder_key_control_mask & (1 << encoder_focus))) {
        encoder_focus = ENCODER_FOCUS_NONE;
        selectEncoderFocus(move > 0 ? 0 : GUI_NUM_ENCODERS - 1,
                           move > 0 ? 1 : -1);
      }
    } else if (nudge != 0) {
      Encoder *encoder = encoders[encoder_focus];
      encoder_t key_encoder = {};
      key_encoder.normal =
          nudge * (encoder->rot_res * ENCODER_RES_MULTIPLIER + 1);
      key_encoder.button = true;
      encoder->update(&key_encoder);
    }
  } else if (func_down || other_modifier_down) {
    return false;
  } else if (!focus_active) {
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

#if defined(MCL_HAS_DESKTOP_MOUSE)
namespace {

constexpr int16_t kMouseEncoderDragPixelsPerStep = 1;

bool encoder_mouse_slot_active(const LightPage *page, uint8_t i) {
  return page != NULL && i < GUI_NUM_ENCODERS && page->encoders[i] != NULL &&
         (page->encoder_key_control_mask & (1 << i));
}

bool mouse_encoder_fast(const mcl_mouse_event_t *event) {
  return event != NULL &&
         (event->modifiers & MCL_MOUSE_MODIFIER_SHIFT) != 0;
}

} // namespace

void LightPage::clearPageEncoderHits() {
  for (uint8_t i = 0; i < page_encoder_hit_count; i++) {
    page_encoder_hits[i].active = false;
  }
}

void LightPage::registerPageEncoderHit(uint8_t slot, int16_t x, int16_t y,
                                       int16_t w, int16_t h) {
  if (slot >= GUI_NUM_ENCODERS || w <= 0 || h <= 0) {
    return;
  }

  for (uint8_t i = 0; i < page_encoder_hit_count; i++) {
    PageEncoderHit &hit = page_encoder_hits[i];
    if (!hit.active) {
      hit.active = true;
      hit.slot = slot;
      hit.x = x;
      hit.y = y;
      hit.w = w;
      hit.h = h;
      return;
    }
  }
}

int8_t LightPage::pageEncoderHit(int16_t x, int16_t y) const {
  for (int8_t i = (int8_t)page_encoder_hit_count - 1; i >= 0; i--) {
    const PageEncoderHit &hit = page_encoder_hits[i];
    if (hit.active && x >= hit.x && y >= hit.y &&
        x < hit.x + hit.w && y < hit.y + hit.h) {
      return (int8_t)hit.slot;
    }
  }
  return -1;
}

void LightPage::resetMouseEncoderDrag() {
  mouse_encoder_focus = ENCODER_FOCUS_NONE;
  mouse_encoder_drag_origin_y = 0;
  mouse_encoder_drag_last_ticks = 0;
  mouse_encoder_press_buttons = 0;
  mouse_encoder_was_dragged = false;
}

void LightPage::applyMouseEncoderDelta(uint8_t i, int16_t ticks, bool fast) {
  if (!encoder_mouse_slot_active(this, i) || ticks == 0) {
    return;
  }

  Encoder *encoder = encoders[i];
  const int8_t step = ticks > 0 ? 1 : -1;
  while (ticks != 0) {
    encoder_t mouse_encoder = {};
    mouse_encoder.normal =
        step * (encoder->rot_res * ENCODER_RES_MULTIPLIER + 1);
    mouse_encoder.button = fast;
    encoder->update(&mouse_encoder);
    ticks -= step;
  }

  encoder_focus = i;
  GUI.wake_screen_saver();
  g_clock_minutes = 0;
  g_clock_ticks = 0;
  encoders_used_clock[i] = read_clock_ms();
}

void LightPage::queueMouseEncoderButtonClick(uint8_t i) {
  if (!encoder_mouse_slot_active(this, i)) {
    return;
  }

  GUI.queueVirtualButton(Buttons.ENCODER1 + i, true);
  GUI.queueVirtualButton(Buttons.ENCODER1 + i, false);
}

bool LightPage::handleEncoderMouseEvent(mcl_mouse_event_t *event) {
  if (event == NULL || !encoder_key_control_mask) {
    return false;
  }

  int8_t hit = pageEncoderHit(event->x, event->y);
  if (hit >= 0 && !encoder_mouse_slot_active(this, (uint8_t)hit)) {
    hit = -1;
  }

  switch (event->type) {
  case MCL_MOUSE_WHEEL:
    if (hit < 0 || event->deltaY == 0) {
      return false;
    }
    applyMouseEncoderDelta((uint8_t)hit, event->deltaY > 0 ? 1 : -1,
                           mouse_encoder_fast(event));
    return true;

  case MCL_MOUSE_DOWN:
  case MCL_MOUSE_DOUBLE_CLICK:
    if (hit < 0) {
      return false;
    }
    mouse_encoder_focus = (uint8_t)hit;
    mouse_encoder_drag_origin_y = event->y;
    mouse_encoder_drag_last_ticks = 0;
    mouse_encoder_press_buttons = event->buttons;
    mouse_encoder_was_dragged = false;
    encoder_focus = (uint8_t)hit;
    GUI.wake_screen_saver();
    g_clock_minutes = 0;
    g_clock_ticks = 0;
    encoders_used_clock[(uint8_t)hit] = read_clock_ms();
    return true;

  case MCL_MOUSE_DRAG:
    if (!encoder_mouse_slot_active(this, mouse_encoder_focus) ||
        (event->buttons & (MCL_MOUSE_BUTTON_LEFT | MCL_MOUSE_BUTTON_RIGHT |
                           MCL_MOUSE_BUTTON_MIDDLE)) == 0) {
      return false;
    }
    {
      int16_t total_ticks =
          (mouse_encoder_drag_origin_y - event->y) /
          kMouseEncoderDragPixelsPerStep;
      int16_t ticks = total_ticks - mouse_encoder_drag_last_ticks;
      if (ticks != 0) {
        mouse_encoder_was_dragged = true;
        applyMouseEncoderDelta(mouse_encoder_focus, ticks,
                               mouse_encoder_fast(event));
        mouse_encoder_drag_last_ticks = total_ticks;
      }
    }
    return true;

  case MCL_MOUSE_UP:
  case MCL_MOUSE_EXIT:
    if (mouse_encoder_focus == ENCODER_FOCUS_NONE) {
      return false;
    }
    if (event->type == MCL_MOUSE_UP && !mouse_encoder_was_dragged &&
        mouse_encoder_press_buttons != 0) {
      queueMouseEncoderButtonClick(mouse_encoder_focus);
    }
    resetMouseEncoderDrag();
    return true;

  default:
    return false;
  }
}
#endif
