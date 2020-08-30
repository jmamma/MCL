/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef SKETCH_H__
#define SKETCH_H__

#include "Pages.h"
#include "Stack.h"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_sketch Sketch class
 *
 * @{
 *
 * \defgroup gui_sketch_switchpage SketchSwitchPage Class
 *
 * \file
 * Sketch class
 **/

/**
 * The standard Sketch class, which is a PageContainer. This is used
 * as the default parent class for individual "firmwares". Basically a
 * Sketch is a standalone piece of functionality, using different
 * Pages for controlling aspects of that functionality. A Sketch can
 * be an arpeggiator, a standard MIDI controller with 4 different
 * pages sending out CC messages, etc...
 *
 * The Mididuino framework provides a huge number of ready to go
 * Sketches that can be further modified or just use as is.
 *
 * Different Sketch classes can be merged together in a big Monster
 * firmware. This enables further configuration of the sketch itself
 * by presenting a "high-level" view. It allows the user to "mute" the
 * sketch, trigger an "extra" function, and display different pages of
 * the sketch. This Monster functionality is used by the
 * SwitchSketchPage described further below.
 **/
class Sketch : public PageContainer {
	/**
	 * \addtogroup gui_sketch
	 * @{
	 **/
	
public:

	/** This is the name of the sketch. **/
  char *name;
	/**
	 * Used in a monster configuration to mute the sketch. The actual
	 * functionality is handled by the child class. (default false)
	 **/
  bool muted;
	/**
	 * Set to true if the sketch is used in a monster firmware. (default false)
	 **/
  bool monster;

	/** Create a new sketch with the given name. **/
  Sketch(char *_name = NULL) {
    name = _name;
    muted = false;
    monster = false;
  }

	/**
	 * Initialize the runtime of the sketch, and set the monster
	 * status. If the sketch is used in a monster firmware, it is
	 * automatically muted at first.
	 **/
  virtual void setupMonster(bool _monster) {
    monster = _monster;
    if (monster) {
      muted = true;
    }
    setup();
  }

	/** Returns true if the sketch is the currently active sketch in the GUI. **/
  bool isVisible() {
    return GUI.sketch == this;
  }

	/**
	 * Do the runtime initialization of the sketch, needs to be
	 * overloaded by the child class.
	 **/
  virtual void setup() {
  }

	/**
	 * Event handler for the overall sketch. Sketch-wide functionality
	 * and page switching needs to be implemented in this method.
	 **/
  virtual bool handleEvent(gui_event_t *event) {
    return false;
  }

	/**
	 * This method is called by the GUI main loop to handle a button
	 * event. The default implementation first defers the handling to
	 * the currently active page, and if that page doesn't handle the
	 * event, the sketch-wide handleEvent() method is called.
	 **/
  bool handleTopEvent(gui_event_t *event) {
    LightPage *curPage = currentPage();
    if (curPage != NULL) {
      if (curPage->handleEvent(event)) {
				return true;
      }
    }
    return handleEvent(event);
  }

	/**
	 * The loop() method of the sketch is called on every iteration of
	 * the GUI main loop.
	 *
	 * This method could check for changes in encoders of specific pages
	 * (although that code may be better kept in the page code itself).
	 **/
  virtual void loop() {
  }

	/**
	 * not used at the moment.
	 **/
  virtual void destroy() {
  }

	/** This method is called by the GUI class when the sketch is displayed. **/
  virtual void show() {
  }

	/** This method is called when the sketch is removed from view. **/
  virtual void hide() {
  }

	/**
	 * This method is called by the SketchSwitchPage to execute a special function of the sketch.
	 * pressed signals if the extra button is pressed (true) or released (false).
	 *
	 * For example, the MDWesenLivePatch triggers the reverse sampling when doExtra() is called.
	 **/
  virtual void doExtra(bool pressed) {
  }

	/**
	 * This method is called by the SketchSwitchPage when the mute functionality is triggered.
	 * pressed signals if the mute button is pressed (true), or released (false).
	 *
	 * The default implementation toggle the mute variable when the button is pressed.
	 **/
  virtual void mute(bool pressed) {
    if (pressed) {
      muted = !muted;
    }
  }

	/** Returns the special page with index i (usually i is either 0 or 1 in the SketchSwitchPage). **/
  virtual Page *getPage(uint8_t i) {
    return NULL;
  }

	/** Returns a two line name (3 characters on each line) to be used by the SketchSwitchPage. **/
  virtual void getName(char *n1, char *n2) {
    n1[0] = '\0';
    n2[0] = '\0';
  }

	/* @} */
};

/**
 * This is the standard sketch that is always instantiated and
 * displayed by default.
 **/
extern Sketch _defaultSketch;

#endif /* SKETCH_H__ */
