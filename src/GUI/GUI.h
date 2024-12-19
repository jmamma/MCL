// GUI.h
#pragma once

#define SHOW_VALUE_TIMEOUT 2000
#define SCREEN_SAVER_TIMEOUT 5

#include <stdlib.h>
#include "Task.h"
#include "Vector.h"
#include "WProgram.h"
#include "Stack.h"

#include "Encoders.h"
#include "Events.h"
#include "Pages.h"

class Page;
typedef bool (*event_handler_t)(gui_event_t *event);

class GuiClass {
protected:
public:
  EventManager events;
  bool display_mirror = false;
  bool use_screen_saver = true;
  bool screen_saver = false;
  Vector<event_handler_t, 4> eventHandlers;
  Vector<Task *, 4> tasks;
  Stack<LightPage *, 8> pageStack;

#ifdef GUI_NUM_ENCODERS
  static const uint8_t NUM_ENCODERS = GUI_NUM_ENCODERS;
#endif
  static const uint8_t NUM_BUTTONS = GUI_NUM_BUTTONS;

  GuiClass();

  // Page management methods
  LightPage *currentPage();
  void setPage(LightPage *page);
  void pushPage(LightPage *page);
  void popPage();
  void popPage(LightPage *page);
  bool handleTopEvent(gui_event_t *event);

  // Event and task management
  void putEvent(gui_event_t* event) {
    events.putEvent(event);
  }

  void addEventHandler(event_handler_t handler) { eventHandlers.add(handler); }
  void removeEventHandler(event_handler_t handler) { eventHandlers.remove(handler); }
  void ignoreNextEvent(uint8_t i) { events.setIgnoreMask(i); }
  void addTask(Task *task) { tasks.add(task); }
  void removeTask(Task *task) { tasks.remove(task); }

  virtual void loop();
  void mirror();
  void display();

  // Methods previously in Sketch that might be needed
  virtual void show() {}
  virtual void hide() {}
  virtual void setup() {}
};
