/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "ExtSeqTrack.h"
#include "GridTrack.h"

#define EMPTY_TRACK_TYPE 0

class ExtTrack : public GridTrack {
public:
  char kitName[17];

  ExtSeqTrackData seq_data;

  bool get_track_from_sysex(int tracknumber, uint8_t column);
  bool place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
};

#endif /* EXTTRACK_H__ */
