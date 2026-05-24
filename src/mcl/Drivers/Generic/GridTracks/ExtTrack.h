/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef EXTTRACK_H__
#define EXTTRACK_H__
#include "DeviceTrack.h"
#include "ExtSeqTrack.h"
#include "GridTrack.h"
#include "MCLMemory.h"
#include "DeviceManager.h"
#include "MidiDevice.h"
#include "SeqTrackModData.h"

#define EMPTY_TRACK_TYPE 0

class ATTR_PACKED() ExtTrack : public DeviceTrack {
public:
  SeqTrackModData mod_data;
  ExtSeqTrackData seq_data;
  ExtTrack() {
    active = EXT_TRACK_TYPE;
    static_assert(sizeof(ExtTrack) <= GRID2_TRACK_LEN);
  }
  size_t _sizeof() const {
     return sizeof(ExtTrack) - sizeof(void*);
  }
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override { return true; }
  void load_seq_data(SeqTrack *seq_track) override;
  void transition_load_device(uint8_t tracknumber, SeqTrack *seq_track, GridSlot slotnumber);
  virtual bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  void init(uint8_t tracknumber, SeqTrack *seq_track) override {
    ExtSeqTrack *ext_seq_track = (ExtSeqTrack *)seq_track;
    seq_data.channel = ext_seq_track->channel;
    link.set_speed(SEQ_SPEED_1X);
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;

  uint8_t get_model() override { return EXT_TRACK_TYPE; }
  void init_defaults() override {
    mod_data.init();
    seq_data.clear();
  }
  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t write_size() {
    return DEVICE_TRACK_LEN + sizeof(SeqTrackModData) +
           seq_data.store_size();
  }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  uint16_t get_region_size() override { return GRID2_TRACK_LEN; }
  uint8_t storage_version() const override { return SEQ_TRACK_MICROTIMING_STORAGE_VERSION; }
#if !defined(__AVR__)
  bool can_materialize_as(uint8_t track_type) override;
#endif
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;
  void *get_sound_data_ptr() override { return nullptr; }
  size_t get_sound_data_size() override { return 0; }

  static void load_ext_seq_data(DeviceTrack &track, GridLink &link,
                                ExtSeqTrackData &seq_data,
                                SeqTrackModData &mod_data,
                                SeqTrack *seq_track);
  static constexpr uint16_t seq_payload_storage_offset() {
    return DEVICE_TRACK_LEN;
  }
  static constexpr uint16_t seq_payload_storage_size() {
    return sizeof(SeqTrackModData) + sizeof(ExtSeqTrackData);
  }
#if !defined(__AVR__)
  static bool can_materialize_legacy_ext(uint8_t active,
                                         uint8_t track_type);
  static DeviceTrack *materialize_legacy_ext(DeviceTrack &track,
                                             GridLink &link,
                                             ExtSeqTrackData &seq_data,
                                             SeqTrackModData &mod_data,
                                             uint8_t track_type,
                                             uint8_t tracknumber);
#endif
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

  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false) {};

  uint8_t get_chunk_count() { return (get_seq_data_size() / sizeof(seq_data_chunk)) + 1; }

 virtual uint16_t get_seq_data_size() { return sizeof(ExtSeqTrackData); }
  virtual uint8_t get_model() { return EXT_TRACK_TYPE; }
  virtual uint16_t get_track_size() { return GRID2_TRACK_LEN; }
  virtual uint16_t get_region() { return BANK1_EXT_TRACKS_START; }
  virtual void *get_sound_data_ptr() { return nullptr; }
  virtual size_t get_sound_data_size() { return 0; }
};
*/

class ExtTrackChunk : public DeviceTrackChunk {
public:
  uint16_t get_seq_data_size() override { return sizeof(ExtSeqTrackData); }
  uint8_t get_model() override { return EXT_TRACK_TYPE; }
  uint16_t get_track_size() override { return GRID2_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  void *get_sound_data_ptr() override { return nullptr; }
  size_t get_sound_data_size() override { return 0; }
};
#endif /* EXTTRACK_H__ */
