/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef GUI_H__
#define GUI_H__

#include <stdlib.h>

#include "Task.h"
#include "Vector.h"
#include "WProgram.h"

#if defined(MIDIDUINO_USE_GUI) || defined(HOST_MIDIDUINO)

#define MIDIDUINO_GUI_ACTIVE 1

#include "Encoders.h"
#include "Events.h"
#include "Pages.h"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \defgroup gui_lcd GUI LCD routines
 *
 * \defgroup gui_mainloop GUI Main Loop
 *
 * \defgroup gui_class GUI Class
 *
 * \file
 * GUI class
 *
 * @}
 *
 **/

/**
 * \addtogroup gui_lcd LCD
 *
 * @{
 **/

/** Structure used to hold displaying information (flash lines, flash timer,
 * flash duration, and display data). **/
typedef struct line_s {
  char data[16];
  bool changed;
  char flash[16];
  bool flashChanged;
  bool flashActive;
  uint16_t flashTimer;
  uint16_t duration;
} line_t;

/** Default duration of a flashed message (in timer ticks). **/
#define DEFAULT_FLASH_DURATION 600

/* @} */

class Page;
class Sketch;

/** The default sketch that is always available. **/
extern Sketch _defaultSketch;

typedef bool (*event_handler_t)(gui_event_t *event);

/**
 * \ingroup gui_class
 * The GUI class acting as a frontend to the display system. This
 * class is used to:
 *
 * - display data on the screen,
 * - add/remove tasks,
 * - add/remove event handlers
 * - handle the toplevel display loop (dispatching events, drawing pages,
 *handling encoders).
 *
 * Although some methods are declared virtual, there is usually no need to
 *subclass the GUI class.
 *
 **/
class GuiClass {
protected:
public:
  /**
   * \addtogroup gui_lcd
   *
   * @{
   */
  /** The two display lines of the MiniCommand display. **/
  line_t lines[2];
  /** The currently active line index. **/
  uint8_t curLine;

  /* @} */

  /**
   * \addtogroup gui_class
   *
   * @{
   */
  bool display_mirror = false;
  bool use_screen_saver = true;
  /** A vector storing the registered event handlers (max 4). **/
  Vector<event_handler_t, 4> eventHandlers;
  /** A vector storing the registered tasks (max 4). **/
  Vector<Task *, 4> tasks;

#ifdef GUI_NUM_ENCODERS
  /** The number of encoders present in the GUI (4 on the minicommand). **/
  static const uint8_t NUM_ENCODERS = GUI_NUM_ENCODERS;
#endif
  /** The number of buttons present in the GUI (4 on the minicommand). **/
  static const uint8_t NUM_BUTTONS = GUI_NUM_BUTTONS;

  GuiClass();
#ifdef HOST_MIDIDUINO
  virtual ~GuiClass() {}
#endif

  /** The currently active sketch (default NULL). **/
  Sketch *sketch;

  /**
   * Set the active sketch (its top page will be displayed).
   *
   * If a sketch is already active, its hide() method is called.
   * After the sketch is switched, the show() method of the new sketch
   * is called, and the currentPage() is redisplayed by calling
   * redisplayPage().
   **/
  void setSketch(Sketch *_sketch);
  /** Returns a pointer to the current sketches currentPage(). **/
  LightPage *currentPage();

  /**
   * Set the current page of the active sketch (all the page stack will be
   *cleared).
   *
   * Refer to the documentation of the Sketch class for more details.
   **/
  void setPage(LightPage *page);
  /**
   * Push a new page on top of the currently active one.
   *
   * Refer to the documentation of the Sketch class for more details.
   **/
  void pushPage(LightPage *page);
  /**
   * Pop the top page.
   *
   * Refer to the documentation of the Sketch class for more details.
   **/
  void popPage();
  /**
   * Pop the top page if it is the same as page.
   *
   * Refer to the documentation of the Sketch class for more details.
   **/
  void popPage(LightPage *page);

  /**
   * Add a new event handler to the event handler vector (max 4). The
   * event handler will then be called on each update loop.
   **/
  void addEventHandler(event_handler_t handler) { eventHandlers.add(handler); }
  /**
   * Remove an event handler.
   **/
  void removeEventHandler(event_handler_t handler) {
    eventHandlers.remove(handler);
  }
  void ignoreNextEvent(uint8_t i) {
  SET_BIT(event_ignore_next_mask, i);
  }
  /**
   * Add a new task to be periodically polled (max 8).
   **/
  void addTask(Task *task) { tasks.add(task); }
  /**
   * Remove a task from the task list.
   **/
  void removeTask(Task *task) { tasks.remove(task); }

  /* @} */

  /**
   * \addtogroup gui_mainloop
   *
   * @{
   */

  /**
   * This is the main toplevel loop. It goes through a few steps to
   * make the whole framework run smoothly.
   *
   * The first step of loop() is to execute the tasks, by calling the
   * checkTask() method of each task.
   *
   * The second step is going through the events stored in
   * EventRB. For each event, it goes through the list of registered
   * event handlers, and executes them. If one of these event handlers
   * returns true, it goes to the next event without trying any
   * further event handlers. After the event handlers were executed,
   * and none returned true, it asks the sketch to handle the event by
   * calling its handleTopEvent method.
   *
   * In the third step, it then calls the update() and loop() methods
   * of the current page of the active sketch (which is _defaultSketch
   * by default).
   *
   * The fourth step is calling the loop() method of the sketch itself.
   *
   * The fifth step is then calling the ::loop() method, which is
   * empty by default, and can be overriden by the user. This is
   * mostly to keep a compatibility to older mididuino sketches and to
   * mirror the structure of arduino sketches, but usually is not used
   * as the main loops are kept inside Sketch classes.
   *
   * The sixth step is then calling the display() method of the GUI class (see
   *below).
   *
   * In a final step, it calls the finalize() method of the current
   * page of the active sketch, if it is available. This is per
   * default an empty method and can be overriden by the programmer.
   **/
  virtual void loop();

  /**
   * This method is called regularly in the main loop and handles the
   * refreshing of the MiniCommand display.
   *
   * It first calls the display() method of the current page of the active
   *sketch.
   *
   * It then handles the displaying of "flash" messages, by checking
   * how long the active flash message has been running.
   **/

  /** update hd44780 **/
  void display_lcd();

  void display();

  /**
   * This method sets the redisplay flag of the active page to true so
   * that it gets redisplayed on the next call to display().
   **/
  void redisplay();

  /* @} */

  /**
   * \addtogroup gui_lcd
   *
   * @{
   */

  void put_value_at1(uint8_t idx, uint8_t value);
  void put_value_at1(uint8_t idx, uint8_t value, char *data);
  
  void put_value_at2(uint8_t idx, uint8_t value);
  void put_value_at2(uint8_t idx, uint8_t value, char *data);
  /** Display the value in base 10 at the given idx (in 4 character increments).
   * **/
  void put_value(uint8_t idx, uint8_t value);
  void put_value(uint8_t idx, uint8_t value, char *data);
  /** Display the value in base 10 at the given idx (in 4 character increments).
   * **/
  void put_value(uint8_t idx, int value);
  void put_value(uint8_t idx, int value, char *data);
  /** Display the value in base 16 at the given idx (in 4 character increments).
   * **/
  void put_value16(uint8_t idx, uint16_t value);
  void put_value16(uint8_t idx, uint16_t value, char *data);
  /** Display the value in base 16 at the given idx (in 4 character increments).
   * **/
  void put_valuex(uint8_t idx, uint8_t value);
  void put_valuex(uint8_t idx, uint8_t value, char *data);
  /** Display the value in base 10 at the given idx (in 1 character increments).
   * **/
  void put_value_at(uint8_t idx, uint8_t value);
  void put_value_at(uint8_t idx, uint8_t value, char *data);
  /** Display the value in base 10 at the given idx (in 1 character increments).
   * **/
  void put_value_at(uint8_t idx, int value);
  void put_value_at(uint8_t idx, int value, char *data);
  /** Display the value in base 16 at the given idx (in 1 character increments).
   * **/
  void put_value16_at(uint8_t idx, uint16_t value);
  void put_value16_at(uint8_t idx, uint16_t value,  char *data);
  /** Display the value in base 16 at the given idx (in 1 character increments).
   * **/
  void put_valuex_at(uint8_t idx, uint8_t value);
  void put_valuex_at(uint8_t idx, uint8_t value, char *data);

  /** Display the string at the given idx (in 4 character increments). **/
  void put_string(uint8_t idx, const char *str);
  /** Display the program space string at the given idx (in 4 character
   * increments). **/
  void put_p_string(uint8_t idx, PGM_P str);
  /** Display the string. **/
  void put_string(const char *str);
  /** Display the program-space string. **/
  void put_p_string(PGM_P str);
  /** Display the string at the given idx (in 4 character increments) and fill
   * up with whitespace. **/
  void put_string_fill(uint8_t idx, const char *str);
  /** Display the string and fill up with whitespace. **/
  void put_string_fill(const char *str);
  /** Display the program-space string at the given idx (in 4 character
   * increments) and fill up with whitespace. **/
  void put_p_string_fill(uint8_t idx, PGM_P str);
  /** Display the program-space string and fill up with whitespace. **/
  void put_p_string_fill(PGM_P str);

  /** Display string of fixed length **/
  void put_string_at_len(uint8_t idx, const char *str, uint8_t len);

  /** Display the string at the given idx (in 1 character increments). **/
  void put_string_at(uint8_t idx, const char *str);
  /** Display the program-space string at the given idx (in 1 character
   * increments). **/
  void put_p_string_at(uint8_t idx, PGM_P str);

  void put_string_at_not(uint8_t idx, const char *str);
  void put_string_at_noterminator(uint8_t idx, const char *str);
  /** Display the string at the given idx (in 1 character increments) and fill
   * up with whitespace. **/
  void put_string_at_fill(uint8_t idx, const char *str);
  /** Display the program-space string at the given idx (in 1 character
   * increments) and fill up with whitespace. **/
  void put_p_string_at_fill(uint8_t idx, PGM_P str);

  /** Print the format string using the vsnprintf function. **/
  void printf(const char *fmt, ...);
  /** Print the format string using the vsnprintf function and fill up with
   * whitespace. **/
  void printf_fill(const char *fmt, ...);
  /** Print the format string at the given idx (in 1 character increments) using
   * the vsnprintf function. **/
  void printf_at(uint8_t idx, const char *fmt, ...);
  /**
   * Print the format string at the given idx (in 1 character
   * increments) using the vsnprintf function and fill up with
   * whitespace.
   **/
  void printf_at_fill(uint8_t idx, const char *fmt, ...);

  /** Set the flash duration of the currently active flash line. **/
  void flash(uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the value in base 10 at the given idx (in 4 character increments)
   * for the given duration. **/
  void flash_put_value(uint8_t idx, uint8_t value,
                       uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the value in base 16 at the given idx (in 4 character increments)
   * for the given duration. **/
  void flash_put_value16(uint8_t idx, uint16_t value,
                         uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the value in base 16 at the given idx (in 4 character increments)
   * for the given duration. **/
  void flash_put_valuex(uint8_t idx, uint8_t value,
                        uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the value in base 10 at the given idx (in 1 character increments)
   * for the given duration. **/
  void flash_put_value_at(uint8_t idx, uint8_t value,
                          uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the value in base 16 at the given idx (in 1 character increments)
   * for the given duration. **/
  void flash_put_value16_at(uint8_t idx, uint16_t value,
                            uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the value in base 16 at the given idx (in 4 character increments)
   * for the given duration. **/
  void flash_put_valuex_at(uint8_t idx, uint8_t value,
                           uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the string for the given duration. **/
  void flash_string(const char *str,
                    uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the program-space string for the given duration. **/
  void flash_p_string(PGM_P str, uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the string for the given duration and fill with whitespace. **/
  void flash_string_fill(const char *str,
                         uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the program-space string for the given duration and fill with
   * whitespace. **/
  void flash_p_string_fill(PGM_P str,
                           uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the string at the given idx (in 1 character increments) for the
   * given duration. **/
  void flash_string_at(uint8_t idx, const char *str,
                       uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Flash the program-space string at the given idx (in 1 character
   * increments) for the given duration. **/
  void flash_p_string_at(uint8_t idx, PGM_P str,
                         uint16_t duration = DEFAULT_FLASH_DURATION);
  /**
   * Flash the string at the given idx (in 1 character increments) for
   * the given duration and fill with whitespace.
   **/
  void flash_string_at_fill(uint8_t idx, const char *str,
                            uint16_t duration = DEFAULT_FLASH_DURATION);
  /**
   * Flash the program-space string at the given idx (in 1 character
   * increments) for the given duration and fill with whitespace.
   **/
  void flash_p_string_at_fill(uint8_t idx, PGM_P str,
                              uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Clear the flash line and display the new string for the given duration.
   * **/
  void flash_string_clear(const char *str,
                          uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Clear the flash line and display the new program-space string for the
   * given duration. **/
  void flash_p_string_clear(const char *str,
                            uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Display both strings and fill with whitespace for the given duration. **/
  void flash_strings_fill(const char *str1, const char *str2,
                          uint16_t duration = DEFAULT_FLASH_DURATION);
  /** Display both program-space strings and fill with whitespace for the given
   * duration. **/
  void flash_p_strings_fill(PGM_P str1, PGM_P str2,
                            uint16_t duration = DEFAULT_FLASH_DURATION);

  /** Flash the given format string using the vsnprintf function. **/
  void flash_printf(const char *fmt, ...);
  /** Flash the given format string using the vsnprintf function and fill with
   * whitespace. **/
  void flash_printf_fill(const char *fmt, ...);
  /**
   * Flash the given format string at the given idx (in 1-character
   * increments) using the vsnprintf function and fill with
   * whitespace.
   **/
  void flash_printf_at(uint8_t idx, const char *fmt, ...);
  /**
   * Flash the given format string using the vsnprintf function and
   * fill with whitespace.
   **/
  void flash_printf_at_fill(uint8_t idx, const char *fmt, ...);

  /** Switch the line to be displayed, using either GUI.LINE1 or GUI.LINE2. **/
  void setLine(const uint8_t line) { curLine = line; }
  /** Clear both lines **/
  void clearLines();
  /** Clear the currently active line. **/
  void clearLine();
  /** Clear the currently active flash line. **/
  void clearFlashLine();
  /** Clear the currently active flash line and reset the flash duration. **/
  void clearFlash(uint16_t duration = DEFAULT_FLASH_DURATION);

  static const uint8_t LINE1 = 0;
  static const uint8_t LINE2 = 1;

  /* @} */
};

/**
 * \addtogroup gui_class
 *
 * @{
 */

/** The single instance of the GUI class. **/
extern GuiClass GUI;

/* @} */

char hex2c(uint8_t hex);

#include "Encoders.h"
#include "Pages.h"
#include "Sketch.h"

#endif

#endif /* GUI_H__ */
