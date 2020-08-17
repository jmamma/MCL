/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "ExtSeqTrack.h"
#include "GridTrack.h"
#include "MCLMemory.h"
#include "GridTrack.h"

#define EMPTY_TRACK_TYPE 0

class ExtTrack_270 : public GridTrack_270 {
public:
  ExtSeqTrackData_270 seq_data;
};

class ExtTrack
    : public GridTrack {
public:
  ExtSeqTrackData seq_data;
  bool load_seq_data(uint8_t tracknumber);
  virtual bool get_track_from_sysex(uint8_t tracknumber);
  bool store_track_in_grid(uint8_t tracknumber, uint16_t row,
                           bool online = false);
  virtual void load_immediate(uint8_t tracknumber);
  bool is() { return (active == EXT_TRACK_TYPE); }
  bool virtual convert(ExtTrack_270 *old) {
    if (active == EXT_TRACK_TYPE_270) {
      chain.speed = old->seq_data.speed;
      chain.length = old->seq_data.length;
      seq_data.convert(&(old->seq_data));
      active = EXT_TRACK_TYPE;
    }
    return false;
  }
  virtual uint16_t get_track_size() { return sizeof(ExtTrack); }
  virtual uint32_t get_region() { return BANK1_A4_TRACKS_START; }
};

#endif /* EXTTRACK_H__ */
