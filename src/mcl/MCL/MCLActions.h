/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "GridChain.h"
#include "MCLActionsEvents.h"
#include "MCLPlatformFeatures.h"
#include "MidiDeviceGrid.h"
#include "MCLSysConfig.h"
#include "GridLink.h"
#include "TrackLoadFade.h"

class DeviceTrack;
class EmptyTrack;
class MidiUartClass;
class SeqTrack;

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254

#define TRANSITION_NORMAL 0
#define TRANSITION_MUTE 2
#define TRANSITION_UNMUTE 3

#if MCL_FEATURE_HOST_ARRANGER
uint32_t selected_destination_mask(const uint8_t *slot_select_array,
                                   GridSlot load_offset);
#endif

//div192th_time = 1.25 / tempo;
//diff * div19th_time > 80ms equivalent to diff > (0.08/1.25) * tempo
//float ms = (0.08 * 0.80) * tempo == 0.064 * tempo; 80ms

#if defined(__AVR__)
  #define GUI_THRESHOLD_FACTOR 0.064f  //80ms
  #define TRACK_MIN_LOAD_TIME 4 //4ms
#else
  #define GUI_THRESHOLD_FACTOR 0.016f  //20ms
  #define TRACK_MIN_LOAD_TIME 1 //1ms
#endif

class DeviceLatency {
public:
  uint16_t latency_bytes;
  uint16_t div192th_latency;
};

class LinkModeData {
public:
  DeviceLatency dev_latency[NUM_DEVS];

  uint16_t div192th_total_latency;
  uint16_t div32th_total_latency;

  GridLink links[NUM_SLOTS];

  uint16_t md_latency;

  uint16_t next_transition = (uint16_t)-1;
  uint16_t nearest_bar;
  uint8_t nearest_beat;

  uint16_t next_transitions[NUM_SLOTS];
  uint8_t transition_offsets[NUM_SLOTS];
  uint8_t send_machine[NUM_SLOTS];
  uint8_t transition_level[NUM_SLOTS];

  GridSlot dev_sync_slot[NUM_DEVS];

  GridChain chains[NUM_SLOTS];
};

#define QUANT_LEN 255

class MCLActions : public LinkModeData {
public:
  uint8_t do_kit_reload;
  int writepattern;
  uint8_t write_original;

  uint8_t patternswitch = PATTERN_UDEF;

  uint16_t start_clock16th;
  uint32_t start_clock32th;
  uint32_t start_clock96th;
  uint8_t store_behaviour;

  MCLActions() {}

  void setup();

  uint8_t get_quant() {
    uint8_t q;
    if (mcl_cfg.chain_load_quant == 1) {
      q = QUANT_LEN; // use slot settings
    } else {
      q = mcl_cfg.chain_load_quant; // override
    }
    return q;
  }

  // This is the track length quantisatioe
  uint8_t get_chain_length() {
    uint8_t q;
    if (mcl_cfg.chain_queue_length == 1) {
      q = QUANT_LEN; // use slot settings
    } else {
      q = mcl_cfg.chain_queue_length; // override
    }
    return q;
  }

  GridDeviceTrack *get_grid_dev_track(GridSlot slot_number);

  void init_chains();

  void send_globals();
  void switch_global(uint8_t global_page);
  void kit_reload(uint8_t pattern);
  void row_update(GridSlot last_slot);
  void save_tracks(GridRow row, uint8_t *slot_select_array, uint8_t merge,
                   uint8_t readpattern = 255);

  void load_tracks(uint8_t *slot_select_array,
                   GridRow *_row_array = nullptr, uint8_t load_mode = 255,
                   GridSlot load_offset = 255
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
                   ,
                   bool immediate = false,
                   bool allow_prestart_fades = false
#endif
                   );
#if !defined(__AVR__)
  void clear_tracks(uint8_t *slot_select_array);
#endif
  void send_tracks_to_devices(uint8_t *slot_select_array,
                              GridRow *row_array = nullptr,
                              GridSlot load_offset = 255
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
                              ,
                              bool allow_prestart_fades = false
#endif
                              );
  void manual_transition(uint8_t *slot_select_array, GridRow *row_array,
                         GridSlot load_offset);
  void update_chain_links(GridSlot n, GridDeviceTrack *gdt);
  void cache_track(GridSlot n, GridDeviceTrack* gdt, GridColumn track_idx);
  void cache_next_tracks(uint8_t *slot_select_array, bool gui_update = false);
  void calc_next_slot_transition(GridSlot n, bool ignore_chain_settings = false,
                                 bool ignore_overflow = false);
  void calc_next_transition();
  void calc_latency();
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  void clear_load_fades(bool preserve_armed_prestart = false);
#else
  void clear_load_fades();
#endif
  void start_load_fade_at(GridSlot slot, const TrackLoadFadeData *fade,
                          uint32_t start_clock
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
                          ,
                          bool allow_prestart = false
#endif
                          );

private:
  DeviceTrack *load_and_prepare_track(GridSlot track_idx, GridRow row,
                                      uint8_t track_type, SeqTrack *seq_track,
                                      uint8_t seq_track_idx, bool &was_rebuilt,
                                      EmptyTrack &scratch,
                                      int8_t link_slot = -1);
  void collect_tracks(uint8_t *slot_select_array, GridRow *row_array,
                      GridSlot load_offset);
  bool load_track_immediate(GridRow row, GridSlot i, GridSlot dst,
                            GridDeviceTrack *gdt_dst, uint8_t *send_masks
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
                            ,
                            bool allow_prestart_fade = false
#endif
                            );
  void restore_mute_states(uint8_t *mute_states);
};

extern void md_import();

extern MCLActionsCallbacks mcl_actions_callbacks;
extern MCLActionsMidiEvents mcl_actions_midievents;

extern MCLActions mcl_actions;
