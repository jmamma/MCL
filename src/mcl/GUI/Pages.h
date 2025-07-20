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
  virtual void update() {}
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
  virtual void finalize() {}

  /**
   * This should clear specific settings of the page and clear the
   * display if necessary.
   **/
  virtual void clear() {}
  /**
   * This method is called by the Page container when the page becomes
   * active (is pushed on top of the active page stack). It can be
   * used to flash specific information for example.
   **/
  virtual void show() {}
  /**
   * This method is called by the Page container when the page is
   * removed from view.
   **/
  virtual void hide() {}

  /**
   * The basic event handler of the page, called by the event handling
   * part of the GUI main loop when the page is active. Should return
   * true if the event was handled by the page.
   **/
  virtual bool handleEvent(gui_event_t *event) { return false; }
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
  Encoder *encoders[GUI_NUM_ENCODERS];

  static uint16_t encoders_used_clock[4];

  LightPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL) {
    setEncoders(e1, e2, e3, e4);
  }

  void setEncoders(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                   Encoder *e4 = NULL) {
    encoders[0] = e1;
    encoders[1] = e2;
    encoders[2] = e3;
    encoders[3] = e4;
    isSetup = false;
  }
  /** This method will update the encoders according to the hardware moves. **/
  virtual void update();
  /** This will clear the encoder movements. **/
  virtual void clear();
  /** Executes the encoder actions by calling checkHandle() on each encoder. **/
  virtual void finalize();
  /** Call this to lock all encoders in the page. **/
  void lockEncoders() {} // TODO
  /** Call this to unlock all encoders in the page. If their value
      changed while locked, they will send out their new value.
  **/
  void unlockEncoders() {} // TODO

  void init_encoders_used_clock(uint16_t timeout = SHOW_VALUE_TIMEOUT);
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
