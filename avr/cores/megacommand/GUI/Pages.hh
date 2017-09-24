/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef PAGES_H__
#define PAGES_H__

#include "Stack.h"
#include "Encoders.hh"
#include "ModalGui.hh"

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
class Page {
	/**
	 * \addtogroup gui_parent_page
	 * @{
	 **/
	
 public:
	/** The long name of a page (max 16 characters), used for example in ScrollSwitchPage. **/
  char name[17];
	/** The short name of a page (3 characters), used for example in SwitchPage. **/
  char shortName[4];
	/** Set to true will cause the next call to display() to redisplay the page on the display. **/
  bool redisplay;
	/** The parent container of the page, usually the Sketch which contains it. **/
  PageContainer *parent;
	/** Set to true when the setup() function has been called. **/
  bool isSetup;

	/**
	 * Create a page with the given long name and short name. setup()
	 * needs to be called for dynamic initialization steps.
	 **/
  Page(const char *_name = NULL, const char *_shortName = NULL) {
    parent = NULL;
    redisplay = false;
    setName(_name);
    setShortName(_shortName);
    isSetup = false;
  }

	/** Set the long name (max 16 chars) of the page. **/
  void setName(const char *_name = NULL) {
    if (_name != NULL) {
      m_strncpy(name, _name, 17);
    } else {
      name[0] = '\0';
    }
  }

	/** Set the short name (max 3 chars) of the page. **/
  void setShortName(const char *_shortName = NULL) {
    if (_shortName != NULL) {
      m_strncpy(shortName, _shortName, 4);
    } else {
      shortName[0] = '\0';
    }
  }

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
  virtual void update();
	/**
	 * The loop() method is basically the same as the update() method.
	 **/
  virtual void loop() { }
	/**
	 * This method is called by the GUI main loop on every iteration,
	 * and should redisplay the page.  Usually, this method checks to
	 * see if the redisplay flag is set to true. The redisplay flag is
	 * automatically cleared by the GUI main loop after calling the Page
	 * display() method.
	 **/
  virtual void display() { }
	/**
	 * This method is called by the GUI main loop on every iteration, at
	 * the end of the main loop.
	 **/
  virtual void finalize() { }

	/**
	 * This should clear specific settings of the page and clear the
	 * display if necessary.
	 **/
  virtual void clear()  { }
	/**
	 * This method is called by the Page container when the page becomes
	 * active (is pushed on top of the active page stack). It can be
	 * used to flash specific information for example.
	 **/
  virtual void show() { }
	/**
	 * This method is called by the Page container when the page is
	 * removed from view.
	 **/
  virtual void hide() { }

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
  virtual bool handleEvent(gui_event_t *event) {
    return false;
  }
	/**
	 * Dynamic initialization of the page (for example registering the
	 * page as a callback handler for MIDI events. This method should
	 * set the isSetup flag to true, and check if the flag was set to
	 * avoid double initialization.
	 **/
  virtual void setup() { }

#ifdef HOST_MIDIDUINO
  virtual ~Page() { }
#endif

	/* @} */
};

/* @} */

class Encoder;

/**
 * \addtogroup gui_encoder_page EncoderPage Class
 *
 * @{
 *
 * The EncoderPage is one of the most used subclasses of the Page
 * class. It groups 4 encoders and displays them.
 *
 * It is also the standard class used as a parent class when creating a custom Page.
 **/
class EncoderPage : public Page {
	/**
	 * \addtogroup gui_encoder_page
	 * @{
	 **/
	
 public:
  Encoder *encoders[GUI_NUM_ENCODERS];
	/**
	 * Create an EncoderPage with one or more encoders. The argument
	 * order determines which encoder will be used. Unused encoders can
	 * be passed as NULL pointers.
	 **/
  EncoderPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) {
    setEncoders(e1, e2, e3, e4);
  }

	/**
	 * Set the encoders used by the page later on.
	 **/
  void setEncoders(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) {
    encoders[0] = e1;
    encoders[1] = e2;
    encoders[2] = e3;
    encoders[3] = e4;
  }
	/** This method will update the encoders according to the hardware moves. **/
  virtual void update();
	/** This will clear the encoder movements. **/
  virtual void clear();
	/** Display the encoders using their short name and their value (as base 10). **/
  virtual void display();
	/** Executes the encoder actions by calling checkHandle() on each encoder. **/
  virtual void finalize();

	/**
	 * Used to display the names of the encoders on its own (useful if
	 * the encoders can update their name, for example when
	 * autolearning.
	 **/
  void displayNames();

	/* @} */
};

/* @} */

/**
 * \addtogroup gui_switch_page Switch Page Classes
 *
 * @{
 *
 * This class is used to switch between different page by pressing the
 * encoder underneath the displayed name. It uses the short name of the page
 * to display it.
 */
class SwitchPage : public Page {
	/**
	 * \addtogroup gui_switch_page
	 * @{
	 **/
	
public:
  Page *pages[4];

	/**
	 * Create a Switchpage allowing to switch between different
	 * pages. The position of the page is given by the argument
	 * position, unused pages can be passed as NULL pointer.
	 **/
  SwitchPage(const char *_name = "SELECT PAGE:",
	     Page *p1 = NULL, Page *p2 = NULL, Page *p3 = NULL, Page *p4 = NULL) :
    Page(_name) {
    initPages(p1, p2, p3, p4);
  }

	/**
	 * Initialize the pages to be switched later on.
	 **/
  void initPages(Page *p1 = NULL,  Page *p2 = NULL, Page *p3 = NULL, Page *p4 = NULL) {
    pages[0] = p1;
    pages[1] = p2;
    pages[2] = p3;
    pages[3] = p4;
  }

  virtual void display();
  virtual bool handleEvent(gui_event_t *event);

	/* @} */
};

/**
 * This class is used to switch between different page by pressing the
 * button next to the displayed name (not the encoders). It uses the short name of the page
 * to display it.
 */
class EncoderSwitchPage : public SwitchPage {
	/**
	 * \addtogroup gui_switch_page
	 * @{
	 **/
	
public:
  EncoderSwitchPage(Page *p1 = NULL, Page *p2 = NULL, Page *p3 = NULL, Page *p4 = NULL) :
    SwitchPage(NULL, p1, p2, p3, p4) {
  }

  virtual void display();
  virtual bool handleEvent(gui_event_t *event);

	/* @} */
};


/**
 * This class is used to switch between different pages by scrolling
 * through the pages using the first encoder.  The page displays the
 * long name of the selected page. This is useful if you want to
 * switch between different pages in a "menu" kind of way.
 **/
class ScrollSwitchPage : public EncoderPage {
	/**
	 * \addtogroup gui_switch_page
	 * @{
	 **/
	
public:
  Vector<Page *, 8> pages;
  RangeEncoder pageEncoder;

  ScrollSwitchPage() : pageEncoder(0, 0) {
    pageEncoder.pressmode = true;
    encoders[0] = &pageEncoder;
  }

  void addPage(Page *page);

  bool setSelectedPage();
  virtual void display();
  virtual void loop();
  virtual bool handleEvent(gui_event_t *event);

	/* @} */
};

/* @} */


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
  Stack<Page *, 8> pageStack;

	/**
	 * This needs to be overriden by the child class to describe how to
	 * handle events. Usually, the currentPage() is used to handle the
	 * event, but one could also walk through the page stack and stop at
	 * the first page that handles the required event.
	 **/
  virtual bool handleTopEvent(gui_event_t *event) {
    return false;
  }

	/** Clear the active page stack, and push page as the currentPage(). **/
  void setPage(Page *page) {
    pageStack.reset();
    pushPage(page);
  }

	/**
	 * Push the given page on top of the stack. This will automatically
	 * call the setup() method of the page if its isSetup flag is not
	 * set to true. It will then call the redisplayPage() method, and
	 * then the show() method of the page.
	 **/
  void pushPage(Page *page) {
		if (currentPage() == page) {
			// can't push the same page twice in a row
			return;
		}
		
    page->parent = this;
    if (!page->isSetup) {
      page->setup();
      page->isSetup = true;
    }
    page->redisplayPage();
    page->show();
    pageStack.push(page);
  }

	/** This will pop the page if it is the topmost page of the stack. **/
  void popPage(Page *page) {
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
    Page *page;
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
  virtual Page *currentPage() {
    Page *page = NULL;
    pageStack.peek(&page);
    return page;
  }

#ifdef HOST_MIDIDUINO
  virtual ~PageContainer() { }
#endif

	/* @} */
};

#endif /* PAGES_H__ */
