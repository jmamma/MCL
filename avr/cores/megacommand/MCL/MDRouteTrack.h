/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"

class RouteData {
public:
  uint8_t routing[16];
  uint16_t poly_mask;
};

class MDRouteTrack : public AUXTrack, public RouteData {
public:
  MDRouteTrack() {
    active = MDROUTE_TRACK_TYPE;
    static_assert(sizeof(MDRouteTrack) <= MDROUTE_TRACK_LEN);
  }

  void init() { memset(routing, 6, sizeof(routing)); }

  void get_routes();
  uint16_t calc_latency(uint8_t tracknumber);
  uint16_t send_routes(bool send = true);
  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false);

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  void send_data();

  virtual uint16_t get_track_size() { return sizeof(MDRouteTrack); }
  virtual uint32_t get_region() { return BANK1_MDROUTE_TRACK_START; }

  virtual uint8_t get_model() { return MDROUTE_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return MDROUTE_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return &routing; }
  virtual size_t get_sound_data_size() { return sizeof(routing); }
};
