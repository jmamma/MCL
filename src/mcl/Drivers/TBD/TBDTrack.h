#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "DeviceTrack.h"
#include "MidiSeqTrack.h"
#include "SeqTrackModData.h"
#include "TBDSeqTrack.h"
#include "TbdP4SoundData.h"

#define TBD_PRESET_ID_LEN TBD_P4_ID_LEN

struct TbdTrackDefault {
  uint8_t p4_track_index;
  uint8_t midi_channel;
  char preset_id[TBD_PRESET_ID_LEN];
  uint8_t rom_bank;
  int32_t sample_slice;
};

const TbdTrackDefault &tbd_track_default_for_slot(uint8_t slot);
void tbd_set_step_sound_default(TbdP4SoundData &sound, uint8_t slot);
void tbd_set_midi_sound_default(TbdP4SoundData &sound, uint8_t slot);
void tbd_ensure_step_sound_default(TbdP4SoundData &sound, uint8_t slot);
void tbd_ensure_midi_sound_default(TbdP4SoundData &sound, uint8_t slot);
void tbd_init_p4_sound_runtime_defaults(TbdP4SoundData &sound);
TbdP4SoundData *tbd_midi_runtime_sound(uint8_t track);
const TbdP4SoundData *tbd_midi_runtime_sound_const(uint8_t track);
void tbd_update_track_default_from_p4(uint8_t p4_track_index,
                                      const char *preset_id,
                                      uint8_t rom_bank,
                                      int32_t sample_slice);
void tbd_mark_p4_sound_applied(const TbdP4SoundData &sound);

class ATTR_PACKED() TBDTrack : public DeviceTrack {
public:
  TbdP4SoundData p4_sound;
  StepSeqTrackStorage seq_data;

  TBDTrack();

  void init(uint8_t tracknumber, SeqTrack *seq_track) override;
  uint16_t calc_latency(uint8_t tracknumber) override;
  uint8_t transition_countdown_resolution() override {
    return STEPSEQ_SEQ_INTERPOLATION;
  }
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override {
    return false;
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_seq_data(SeqTrack *seq_track) override;
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t get_store_size() override {
    return reinterpret_cast<uintptr_t>(&seq_data) -
           reinterpret_cast<uintptr_t>(_this()) + seq_data.store_size();
  }
  uint16_t get_region_size() override { return TBD_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_TBD_TRACKS_START; }
  bool copy_grid_slot_label(const GridSlotLabelContext &ctx,
                            char label[3]) override;
  uint8_t get_model() override { return p4_sound.p4_track_index; }
  uint8_t storage_version() const override { return SEQ_TRACK_MOD_STORAGE_VERSION; }
  void init_defaults() override {
    tbd_init_p4_sound_runtime_defaults(p4_sound);
    seq_data.init_storage();
  }
  void *get_sound_data_ptr() override { return &p4_sound; }
  size_t get_sound_data_size() override { return sizeof(TbdP4SoundData); }

  size_t _sizeof() const { return sizeof(TBDTrack) - sizeof(void *); }

private:
  void apply_preset(uint8_t fallback_tracknumber, const char *source,
                    GridSlot slotnumber);
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
};

class ATTR_PACKED() TBDMidiTrack : public DeviceTrack {
public:
  TbdP4SoundData p4_sound;
  MidiSeqTrackStorage seq_data;

  TBDMidiTrack();

  void init(uint8_t tracknumber, SeqTrack *seq_track) override;
  uint16_t calc_latency(uint8_t tracknumber) override;
  uint8_t transition_countdown_resolution() override {
    return STEPSEQ_SEQ_INTERPOLATION;
  }
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override {
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
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t get_store_size() override {
    return reinterpret_cast<uintptr_t>(&seq_data) -
           reinterpret_cast<uintptr_t>(_this()) + seq_data.store_size();
  }
  uint16_t get_region_size() override { return GRID2_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  bool copy_grid_slot_label(const GridSlotLabelContext &ctx,
                            char label[3]) override;
  uint8_t get_model() override { return p4_sound.p4_track_index; }
  uint8_t storage_version() const override { return SEQ_TRACK_MOD_STORAGE_VERSION; }
  void init_defaults() override {
    tbd_init_p4_sound_runtime_defaults(p4_sound);
    seq_data.clear_storage();
  }
  void *get_sound_data_ptr() override { return &p4_sound; }
  size_t get_sound_data_size() override { return sizeof(TbdP4SoundData); }

  size_t _sizeof() const { return sizeof(TBDMidiTrack) - sizeof(void *); }

private:
  void apply_preset(uint8_t fallback_tracknumber, const char *source,
                    GridSlot slotnumber);
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
};

#endif // PLATFORM_TBD
