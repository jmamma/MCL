/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EMPTYTRACK_H__
#define EMPTYTRACK_H__

#include "Grid.h"
#include "GridTrack.h"
#include "MCLMemory.h"

class EmptyTrack : public GridTrack {
public:
  //Assume A4Track consume most data size out
  //of all the tracktypes

  uint8_t data[EMPTY_TRACK_LEN];
        /*
  bool get_track_from_sysex(uint8_t tracknumber, uint8_t column);
  void place_track_in_sysex(uint8_t tracknumber, uint8_t column);
  bool load_track_from_grid(uint8_t column, uint8_t row, int m);
  bool store_track_in_grid(uint8_t track, uint8_t column, uint8_t row);
*/
};

#endif /* EMPTYTRACK_H__ */
