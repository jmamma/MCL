/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "DeviceTrack.h"

class MDFXTrack : public DeviceTrck {
public:
  MDFXTrack() {
    active = MDFX_TRACK_TYPE;
    DIAG_PRINTLN("MDFXTrack ctor");
  }

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

  virtual void on_copy(int16_t s_col, int16_t d_col, bool destination_same);

  virtual uint8_t get_model() { return MDFX_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return MDFX_TRACK_TYPE; }
};
