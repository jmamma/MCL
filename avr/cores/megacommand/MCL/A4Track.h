/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "ExtTrack.h"
#include "A4.h"
#include "Project.h"
#include "MCLMemory.h"

class A4Track : public ExtTrack {
public:
  A4Sound sound;
  void load_seq_data(int tracknumber);
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  bool place_track_in_sysex(int tracknumber, uint8_t column,
                           A4Sound *analogfour_sound);
  bool load_track_from_grid(int32_t column, int32_t row, int m = 0);
  bool store_track_in_grid(int32_t column, int32_t row, int track = 255);

  //Store/retrieve portion of track object in mem bank2
  bool store_in_mem(uint8_t column, uint32_t region = BANK1_A4_TRACKS_START);
  bool load_from_mem(uint8_t column, uint32_t region = BANK1_A4_TRACKS_START);
};

#endif /* A4TRACK_H__ */
