/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

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
  uint8_t div32th_latency;
  uint8_t div192th_latency;
};

class ChainModeData {
public:
  DeviceLatency dev_latency[2];
  uint8_t div32th_total_latency;
  uint8_t div192th_total_latency;

  GridChain chains[NUM_SLOTS];

  uint16_t md_latency;

  uint16_t next_transition = (uint16_t)-1;

  uint16_t nearest_bar;
  uint8_t nearest_beat;

  uint16_t next_transitions[NUM_SLOTS];
  uint8_t transition_offsets[NUM_SLOTS];
  uint8_t send_machine[NUM_SLOTS];
  uint8_t transition_level[NUM_SLOTS];

};

class MCLActions : public ChainModeData {
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

  uint8_t get_grid_idx(uint8_t slot_number);
  GridDeviceTrack *get_grid_dev_track(uint8_t slot_number, uint8_t *id, uint8_t *dev_idx);

  SeqTrack *get_dev_slot_info(uint8_t slot_number, uint8_t *grid_idx, uint8_t *track_idx, uint8_t *track_type, uint8_t *dev_idx, bool *is_aux = nullptr);

  void send_globals();
  void switch_global(uint8_t global_page);
  void kit_reload(uint8_t pattern);

  void store_tracks_in_mem(int column, int row, uint8_t *slot_select_array,
                           uint8_t merge);

  void write_tracks(int column, int row, uint8_t *slot_select_array);
  void send_tracks_to_devices(uint8_t *slot_select_array);
  void prepare_next_chain(int row, uint8_t *slot_select_array);

  void cache_next_tracks(uint8_t *slot_select_array, EmptyTrack *empty_track,
                         EmptyTrack *empty_track2, bool update_gui = false);
  void calc_next_slot_transition(uint8_t n);
  void calc_next_transition();
  void calc_latency(DeviceTrack *empty_track);
};

extern MCLActionsCallbacks mcl_actions_callbacks;
extern MCLActionsMidiEvents mcl_actions_midievents;

extern MCLActions mcl_actions;
