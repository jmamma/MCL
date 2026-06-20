#pragma once

#include "platform.h"

#if !defined(__AVR__)

#include "DeviceTrack.h"
#include "MidiSeqTrack.h"
#include "Sequencer/SeqTrackModData.h"

class ATTR_PACKED() MidiBackedDeviceTrack : public DeviceTrack {
public:
  TrackLoadFadeData load_fade;

  MidiBackedDeviceTrack();

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
  bool materialized_storage_range(uint8_t track_type,
                                  uint16_t &source_offset,
                                  uint16_t &target_offset,
                                  uint16_t &len) override;
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;

  uint16_t get_region_size() override { return GRID2_TRACK_LEN; }
  uint16_t get_store_size() override {
    return get_track_size();
  }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  uint8_t storage_version() const override { return SEQ_TRACK_LOAD_FADE_STORAGE_VERSION; }
  void init_defaults() override {
    midi_seq_storage().clear_storage();
    load_fade.init();
  }
  TrackLoadFadeData *load_fade_data() override { return &load_fade; }
  const TrackLoadFadeData *load_fade_data() const override {
    return &load_fade;
  }

protected:
  virtual MidiSeqTrackStorage &midi_seq_storage() = 0;
  void import_legacy_ext_storage(const GridLink &old_link,
                                 const ExtSeqTrackData &old_seq_data,
                                 const SeqTrackModData &old_mod_data,
                                 uint8_t tracknumber);
  void store_midi_seq_data(GridSlot column, SeqTrack *seq_track);

private:
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
};

#endif // !defined(__AVR__)
