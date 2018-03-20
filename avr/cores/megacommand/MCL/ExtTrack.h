/* Justin Mammarella jmamma@gmail.com 2018 */
#include "MCL.h"

#ifndef EXTTRACK_H__
#define EXTTRACK_H__

class ExtTrack {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  char kitName[17];
  char trackName[17];

  ExtTrackData seq_data;

  bool get_track_from_sysex(int tracknumber, uint8_t column);
  bool place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
}

#endif /* EXTTRACK_H__ */
