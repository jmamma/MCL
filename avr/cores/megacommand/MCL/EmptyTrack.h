/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EMPTYTRACK_H__
#define EMPTYTRACK_H__

#include "Grid.h"
#include "DeviceTrack.h"
#include "MCLMemory.h"

class EmptyTrack : public DeviceTrack {
public:
  //Assume A4Track consume most data size out
  //of all the tracktypes

  uint8_t data[EMPTY_TRACK_LEN];
  uint16_t get_track_size() { return sizeof(EmptyTrack); }
  virtual bool store_in_grid(uint8_t column, uint16_t row, uint8_t merge = 0, bool online = false) {
    // should not reach here
    DEBUG_PRINT_FN();
    GridTrack::store_in_grid(column, row);
  }

  /*
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  void place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
*/
};

#endif /* EMPTYTRACK_H__ */
