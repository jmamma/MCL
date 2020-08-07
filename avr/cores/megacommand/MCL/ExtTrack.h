/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "ExtSeqTrack.h"
#include "GridTrack.h"
#include "MCLMemory.h"
#include "Bank1Object.h"

#define EMPTY_TRACK_TYPE 0

class ExtTrack_270 : public GridTrack_270 {
public:
  ExtSeqTrackData_270 seq_data;
};

class ExtTrack
    : public GridTrack,
      public Bank1Object<ExtTrack, NUM_MD_TRACKS, BANK1_A4_TRACKS_START> {
public:
  ExtSeqTrackData seq_data;
  bool load_seq_data(int tracknumber);
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  bool store_track_in_grid(int track, int32_t column, int32_t row,
                           bool online = false);
  void load_immediate(uint8_t tracknumber);
  virtual bool is() {
    return (active == EXT_TRACK_TYPE);
  }
  bool convert(ExtTrack_270 *old) {
    if (active == EXT_TRACK_TYPE_270) {
      chain.speed = old->speed;
      chain.length = old->length;
      seq_data.convert(&(old->seq_data));
      active = EXT_TRACK_TYPE;
    }
    return false;
  }
};

#endif /* EXTTRACK_H__ */
