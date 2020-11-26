/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef PAGES_H__
#define PAGES_H__

#include "Encoders.h"
#include "Stack.h"

class Sketch;
class PageContainer;

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

/**
 * \addtogroup gui_parent_page Parent Page Class
 *
 * @{
 *
 * This is the parent Page class that is used through the framework.
 *
 * A page represent a single "screen" on the midi controller. All the
 * code to display information and handle encoder movements and button
 * presses are encapsulated into a Page.
 *
 * The parent Page class just defines the protocol used by the GUI
 * framework to display and handle pages. Some of this information can
 * be found in the description of the \ref gui_mainloop "GUI main loop".
 *
 * The programmer needs to create his own child class of the Page
 * class, and override methods like update(), loop, display(), show(),
 * hide() and handleEvent().
 *
 * Multiple pages related to one functionality can be grouped into a
 * structure called Sketch, which can also handle a few of the button
 * presses, so that for example switching from one page to another is
 * encapsulated.
 *
 * A collection of useful Page subclasses is provided by the
 * framework. For example, EncoderPage which groups 4 encoders, but
 * also more complicated pages like SwitchPage, EncoderSwitchPage or
 * ScrollSwitchPage to switch between different other pages.
 *
 * Also, a lot of the functionality provided by other parts of the
 * framework are encapsulated into their own pages which can just be
 * added to an existing sketch, for example the MidiClockPage used to
 * configure/store/restore midi clock and midi merge setting.
 **/

#define DISPLAY_PIXEL_MODE 0
#define DISPLAY_TEXT_MODE0 1

class PageParent {
  /**
   * \addtogroup gui_parent_page
   * @{
   **/

public:
  /** Set to true will cause the next call to display() to redisplay the page on
   * the display. **/
  bool redisplay;
  /** Set to true when the setup() function has been called. **/
  bool isSetup;

  bool classic_display = true;

  uint8_t displaymode = DISPLAY_TEXT_MODE0;

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
   * This method clears the display and sets the redisplay flag of the
   * page to true.
   **/
  void redisplayPage();
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
  uint8_t curpage;
  PageContainer *parent;
  Encoder *encoders[GUI_NUM_ENCODERS];
  static uint16_t encoders_used_clock[GUI_NUM_ENCODERS];

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
};

class Page : public PageParent {
public:
  /** The parent container of the page, usually the Sketch which contains it.
   * **/
  PageContainer *parent;
  bool redisplay;

  Page(const char *_name = NULL, const char *_shortName = NULL) : PageParent() {
    parent = NULL;
    redisplay = false;
    isSetup = false;
  }

  void update();
};

/* @} */

class Encoder;

/**
 * \addtogroup gui_page_container GUI Page Container
 *
 * @{
 *
 * This is the parent class used for classes that can contain other
 * pages. The Sketch class is an example of a page container.
 */
class PageContainer {
  /**
   * \addtogroup gui_page_container
   * @{
   **/

public:
  /** Stores the active pages in a stack (max 8 pages). **/
  Stack<LightPage *, 8> pageStack;

  /**
   * This needs to be overriden by the child class to describe how to
   * handle events. Usually, the currentPage() is used to handle the
   * event, but one could also walk through the page stack and stop at
   * the first page that handles the required event.
   **/
  virtual bool handleTopEvent(gui_event_t *event) { return false; }

  /** Clear the active page stack, and push page as the currentPage(). **/
  void setPage(LightPage *page) {
    if (currentPage() != NULL) {
      DEBUG_PRINTLN(F("calling cleanup"));
      currentPage()->cleanup();
    }

    else {
      DEBUG_PRINTLN(F("Current Page is NULL"));
    }

    pageStack.reset();
    pushPage(page);
  }

  /**
   * Push the given page on top of the stack. This will automatically
   * call the setup() method of the page if its isSetup flag is not
   * set to true. It will then call the redisplayPage() method, and
   * then the show() method of the page.
   **/
  void pushPage(LightPage *page);

  /** This will pop the page if it is the topmost page of the stack. **/
  void popPage(LightPage *page) {
    if (currentPage() == page) {
      popPage();
    }
  }

  /**
   * Pop the topmost page of the stack. It will then call the hide()
   * method of the page. Finally, it will call the redisplayPage()
   * method of the new topmost page.
   **/
  void popPage() {
    currentPage()->cleanup();
    LightPage *page;
    pageStack.pop(&page);
    if (page != NULL) {
      page->parent = NULL;
      page->hide();
    }

    page = currentPage();
    if (page != NULL) {
      page->redisplayPage();
    }
  }

  /** Returns the topmost page of the stack. **/
  virtual LightPage *currentPage() {
    LightPage *page = NULL;
    pageStack.peek(&page);
    return page;
  }

#ifdef HOST_MIDIDUINO
  virtual ~PageContainer() {}
#endif

  /* @} */
};

#endif /* PAGES_H__ */
