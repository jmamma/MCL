/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "DeviceTrack.h"

class MDFXData {
public:
  bool enable_reverb;
  bool enable_delay;
  bool enable_eq;
  bool enable_dynamics;
  /** The settings of the reverb effect. f**/
  uint8_t reverb[8];
  /** The settings of the delay effect. **/
  uint8_t delay[8];
  /** The settings of the EQ effect. **/
  uint8_t eq[8];
  /** The settings of the compressor effect. **/
  uint8_t dynamics[8];
};

class MDFXTrack : public DeviceTrack, public MDFXData {
public:
  MDFXTrack() {
    active = MDFX_TRACK_TYPE;
  }

  void init() {
     enable_reverb = false;
     enable_delay = false;
     enable_eq = false;
     enable_dynamics = false;
  }

  void place_fx_in_kit();
  void get_fx_from_kit();
  uint16_t calc_latency(uint8_t tracknumber);
  uint16_t send_fx(bool send = true);
  void transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber);

  bool store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track = nullptr,
                                uint8_t merge = 0, bool online = false);

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);

  virtual uint16_t get_track_size() { return sizeof(MDFXTrack); }
  virtual uint32_t get_region() { return BANK1_AUX_TRACKS_START; }

  virtual uint8_t get_model() { return MDFX_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return MDFX_TRACK_TYPE; }

  virtual void* get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }
};
