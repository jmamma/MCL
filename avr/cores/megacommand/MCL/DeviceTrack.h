/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef DEVICETRACK_H__
#define DEVICETRACK_H__

#include "GridTrack.h"

#define A4_TRACK_TYPE_270 2
#define MD_TRACK_TYPE_270 1
#define EXT_TRACK_TYPE_270 3

#define MD_TRACK_TYPE 4
#define A4_TRACK_TYPE 5
#define EXT_TRACK_TYPE 6

class DeviceTrack : public GridTrack {

public:
  uint8_t active = EMPTY_TRACK_TYPE;

  GridChain chain;
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);
  bool init_track_type(uint8_t track_type);

};

#endif /* DEVICETRACK_H__ */
