#ifndef GRID_TASK_H__
#define GRID_TASK_H__

#include "MCL.h"
#include "Task.h"
#include "Elektron.h"
#include "Grid/GridChain.h"
#include "MCLPlatformFeatures.h"

class LoadQueueModes {
  public:
  uint8_t mode;
  GridSlot offset;
};

#if MCL_FEATURE_GRID_PRIVATE_LOADS || MCL_FEATURE_HOST_ARRANGER
#define LOAD_QUEUE_CLEAR_ROW 254
#define LOAD_QUEUE_PRIVATE_ROW 253
#define LOAD_QUEUE_FLAG_IMMEDIATE 0x80
#define LOAD_QUEUE_FLAG_PRESTART_FADE 0x40
#define LOAD_QUEUE_FLAG_ARRANGER_PRELOAD 0x20
#endif

class LoadQueue {
  public:
#if MCL_FEATURE_HOST_ARRANGER
  // One arranger boundary can contain an active and a preloaded source for
  // every destination, plus a clear request. Leave the original eight-entry
  // backlog in front of it. AVR has no host arranger and retains its original,
  // SRAM-conscious depth and power-of-two wrap.
  static constexpr uint8_t kCapacity = NUM_SLOTS * 2 + 1 + NUM_LINKS;
#else
  static constexpr uint8_t kCapacity = NUM_LINKS;
  static_assert((kCapacity & (kCapacity - 1)) == 0,
                "LoadQueue wrap assumes a power-of-two capacity");
#endif
  GridRow row_selects[kCapacity][NUM_SLOTS];
#if MCL_FEATURE_GRID_PRIVATE_LOADS
  uint32_t private_source_ids[kCapacity][NUM_SLOTS];
#endif
  LoadQueueModes modes[kCapacity];
  uint8_t rd;
  uint8_t wr;
  bool full;

  static uint8_t next_index(uint8_t index) {
#if MCL_FEATURE_HOST_ARRANGER
    ++index;
    return index == kCapacity ? 0 : index;
#else
    return (index + 1) & (kCapacity - 1);
#endif
  }

  void clear_private_sources(GridSlot link_idx) {
#if MCL_FEATURE_GRID_PRIVATE_LOADS
    memset(private_source_ids[link_idx], 0, sizeof(private_source_ids[link_idx]));
#else
    (void)link_idx;
#endif
  }

  void clear_all_private_sources() {
#if MCL_FEATURE_GRID_PRIVATE_LOADS
    memset(private_source_ids, 0, sizeof(private_source_ids));
#endif
  }

  void commit_write(uint8_t mode, GridSlot offset) {
    modes[wr].mode = mode;
    modes[wr].offset = offset;
    wr = next_index(wr);
    if (wr == rd) {
        full = true;
    }
  }

  void init() {
    rd = 0;
    wr = 0;
    full = false;
    clear_all_private_sources();
  }

  bool put(uint8_t mode, GridRow *row_select, GridSlot offset = 255) {
    if (full || row_select == nullptr) { return false; }
    memcpy(row_selects[wr], row_select, NUM_SLOTS);
    clear_private_sources(wr);
    commit_write(mode, offset);
    return true;
  }

#if MCL_FEATURE_GRID_PRIVATE_LOADS
  bool put_arrangement(uint8_t mode, GridRow *row_select,
                       const uint32_t *source_ids,
                       GridSlot offset = 255) {
    if (full || row_select == nullptr) { return false; }
    memcpy(row_selects[wr], row_select, NUM_SLOTS);
    memset(private_source_ids[wr], 0, sizeof(private_source_ids[wr]));
    if (source_ids != nullptr) {
      memcpy(private_source_ids[wr], source_ids,
             sizeof(private_source_ids[wr]));
    }
    commit_write(mode, offset);
    return true;
  }
#endif

  bool put(uint8_t mode, GridRow row, uint8_t *track_select_array,
           GridSlot offset = 255) {
    if (full || track_select_array == nullptr) { return false; }

    memset(row_selects[wr], 255, NUM_SLOTS);
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
       if (track_select_array[n]) { row_selects[wr][n] = row; }
    }
    clear_private_sources(wr);
    commit_write(mode, offset);
    return true;
  }

  void get(uint8_t &mode, GridSlot &offset, GridRow *row_select,
           uint8_t *track_select) {
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      GridRow row = row_selects[rd][n];
      row_select[n] = row;
      track_select[n] = row < GRID_LENGTH;
    }
    mode = modes[rd].mode;
    offset = modes[rd].offset;
    rd = next_index(rd);
    full = false;
  }

#if defined(__AVR__)
  GridRow *get(uint8_t &mode, GridSlot &offset, uint8_t *track_select) {
    GridRow *row_select = row_selects[rd];
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      track_select[n] = row_select[n] < GRID_LENGTH;
    }
    mode = modes[rd].mode;
    offset = modes[rd].offset;
    rd = next_index(rd);
    full = false;
    return row_select;
  }
#endif

#if MCL_FEATURE_GRID_PRIVATE_LOADS
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
    rd = next_index(rd);
    full = false;
  }
#endif

  uint8_t available() const {
    if (full) {
      return 0;
    }
#if MCL_FEATURE_HOST_ARRANGER
    uint8_t used = wr >= rd ? (uint8_t)(wr - rd)
                            : (uint8_t)(kCapacity - rd + wr);
#else
    uint8_t used = (uint8_t)((wr - rd) & (kCapacity - 1));
#endif
    return (uint8_t)(kCapacity - used);
  }

  bool is_empty() const {
    return !full && (rd == wr);
  }

};

#if MCL_FEATURE_GRID_SAVE_QUEUE
class SaveQueue {
  public:
  static_assert((NUM_LINKS & (NUM_LINKS - 1)) == 0,
                "SaveQueue wrap assumes power-of-two NUM_LINKS");
  GridRow rows[NUM_LINKS];
  uint8_t track_selects[NUM_LINKS][NUM_SLOTS];
  uint8_t merges[NUM_LINKS];
#if MCL_FEATURE_HOST_ARRANGER
  uint8_t ack_tags[NUM_LINKS];
  uint8_t ack_valids[NUM_LINKS];
#endif
  uint8_t rd;
  uint8_t wr;
  bool full;

  void init() {
    rd = 0;
    wr = 0;
    full = false;
    memset(rows, 255, sizeof(rows));
    memset(track_selects, 0, sizeof(track_selects));
    memset(merges, 0, sizeof(merges));
#if MCL_FEATURE_HOST_ARRANGER
    memset(ack_tags, 0, sizeof(ack_tags));
    memset(ack_valids, 0, sizeof(ack_valids));
#endif
  }

  bool put(GridRow row, const uint8_t *track_select_array, uint8_t merge
#if MCL_FEATURE_HOST_ARRANGER
           ,
           uint8_t ack_tag = 0, bool ack_valid = false
#endif
           ) {
    if (full || track_select_array == nullptr) {
      return false;
    }
    rows[wr] = row;
    memcpy(track_selects[wr], track_select_array, NUM_SLOTS);
    merges[wr] = merge;
#if MCL_FEATURE_HOST_ARRANGER
    ack_tags[wr] = ack_tag;
    ack_valids[wr] = ack_valid ? 1 : 0;
#endif
    wr = (wr + 1) & (NUM_LINKS - 1);
    if (wr == rd) {
      full = true;
    }
    return true;
  }

  bool get(GridRow &row, uint8_t *track_select_array, uint8_t &merge
#if MCL_FEATURE_HOST_ARRANGER
           ,
           uint8_t *ack_tag = nullptr, bool *ack_valid = nullptr
#endif
           ) {
    if (is_empty() || track_select_array == nullptr) {
      return false;
    }
    row = rows[rd];
    memcpy(track_select_array, track_selects[rd], NUM_SLOTS);
    merge = merges[rd];
#if MCL_FEATURE_HOST_ARRANGER
    if (ack_tag != nullptr) {
      *ack_tag = ack_tags[rd];
    }
    if (ack_valid != nullptr) {
      *ack_valid = ack_valids[rd] != 0;
    }
#endif
    rd = (rd + 1) & (NUM_LINKS - 1);
    full = false;
    return true;
  }

  bool is_empty() {
    return !full && (rd == wr);
  }
};
#endif

struct GridActiveStateDirty {
#if MCL_FEATURE_HOST_ARRANGER
  bool changed = false;

  void mark() {
    changed = true;
  }

  void mark_row_change(GridRow old_row, GridRow new_row,
                       bool old_chain, bool new_chain) {
    if (old_row != new_row || old_chain != new_chain) {
      changed = true;
    }
  }

  void flush();
#else
  void mark() {}
  void mark_row_change(GridRow, GridRow, bool, bool) {}
  void flush() {}
#endif
};

class GridTask : public Task {
  static bool is_grid_chain_load_mode(uint8_t mode) {
    return mode == LOAD_QUEUE || mode == LOAD_AUTO;
  }

#if MCL_FEATURE_GRID_PRIVATE_LOADS
  static uint32_t selected_track_mask(const uint8_t *track_select);
#endif

#if MCL_FEATURE_HOST_ARRANGER
  static bool save_needs_md_current_pattern();
  static void reset_host_playback_after_stop();
  static void tick_host_arranger();
  static void flush_host_automation_writes();
  static void notify_host_seq_dirty_for_load(const uint8_t *track_select,
                                             const uint8_t *clear_select,
                                             GridSlot load_offset);
#else
  static void reset_host_playback_after_stop() {}
  static void tick_host_arranger() {}
  static void flush_host_automation_writes() {}
  static void notify_host_seq_dirty_for_load(const uint8_t *,
                                             const uint8_t *, GridSlot) {}
#endif

#if MCL_FEATURE_GRID_SAVE_QUEUE
  void init_save_queue();
  void save_queue_handler();
#else
  void init_save_queue() {}
  void save_queue_handler() {}
#endif

#if defined(__AVR__)
  static void pre_cache_ui_yield();
#else
  static void pre_cache_ui_yield() {}
#endif

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
#if MCL_FEATURE_GRID_SAVE_QUEUE
  SaveQueue save_queue;
#endif

  GridTask(uint16_t interval) : Task(interval) { setup(interval); }

  void setup(uint16_t interval = 0);

  virtual void run();
  void init() {
     reset_midi_states();
     load_queue.init();
     init_save_queue();
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
  void service_host_arranger_load_before_edit();
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
