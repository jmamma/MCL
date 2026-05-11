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
void tbd_update_track_default_from_p4(uint8_t p4_track_index,
                                      const char *preset_id,
                                      uint8_t rom_bank,
                                      int32_t sample_slice);
void tbd_mark_p4_sound_applied(const TbdP4SoundData &sound);

class ATTR_PACKED() TBDTrack : public DeviceTrack {
public:
  TbdP4SoundData p4_sound;
  TBDSeqTrackData seq_data;
  SeqTrackModData mod_data;

  TBDTrack();

  void init(uint8_t tracknumber, SeqTrack *seq_track) override;
  uint16_t calc_latency(uint8_t tracknumber) override;
  uint8_t transition_countdown_resolution() override {
    return STEPSEQ_SEQ_INTERPOLATION;
  }
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber) override;
  void transition_send(uint8_t tracknumber, uint8_t slotnumber) override;
  bool transition_cache(uint8_t tracknumber, uint8_t slotnumber) override {
    return false;
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_seq_data(SeqTrack *seq_track) override;
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t get_region_size() override { return TBD_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_TBD_TRACKS_START; }
  uint8_t get_model() override { return p4_sound.p4_track_index; }
  uint8_t get_device_type() override { return TBD_TRACK_TYPE; }
  uint8_t storage_version() const override { return SEQ_TRACK_MOD_STORAGE_VERSION; }
  void *get_sound_data_ptr() override { return &p4_sound; }
  size_t get_sound_data_size() override { return sizeof(TbdP4SoundData); }

  size_t _sizeof() const { return sizeof(TBDTrack) - sizeof(void *); }

private:
  void apply_preset(uint8_t fallback_tracknumber, const char *source,
                    uint8_t slotnumber);
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
  void load_arp_data(SeqTrack *seq_track);
  void load_lfo_data(SeqTrack *seq_track);
};

class ATTR_PACKED() TBDMidiTrack : public DeviceTrack {
public:
  TbdP4SoundData p4_sound;
  MidiSeqTrackData seq_data;
  SeqTrackModData mod_data;

  TBDMidiTrack();

  void init(uint8_t tracknumber, SeqTrack *seq_track) override;
  uint16_t calc_latency(uint8_t tracknumber) override;
  uint8_t transition_countdown_resolution() override {
    return STEPSEQ_SEQ_INTERPOLATION;
  }
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber) override;
  void transition_send(uint8_t tracknumber, uint8_t slotnumber) override;
  bool transition_cache(uint8_t tracknumber, uint8_t slotnumber) override {
    return false;
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_seq_data(SeqTrack *seq_track) override;
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t get_region_size() override { return GRID2_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_EXT_TRACKS_START; }
  uint8_t get_model() override { return p4_sound.p4_track_index; }
  uint8_t get_device_type() override { return TBD_MIDI_TRACK_TYPE; }
  uint8_t storage_version() const override { return SEQ_TRACK_MOD_STORAGE_VERSION; }
  void *get_sound_data_ptr() override { return &p4_sound; }
  size_t get_sound_data_size() override { return sizeof(TbdP4SoundData); }

  size_t _sizeof() const { return sizeof(TBDMidiTrack) - sizeof(void *); }

private:
  void apply_preset(uint8_t fallback_tracknumber, const char *source,
                    uint8_t slotnumber);
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
  void load_arp_data(SeqTrack *seq_track);
  void load_lfo_data(SeqTrack *seq_track);
};

#endif // PLATFORM_TBD
