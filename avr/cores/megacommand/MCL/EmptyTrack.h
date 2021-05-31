/* Justin Mammarella jmamma@gmail.com 2018 */
#pragma once

#include "DeviceTrack.h"

class EmptyTrack : public DeviceTrack {
public:
  //Assume A4Track consume most data size out
  //of all the tracktypes

  uint8_t data[EMPTY_TRACK_LEN];
  EmptyTrack() {
  active = EMPTY_TRACK_TYPE;
  }
  uint16_t get_track_size() { return sizeof(EmptyTrack); }
  virtual bool store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track = nullptr, uint8_t merge = 0, bool online = false) {
    // should not reach here
    DEBUG_PRINT_FN();
    GridTrack::store_in_grid(column, row);
  }

  virtual void* get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }

  /// Erase the track data, without erasing the link data.
  void clear();

  /*
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  void place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
*/
};

