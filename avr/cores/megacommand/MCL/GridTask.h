#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.h"
#include "Elektron.h"

class GridTask : public Task {

public:
  bool stop_hard_callback = false;

  char kit_names[NUM_DEVS][16];

  uint8_t last_active_row;
  uint8_t next_active_row;
  bool chain_behaviour;
  uint8_t load_row = 255;
  GridTask(uint16_t interval) : Task(interval) { setup(interval); }

  void setup(uint16_t interval = 0);

  virtual void run();
  virtual void destroy();

  void init() { load_row = 255; }
  void gui_update();
  void transition_handler();

  bool link_load(uint8_t n, uint8_t track_idx, uint8_t *slots_changed, uint8_t *track_select_array, GridDeviceTrack *gdt);
  bool transition_load(uint8_t n, uint8_t track_idx, uint8_t dev_idx, GridDeviceTrack *gdt);

  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
