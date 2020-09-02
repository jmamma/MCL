/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "DeviceTrack.h"
#include "MDFXData.h"

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

  bool store_in_grid(uint8_t tracknumber, uint16_t row,
                                uint8_t merge, bool online);

  void load_immediate(uint8_t tracknumber);

  virtual uint16_t get_track_size() { return sizeof(MDFXTrack); }
  virtual uint32_t get_region() { return BANK1_FX_TRACKS_START; }

  virtual uint8_t get_model() { return MDFX_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return MDFX_TRACK_TYPE; }
};
