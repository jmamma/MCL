/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "GridChain.h"
#include "MCLActionsEvents.h"

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254

#define TRANSITION_NORMAL 0
#define TRANSITION_MUTE 2
#define TRANSITION_UNMUTE 3

class DeviceLatency {
public:
  uint16_t latency;
  // uint16_t load_latency;
  uint8_t div32th_latency;
  uint8_t div192th_latency;
};

class LinkModeData {
public:
  DeviceLatency dev_latency[NUM_DEVS];

  uint8_t div192th_total_latency;
  uint8_t div32th_total_latency;

  GridLink links[NUM_SLOTS];

  uint16_t md_latency;

  uint16_t next_transition = (uint16_t)-1;

  uint16_t nearest_bar;
  uint8_t nearest_beat;

  uint16_t next_transitions[NUM_SLOTS];
  uint8_t transition_offsets[NUM_SLOTS];
  uint8_t send_machine[NUM_SLOTS];
  uint8_t transition_level[NUM_SLOTS];

  uint8_t dev_sync_slot[NUM_DEVS];

  GridChain chains[NUM_SLOTS];
};

class MCLActions : public LinkModeData {
public:
  uint8_t do_kit_reload;
  int writepattern;
  uint8_t write_original = 0;

  uint8_t patternswitch = PATTERN_UDEF;

  uint16_t start_clock16th = 0;
  uint32_t start_clock32th = 0;
  uint32_t start_clock96th = 0;
  uint8_t store_behaviour;

  MCLActions() {}

  void setup();

  uint8_t get_quant() {
    uint8_t q;
    if (gridio_param4.cur == 0) {
      q = 4;
    } else {
      q = 1 << gridio_param4.cur;
    }
    if (q < 4) {
      q = 4;
    }
    return q;
  }

  uint8_t get_grid_idx(uint8_t slot_number);
  GridDeviceTrack *get_grid_dev_track(uint8_t slot_number, uint8_t *id,
                                      uint8_t *dev_idx);

  void send_globals();
  void switch_global(uint8_t global_page);
  void kit_reload(uint8_t pattern);

  void store_tracks_in_mem(int column, int row, uint8_t *slot_select_array,
                           uint8_t merge);

  void load_tracks(int column, int row, uint8_t *slot_select_array);
  void send_tracks_to_devices(uint8_t *slot_select_array, uint8_t *row_array = nullptr);
  void prepare_next_transition(int row, uint8_t *slot_select_array);

  void cache_next_tracks(uint8_t *slot_select_array, bool gui_update = false);
  void calc_next_slot_transition(uint8_t n);
  void calc_next_transition();
  void calc_latency();

private:
  void collect_tracks(int row, uint8_t *slot_select_array);
  void cache_track(uint8_t n, uint8_t track_idx, uint8_t dev_idx, GridDeviceTrack *gdt);
  void load_track(uint8_t track_idx, uint8_t row, uint8_t pos, GridDeviceTrack *gdt, uint8_t *send_masks);
};

extern MCLActionsCallbacks mcl_actions_callbacks;
extern MCLActionsMidiEvents mcl_actions_midievents;

extern MCLActions mcl_actions;
