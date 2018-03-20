/* Justin Mammarella jmamma@gmail.com 2018 */
#include "MCL.h"

#ifndef A4TRACK_H__
#define A4TRACK_H__

class A4Track : public ExtTrack {
public:
  A4Sound sound;
  bool getTrack_from_sysex(int tracknumber, uint8_t column);
  bool placeTrack_in_sysex(int tracknumber, uint8_t column,
                           A4Sound *analogfour_sound);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
};

#endif /* A4TRACK_H__ */
