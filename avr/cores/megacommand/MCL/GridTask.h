#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.h"

#define CHAIN_AUTO 1
#define CHAIN_MANUAL 2
#define CHAIN_QUEUE 3

class GridTask : public Task {

public:

  GridTask(uint16_t interval) : Task(interval) { setup(interval); }

  void setup(uint16_t interval = 0);

  virtual void run();

  virtual void destroy();
  void init();
  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
