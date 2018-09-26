#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.hh"

class GridTask : public Task {

public:
  GridChain chains[20];
  GridTask(uint16_t interval) : Task(interval) { setup(interval); }

  void setup(uint16_t interval = 0);

  virtual void run();

  virtual void destroy();
  void init();
  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
