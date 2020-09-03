/* Yatao Li yatao.li@live.com 2020 */

#pragma once

#include "ExtTrack.h"
#include "MNM.h"

class MNMTrack : public ExtTrack {
public:
  MNMMachine machine;
  MNMTrack() {
    active = MNM_TRACK_TYPE;
    static_assert(sizeof(MNMTrack) <= GRID2_TRACK_LEN);
  }

  virtual void load_immediate(uint8_t tracknumber);
  void place_track_in_kit(uint8_t tracknumber, MNMKit *kit, bool levels = true);
  void get_machine_from_kit(uint8_t tracknumber);
  virtual bool store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge, bool online = false);
  virtual uint16_t get_track_size() { return sizeof(MNMTrack); }
  virtual uint8_t get_model() { return machine.model; }
  virtual uint8_t get_device_type() { return MNM_TRACK_TYPE; }

  virtual void* get_sound_data_ptr() { return &machine; }
  virtual size_t get_sound_data_size() { return sizeof(MNMMachine); }
};
