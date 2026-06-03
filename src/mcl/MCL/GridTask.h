#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.h"
#include "Elektron.h"
#include "GridChain.h"

class LoadQueueModes {
  public:
  uint8_t mode;
  GridSlot offset;
};

#if !defined(__AVR__)
#define LOAD_QUEUE_CLEAR_ROW 254
#define LOAD_QUEUE_PRIVATE_ROW 253
#define LOAD_QUEUE_FLAG_IMMEDIATE 0x80
#define LOAD_QUEUE_FLAG_PRESTART_FADE 0x40
#endif

class LoadQueue {
  public:
  static_assert((NUM_LINKS & (NUM_LINKS - 1)) == 0,
                "LoadQueue wrap assumes power-of-two NUM_LINKS");
  GridRow row_selects[NUM_LINKS][NUM_SLOTS];
#if !defined(__AVR__)
  uint32_t private_source_ids[NUM_LINKS][NUM_SLOTS];
#endif
  LoadQueueModes modes[NUM_LINKS];
  uint8_t rd;
  uint8_t wr;
  bool full;

  void init() {
    rd = 0;
    wr = 0;
    full = false;
#if !defined(__AVR__)
    memset(private_source_ids, 0, sizeof(private_source_ids));
#endif
  }

  void put(uint8_t mode, GridRow *row_select, GridSlot offset = 255) {
    if (full) { return; }
    memcpy(row_selects[wr], row_select, NUM_SLOTS);
#if !defined(__AVR__)
    memset(private_source_ids[wr], 0, sizeof(private_source_ids[wr]));
#endif
    modes[wr].mode = mode;
    modes[wr].offset = offset;
    wr = (wr + 1) & (NUM_LINKS - 1);
    if (wr == rd) {
        full = true;
    }
  }

#if !defined(__AVR__)
  void put_arrangement(uint8_t mode, GridRow *row_select,
                       const uint32_t *source_ids,
                       GridSlot offset = 255) {
    if (full) { return; }
    memcpy(row_selects[wr], row_select, NUM_SLOTS);
    memset(private_source_ids[wr], 0, sizeof(private_source_ids[wr]));
    if (source_ids != nullptr) {
      memcpy(private_source_ids[wr], source_ids,
             sizeof(private_source_ids[wr]));
    }
    modes[wr].mode = mode;
    modes[wr].offset = offset;
    wr = (wr + 1) & (NUM_LINKS - 1);
    if (wr == rd) {
      full = true;
    }
  }
#endif

  void put(uint8_t mode, GridRow row, uint8_t *track_select_array,
           GridSlot offset = 255) {
    if (full) { return; }

    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
       row_selects[wr][n] = 255;
       if (track_select_array[n]) { row_selects[wr][n] = row; }
    }
#if !defined(__AVR__)
    memset(private_source_ids[wr], 0, sizeof(private_source_ids[wr]));
#endif
    modes[wr].mode = mode;
    modes[wr].offset = offset;
    wr = (wr + 1) & (NUM_LINKS - 1);
    if (wr == rd) {
      full = true;
    }
  }

  void get(uint8_t &mode, GridSlot &offset, GridRow *row_select,
           uint8_t *track_select) {
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      row_select[n] = row_selects[rd][n];
      track_select[n] = row_select[n] < GRID_LENGTH;
    }
    mode = modes[rd].mode;
    offset = modes[rd].offset;
    rd = (rd + 1) & (NUM_LINKS - 1);
    full = false;
  }

#if !defined(__AVR__)
  void get(uint8_t &mode, GridSlot &offset, GridRow *row_select,
           uint8_t *track_select, uint32_t *source_ids) {
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      row_select[n] = row_selects[rd][n];
      if (source_ids != nullptr) {
        source_ids[n] = private_source_ids[rd][n];
      }
      track_select[n] = row_select[n] < GRID_LENGTH ||
                        row_select[n] == LOAD_QUEUE_PRIVATE_ROW;
    }
    mode = modes[rd].mode;
    offset = modes[rd].offset;
    rd = (rd + 1) & (NUM_LINKS - 1);
    full = false;
  }
#endif

  bool is_empty() {
    return !full && (rd == wr);
  }

};

class GridTask : public Task {

public:
  bool stop_hard_callback;

  char kit_names[NUM_DEVS][16];
  bool send_kit_name;

  GridRow last_active_row;
  GridRow next_active_row;
  bool chain_behaviour;
  bool update;

  GridRow load_row_midi = 255;
  GridRow load_row = 255;
  GridRow midi_row = 255;

  GridRow load_track_select[NUM_SLOTS];
  GridRow midi_row_select = 255;

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
  void wait_blocking(uint32_t go_step);

  bool link_load(GridSlot n, GridRow *slots_changed,
                 uint8_t *track_select_array, GridDeviceTrack *gdt);
  bool transition_load(GridSlot n, GridColumn track_idx, GridDeviceTrack *gdt);
  bool transition_send(GridSlot n, GridColumn track_idx, uint8_t dev_idx, GridDeviceTrack *gdt);

  /* @} */
};

extern GridTask grid_task;
#endif /* GRID_TASK_H__ */
