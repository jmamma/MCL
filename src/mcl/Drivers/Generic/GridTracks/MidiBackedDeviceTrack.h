#pragma once

#include "platform.h"

#if !defined(__AVR__)

#include "DeviceTrack.h"
#include "MidiSeqTrack.h"
#include "SeqTrackModData.h"

class ATTR_PACKED() MidiBackedDeviceTrack : public DeviceTrack {
public:
  MidiSeqTrackStorage seq_data;

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
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;

  uint16_t get_region_size() override { return GRID2_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  uint8_t storage_version() const override { return SEQ_TRACK_MOD_STORAGE_VERSION; }

protected:
  void import_legacy_ext_storage(const GridLink &old_link,
                                 const ExtSeqTrackData &old_seq_data,
                                 const SeqTrackModData &old_mod_data,
                                 uint8_t tracknumber);
  void store_midi_seq_data(GridSlot column, SeqTrack *seq_track);

private:
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
};

#endif // !defined(__AVR__)
