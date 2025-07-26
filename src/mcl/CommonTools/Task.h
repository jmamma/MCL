/* Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include <inttypes.h>
#include <helpers.h>

/** Represents a task that is executed at a regular interval. **/
class Task {
	/**
	 * \addtogroup helpers_task
	 * @{
	 **/

public:
  uint16_t interval;
  uint16_t lastExecution;
  bool starting;

  void (*taskFunction)();

	/** Create a task to be executed approximately every _interval ticks, calling the function _taskFunction. **/
  Task(uint16_t _interval, void (*_taskFunction)() = NULL) {
    interval = _interval;
    lastExecution = 0;
    taskFunction = _taskFunction;
    starting = true;
  }

	/** Execute the task by calling the taskFunction. **/
  virtual void run() {
    if (taskFunction != NULL) {
      taskFunction();
    }
  }

	/** Check if the task needs to be executed. **/
  void checkTask();

  /** Remove the task, calling its cleanup code (empty for now). **/
  virtual void destroy() {
  }

#ifdef HOST_MIDIDUINO
  virtual ~Task() {
  }
#endif

	/* @} */
};

