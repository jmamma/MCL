/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef GRIDTRACK_H__
#define GRIDTRACK_H__

#define EMPTY_TRACK_TYPE 0

class GridTrack {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  char trackName[17];
  /*
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  void place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
*/
};

#endif /* GRIDTRACK_H__ */
