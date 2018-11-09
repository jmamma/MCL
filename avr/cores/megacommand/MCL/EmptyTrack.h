/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EMPTYTRACK_H__
#define EMPTYTRACK_H__

#include "Grid.h"
#include "GridTrack.h"

class EmptyTrack : public GridTrack {
public:
  //Assume A4Track consume most data size out
  //of all the tracktypes

  uint8_t data[sizeof(A4Track)];
        /*
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  void place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
*/
};

#endif /* EMPTYTRACK_H__ */
