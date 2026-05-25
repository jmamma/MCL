#pragma once

#include "platform.h"

#if !defined(__AVR__)

#include "DeviceTrack.h"
#include "MidiSeqTrack.h"
#include "SeqTrackModData.h"

class ATTR_PACKED() MidiTrack : public DeviceTrack {
public:
  MidiSeqTrackStorage seq_data;

  MidiTrack();

  void init(uint8_t tracknumber, SeqTrack *seq_track) override;
  uint8_t transition_countdown_resolution() override;
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override {
    (void)tracknumber;
    (void)slotnumber;
    return false;
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_seq_data(SeqTrack *seq_track) override;
  bool can_materialize_as(uint8_t track_type) override;
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t get_store_size() override {
    return get_track_size();
  }
  uint16_t get_region_size() override { return GRID2_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  uint16_t grid_slot_label(GridSlotLabelContext ctx) override {
    return make_grid_slot_label('M', ctx.column + '1');
  }
  uint8_t get_model() override { return MIDI_TRACK_TYPE; }
  uint8_t storage_version() const override { return SEQ_TRACK_MOD_STORAGE_VERSION; }
  void init_defaults() override { seq_data.clear_storage(); }
  void *get_sound_data_ptr() override { return nullptr; }
  size_t get_sound_data_size() override { return 0; }

  size_t _sizeof() const { return sizeof(MidiTrack) - sizeof(void *); }

private:
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
};

#endif // !defined(__AVR__)
