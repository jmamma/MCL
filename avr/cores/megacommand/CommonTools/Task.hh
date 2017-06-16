/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef TASK_H__
#define TASK_H__

#include <inttypes.h>

/**
 * \addtogroup CommonTools
 *
 * @{
 *
 * \file
 * Task class
 **/

/**
 * \addtogroup helpers_task Task class
 *
 * @{
 **/

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
  void checkTask() {
    uint16_t clock = read_slowclock();
    if (clock_diff(lastExecution, clock) > interval || starting) {
      run();
      lastExecution = clock;
      starting = false;
    }
  }

	/** Remove the task, calling its cleanup code (empty for now). **/
  virtual void destroy() {
  }

#ifdef HOST_MIDIDUINO
  virtual ~Task() {
  }
#endif

	/* @} */
};

/* @} @} */

#endif /* TASK_H__ */
