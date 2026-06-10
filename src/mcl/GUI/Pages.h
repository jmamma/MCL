/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include "Events.h"

class PageParent {
  /**
   * \addtogroup gui_parent_page
   * @{
   **/

public:
  /** Set to true when the setup() function has been called. **/
  bool isSetup;

  PageParent() {}

  /**
   * The update() method is called by the GUI main loop on every
   * iteration. Code to handle special encoder changes has to be put
   * here, using
   *
   * \code
   * if (encoder->hasChanged() {
   *  // some action
   * }
   * \endcode
   **/
  void update() {}
  /**
   * The loop() method is basically the same as the update() method.
   **/
  virtual void loop() {}
  /**
   * This method is called by the GUI main loop on every iteration,
   * and should redisplay the page.  Usually, this method checks to
   * see if the redisplay flag is set to true. The redisplay flag is
   * automatically cleared by the GUI main loop after calling the Page
   * display() method.
   **/
  virtual void display() {}
  /**
   * This method is called by the GUI main loop on every iteration, at
   * the end of the main loop.
   **/
  void finalize() {}

  /**
   * This should clear specific settings of the page and clear the
   * display if necessary.
   **/
  void clear() {}
  /**
   * This method is called by the Page container when the page becomes
   * active (is pushed on top of the active page stack). It can be
   * used to flash specific information for example.
   **/
  void show() {}
  /**
   * This method is called by the Page container when the page is
   * removed from view.
   **/
  void hide() {}

  /**
   * The basic event handler of the page, called by the event handling
   * part of the GUI main loop when the page is active. Should return
   * true if the event was handled by the page.
   **/
  virtual bool handleEvent(gui_event_t *event) { return false; }
#if defined(MCL_HAS_DESKTOP_MOUSE)
  virtual bool handleMouseEvent(mcl_mouse_event_t *event) {
    (void)event;
    return false;
  }
#endif
  /**
   * Dynamic initialization of the page (for example registering the
   * page as a callback handler for MIDI events. This method should
   * set the isSetup flag to true, and check if the flag was set to
   * avoid double initialization.
   **/

  // Call an init routine each time the page is loaded
  virtual void init() {}
  virtual void config() {}
  virtual void cleanup() {}
  virtual void setup() {}

#ifdef HOST_MIDIDUINO
  virtual ~Page() {}
#endif

  /* @} */
};

class LightPage : public PageParent {

public:
  enum { ENCODER_FOCUS_NONE = 255 };

  Encoder *encoders[GUI_NUM_ENCODERS];

  static uint16_t encoders_used_clock[4];
  uint8_t encoder_focus;
  uint8_t encoder_key_control_mask;

  LightPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL) NOINLINE();

  void setEncoders(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                   Encoder *e4 = NULL) {
    encoders[0] = e1;
    encoders[1] = e2;
    encoders[2] = e3;
    encoders[3] = e4;
    isSetup = false;
  }
  /** This method will update the encoders according to the hardware moves. **/
  void update();
  /** This will clear the encoder movements. **/
  void clear();
  /** Executes the encoder actions by calling checkHandle() on each encoder. **/
  void finalize();
  bool handleEncoderKeyControls(gui_event_t *event);
#if defined(MCL_HAS_DESKTOP_MOUSE)
  bool handleEncoderMouseEvent(mcl_mouse_event_t *event);
#endif
  bool selectEncoderFocus(int8_t start, int8_t step);
  virtual bool moveEncoderFocusPage(int8_t direction) { return false; }
  void enableEncoderKeyControls(uint8_t mask = 0x0F) {
    encoder_key_control_mask = mask;
  }
  void disableEncoderKeyControls() {
    encoder_key_control_mask = 0;
    resetEncoderFocus();
  }
  void resetEncoderFocus() { encoder_focus = ENCODER_FOCUS_NONE; }
  bool isEncoderFocused(uint8_t i) const {
    return encoder_focus == i &&
           clock_diff(encoders_used_clock[i], read_clock_ms()) <
               SHOW_VALUE_TIMEOUT;
  }
  /** Call this to lock all encoders in the page. **/
  void lockEncoders() {} // TODO
  /** Call this to unlock all encoders in the page. If their value
      changed while locked, they will send out their new value.
  **/
  void unlockEncoders() {} // TODO

  void init_encoders_used_clock(uint16_t timeout = SHOW_VALUE_TIMEOUT);

#if defined(MCL_HAS_DESKTOP_MOUSE)
  void clearPageEncoderHits();
  void registerPageEncoderHit(uint8_t slot, int16_t x, int16_t y,
                              int16_t w, int16_t h);
  int8_t pageEncoderHit(int16_t x, int16_t y) const;

  uint8_t mouse_encoder_focus;
  int16_t mouse_encoder_drag_origin_y;
  int16_t mouse_encoder_drag_last_ticks;
  uint8_t mouse_encoder_press_buttons;
  bool mouse_encoder_was_dragged;

  struct PageEncoderHit {
    bool active;
    uint8_t slot;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
  };

  static constexpr uint8_t page_encoder_hit_count = 12;
  PageEncoderHit page_encoder_hits[page_encoder_hit_count];

  void resetMouseEncoderDrag();
  void applyMouseEncoderDelta(uint8_t i, int16_t ticks, bool fast);
  void queueMouseEncoderButtonClick(uint8_t i);
#endif
};

class Page : public PageParent {
public:
  /** The parent container of the page, usually the Sketch which contains it.
   * **/

  Page(const char *_name = NULL, const char *_shortName = NULL) : PageParent() {
    isSetup = false;
  }

  void update();
};
