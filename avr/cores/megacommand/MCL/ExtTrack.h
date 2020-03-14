/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "ExtSeqTrack.h"
#include "GridTrack.h"

#define EMPTY_TRACK_TYPE 0

class ExtTrack : public GridTrack {
public:
  ExtSeqTrackData seq_data;

  bool get_track_from_sysex(uint8_t tracknumber, uint8_t column);
  bool place_track_in_sysex(uint8_t tracknumber, uint8_t column);
  bool load_track_from_grid(uint8_t column, uint8_t row, int m);
  bool store_track_in_grid(uint8_t track, uint8_t column, uint8_t row, bool online = false);
};

#endif /* EXTTRACK_H__ */
