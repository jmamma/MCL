#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.h"

class GridTask : public Task {

public:

  GridTask(uint16_t interval) : Task(interval) { setup(interval); }

  void setup(uint16_t interval = 0);

  virtual void run();

  virtual void destroy();
  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
