/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLACTIONS_H__
#define MCLACTIONS_H__

#include "MCL.h"
#include "MCLActionsEvents.h"

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254

class MCLActions {
public:
  unit8_t kit_reload;

  MCLActions() {}
  void setup();
  void send_globals();
  void switch_global(uint8_t global_page);
  void kit_reload(uint8_t pattern);

  void md_setsysex_recpos(uint8_t rec_type, uint8_t position);
  void store_tracks_in_men(int column, int row, int store_behaviour_);
  void write_tracks_to_md(int column, int row, int b);
  void send_pattern_kit_to_md();
};

MCLActionsCallbacks mcl_actions_callbacks;
MCLActionsMidiEvents mcl_actions_midievents;

extern MCLActions mcl_actions;

#endif /* MCLACTIONS_H__ */
