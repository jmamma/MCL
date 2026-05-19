/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "PtcGroups.h"

class ATTR_PACKED() LegacyMDRouteData {
public:
  uint8_t routing[16];
  uint16_t poly_mask;
};

class ATTR_PACKED() MDRouteData {
public:
  uint8_t routing[16];
  uint8_t ptc_group[PTC_GROUP_TRACKS];
};

class ATTR_PACKED() LegacyMDRouteTrack : public AUXTrack,
                                         public LegacyMDRouteData {
public:
  LegacyMDRouteTrack() {
    active = MDROUTE_TRACK_TYPE;
  }
  size_t _sizeof() const {
     return sizeof(LegacyMDRouteTrack) - sizeof(void*);
  }
  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {
    memset(routing, 6, sizeof(routing));
    poly_mask = 0;
  }

  void get_routes();
  uint16_t calc_latency(uint8_t tracknumber);
  uint16_t send_routes(bool send = true);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber);
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  void load_routes();

  virtual uint16_t get_track_size() { return _sizeof(); }
  virtual uintptr_t get_region() { return BANK1_MDROUTE_TRACK_START; }

  virtual uint8_t get_model() { return MDROUTE_TRACK_TYPE; }
  virtual void *get_sound_data_ptr() { return &routing; }
  virtual size_t get_sound_data_size() { return sizeof(LegacyMDRouteData); }
};

class ATTR_PACKED() MDRouteTrack : public AUXTrack, public MDRouteData {
public:
  MDRouteTrack() {
    active = MD_ROUTE_TRACK_TYPE;
  }
  size_t _sizeof() const {
     return sizeof(MDRouteTrack) - sizeof(void*);
  }
  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {
    memset(routing, 6, sizeof(routing));
    clear_ptc_groups();
  }

  void clear_ptc_groups();
  void load_ptc_groups();
  void get_routes();
  uint16_t calc_latency(uint8_t tracknumber);
  uint16_t send_routes(bool send = true);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber);
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  void load_routes();

  virtual uint16_t get_track_size() { return _sizeof(); }
  virtual uintptr_t get_region() { return BANK1_MDROUTE_TRACK_START; }

  virtual uint8_t get_model() { return MD_ROUTE_TRACK_TYPE; }
  virtual void *get_sound_data_ptr() { return &routing; }
  virtual size_t get_sound_data_size() { return sizeof(MDRouteData); }
};

static_assert(sizeof(LegacyMDRouteData) == 18,
              "LegacyMDRouteData storage size changed");
static_assert(sizeof(MDRouteData) == 32,
              "MDRouteData storage size changed");
