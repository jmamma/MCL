/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "DeviceTrack.h"
#include "ExtSeqTrack.h"
#include "GridTrack.h"
#include "MCLMemory.h"
#include "MidiActivePeering.h"

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
  virtual bool transition_cache(uint8_t tracknumber, uint8_t slotnumber) { return true; }
  void load_seq_data(SeqTrack *seq_track);
  virtual bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr);

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {
    ExtSeqTrack *ext_seq_track = (ExtSeqTrack *)seq_track;
    seq_data.channel = ext_seq_track->channel;
    link.speed = SEQ_SPEED_1X;
  }
  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);

  bool convert(ExtTrack_270 *old) {
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
  virtual uint16_t get_track_size() { return sizeof(ExtTrack); }
  virtual uint16_t get_region() { return BANK1_A4_TRACKS_START; }
  virtual uint16_t get_region_size() { return GRID2_TRACK_LEN; }
  virtual uint8_t get_device_type() { return EXT_TRACK_TYPE; }
  virtual uint8_t get_parent_model() { return midi_active_peering.get_device(UART2_PORT)->track_type; }
  virtual void *get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }
};
/*
class ExtTrackChunk : public DeviceTrack {
  public:
  ExtTrackChunk() {
  }

  uint8_t seq_data_chunk[256];

  bool load_from_mem_chunk(uint8_t column, uint8_t chunk) {
    size_t chunk_size = sizeof(seq_data_chunk);
    uint32_t offset = (uint32_t) seq_data_chunk - (uint32_t) this;
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column) + offset + chunk_size * chunk;
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(seq_data_chunk, ptr, chunk_size);
    return true;
  }
  bool load_chunk(volatile void *ptr, uint8_t chunk) {
    size_t chunk_size = sizeof(seq_data_chunk);
    if (chunk == get_chunk_count() - 1) { chunk_size = get_seq_data_size() - sizeof(seq_data_chunk) * chunk; }
    memcpy(ptr + sizeof(seq_data_chunk) * chunk, seq_data_chunk, chunk_size);
    return true;
  }

  bool load_link_from_mem(uint8_t column) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(this, ptr, sizeof(GridTrack));
    return true;
  }

  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false) {};

  uint8_t get_chunk_count() { return (get_seq_data_size() / sizeof(seq_data_chunk)) + 1; }

 virtual uint16_t get_seq_data_size() { return sizeof(ExtSeqTrackData); }
  virtual uint8_t get_model() { return EXT_TRACK_TYPE; }
  virtual uint16_t get_track_size() { return GRID2_TRACK_LEN; }
  virtual uint16_t get_region() { return BANK1_A4_TRACKS_START; }
  virtual uint8_t get_device_type() { return EXT_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }
};
*/

class ExtTrackChunk : public DeviceTrackChunk {
public:
  virtual uint16_t get_seq_data_size() { return sizeof(ExtSeqTrackData); }
  virtual uint8_t get_model() { return EXT_TRACK_TYPE; }
  virtual uint16_t get_track_size() { return GRID2_TRACK_LEN; }
  virtual uint16_t get_region() { return BANK1_A4_TRACKS_START; }
  virtual uint8_t get_device_type() { return EXT_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }
};
#endif /* EXTTRACK_H__ */
