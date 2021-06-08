#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.h"

class GridTask : public Task {

public:
  bool stop_hard_callback = true;

  GridTask(uint16_t interval) : Task(interval) { setup(interval); }


  void setup(uint16_t interval = 0);

  virtual void run();

  virtual void destroy();
  void init();
  void transition_handler();
  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
