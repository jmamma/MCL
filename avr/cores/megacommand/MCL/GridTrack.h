/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef GRIDTRACK_H__
#define GRIDTRACK_H__

#define EMPTY_TRACK_TYPE 0

#include "GridChain.h"

class GridTrack {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  char trackName[17];
  GridChain chain;
//  bool get_track_from_sysex(uint8_t tracknumber, uint8_t column);
//  void place_track_in_sysex(uint8_t tracknumber, uint8_t column);
  bool load_track_from_grid(uint8_t column, uint8_t row);
  bool store_track_in_grid(uint8_t column, uint8_t row);

};

#endif /* GRIDTRACK_H__ */
