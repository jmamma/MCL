/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "DeviceTrack.h"
#include "ExtSeqTrack.h"
#include "GridTrack.h"
#include "MCLMemory.h"

#define EMPTY_TRACK_TYPE 0

class ExtTrack_270 : public GridTrack_270 {
public:
  ExtSeqTrackData_270 seq_data;
};

class ExtTrack : public DeviceTrack {
public:
  ExtSeqTrackData seq_data;
  ExtTrack() {
    active = EXT_TRACK_TYPE;
    static_assert(sizeof(ExtTrack) <= GRID2_TRACK_LEN);
  }
  virtual void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                               uint8_t slotnumber);

  bool load_seq_data(SeqTrack *seq_track);
  virtual bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false);

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {
    ExtSeqTrack *ext_seq_track = (ExtSeqTrack *)seq_track;
    seq_data.channel = ext_seq_track->channel;
    link.speed = SEQ_SPEED_1X;
  }
  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);

  bool virtual convert(ExtTrack_270 *old) {
    link.row = old->link.row;
    link.loops = old->link.loops;
    if (link.row >= GRID_LENGTH) {
      link.row = GRID_LENGTH - 1;
    }

    if (old->active == EXT_TRACK_TYPE_270) {
      if (old->seq_data.speed == 0) {
        link.speed = SEQ_SPEED_2X;
      } else {
        link.speed = old->seq_data.speed - 1;
        if (link.speed == 0) {
          link.speed = SEQ_SPEED_2X;
        } else if (link.speed == 1) {
          link.speed = SEQ_SPEED_1X;
        }
      }
      link.length = old->seq_data.length;
      if (link.length == 0) {
        link.length = 16;
      }
      seq_data.convert(&(old->seq_data));
      active = EXT_TRACK_TYPE;
    } else {
      link.speed = SEQ_SPEED_1X;
      link.length = 16;
      active = EMPTY_TRACK_TYPE;
    }
    return true;
  }
  virtual uint8_t get_model() { return EXT_TRACK_TYPE; }
  virtual uint16_t get_track_size() { return GRID2_TRACK_LEN; }
  virtual uint32_t get_region() { return BANK1_A4_TRACKS_START; }
  virtual uint8_t get_device_type() { return EXT_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }
};

class ExtTrackSmall {
public:
  uint8_t buf[sizeof(ExtSeqTrackData) / 4];
  bool load_from_mem_chunk(uint8_t column, uint8_t chunk) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column) + sizeof(ExtTrackSmall) * chunk;
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(this, ptr, sizeof(ExtTrackSmall));
    return true;
  }
  bool load_chunk(volatile void *ptr, uint8_t chunk) {
    memcpy(ptr + sizeof(ExtTrackSmall) * chunk, buf, sizeof(ExtTrackSmall));
    return true;
  }
  uint16_t get_track_size() { return GRID2_TRACK_LEN; }
  uint32_t get_region() { return BANK1_A4_TRACKS_START; }

};
#endif /* EXTTRACK_H__ */
