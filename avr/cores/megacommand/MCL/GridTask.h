#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "mcl.h"
#include "Task.h"
#include "Elektron.h"
#include "GridChain.h"

class LoadQueueModes {
  public:
  uint8_t mode;
  uint8_t offset;
};

class LoadQueue {
  public:
  uint8_t row_selects[NUM_LINKS][NUM_SLOTS];
  LoadQueueModes modes[NUM_LINKS];
  uint8_t rd;
  uint8_t wr;
  bool full;

  void init() {
    rd = 0;
    wr = 0;
    bool full = false;
  }

  void put(uint8_t mode, uint8_t *row_select, uint8_t offset = 255) {
    if (full) { return; }
    memcpy(row_selects[wr],row_select,NUM_SLOTS);
    modes[wr].mode = mode;
    modes[wr++].offset = offset;
    if (wr == NUM_LINKS) {
       wr = 0;
    }
    if (wr == rd) {
        full = true;
    }
  }


  void put(uint8_t mode, uint8_t row, uint8_t *track_select_array, uint8_t offset = 255) {
    if (full) { return; }

    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
       row_selects[wr][n] = 255;
       if (track_select_array[n]) { row_selects[wr][n] = row; }
    }
    modes[wr].mode = mode;
    modes[wr++].offset = offset;
    if (wr == NUM_LINKS) {
       wr = 0;
    }
    if (wr == rd) {
      full = true;
    }
  }

  void get(uint8_t &mode, uint8_t &offset, uint8_t *row_select) {
    memcpy(row_select,row_selects[rd],NUM_SLOTS);
    mode = modes[rd].mode;
    offset = modes[rd++].offset;
    if (rd == NUM_LINKS) {
       rd = 0;
    }
    full = false;
  }

  bool is_empty() {
    return !full && (rd == wr);
  }

};

class GridTask : public Task {

public:
  bool stop_hard_callback = false;

  char kit_names[NUM_DEVS][16];

  uint8_t last_active_row;
  uint8_t next_active_row;
  bool chain_behaviour;


  uint8_t load_row_midi = 255;
  uint8_t load_row = 255;
  uint8_t midi_row = 255;

  uint8_t load_track_select[NUM_SLOTS];
  uint8_t midi_row_select = 255;

  bool midi_load;

  LoadQueue load_queue;

  GridTask(uint16_t interval) : Task(interval) { setup(interval); }

  void setup(uint16_t interval = 0);

  virtual void run();
  virtual void destroy();
  void init() {
     reset_midi_states();
     load_queue.init();
  }

  void reset_midi_states() {
    midi_row = 255;
    load_row = 255;
    memset(load_track_select, 255, sizeof(load_track_select));
    //midi_row_select = 255;
    midi_load = false;
  }
  void row_update();
  void gui_update();
  void update_transition_details();
  void load_queue_handler();
  void transition_handler();

  bool link_load(uint8_t n, uint8_t track_idx, uint8_t *slots_changed, uint8_t *track_select_array, GridDeviceTrack *gdt);
  bool transition_load(uint8_t n, uint8_t track_idx, GridDeviceTrack *gdt);
  bool transition_send(uint8_t n, uint8_t track_idx, uint8_t dev_idx, GridDeviceTrack *gdt);

  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
