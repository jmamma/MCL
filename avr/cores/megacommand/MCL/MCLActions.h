/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLACTIONS_H__
#define MCLACTIONS_H__

#include "A4.h"
#include "EmptyTrack.h"
#include "MCLActionsEvents.h"
#include "MD.h"

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254

class MCLActions {
public:
  uint8_t do_kit_reload;
  int writepattern;
  uint8_t write_original = 0;

  uint8_t patternswitch = PATTERN_UDEF;

  uint16_t start_clock16th = 0;
  uint32_t start_clock32th = 0;
  uint32_t start_clock96th = 0;
  uint8_t store_behaviour;

  GridChain chains[20];
  uint16_t md_latency;
  uint16_t a4_latency;
  uint16_t nearest_step;
  bool active = false;

  uint16_t nearest_steps[20] = { };

  MCLActions() {}

  void setup();
  void send_globals();
  void switch_global(uint8_t global_page);
  void kit_reload(uint8_t pattern);

  bool place_track_inpattern(int curtrack, int column, int row,
                             A4Sound *analogfour_sound,
                             EmptyTrack *empty_track);
  void md_setsysex_recpos(uint8_t rec_type, uint8_t position);

  void store_tracks_in_mem(int column, int row, int store_behaviour_);
  void write_tracks_to_md(int column, int row, int b);
  void send_pattern_kit_to_md();
  
  void calc_nearest_slot_step(uint8_t n);
  void calc_nearest_step();
  void calc_latency(EmptyTrack *empty_track);
  int calc_md_set_machine_latency(uint8_t track, MDMachine *model,
                                  MDKit *kit_ = NULL);
  void md_set_machine(uint8_t track, MDMachine *model, MDKit *kit_ = NULL);

};

extern MCLActionsCallbacks mcl_actions_callbacks;
extern MCLActionsMidiEvents mcl_actions_midievents;

extern MCLActions mcl_actions;

#endif /* MCLACTIONS_H__ */
