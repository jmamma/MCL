#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include "ExtTrack.h"

#define TBD_PRESET_ID_LEN 32

struct TbdTrackDefault {
  uint8_t p4_track_index;
  uint8_t midi_channel;
  const char *preset_id;
};

const TbdTrackDefault &tbd_track_default_for_slot(uint8_t slot);

class ATTR_PACKED() TBDTrackData {
public:
  uint8_t version;
  uint8_t p4_track_index;
  uint8_t midi_channel;
  uint8_t rom_bank;
  int32_t sample_slice;
  char preset_id[TBD_PRESET_ID_LEN];

  void clear();
  void set_default(uint8_t slot);
  bool has_preset() const { return preset_id[0] != '\0'; }
};

class ATTR_PACKED() TBDTrack : public ExtTrack {
public:
  TBDTrackData p4_preset;

  TBDTrack();

  void init(uint8_t tracknumber, SeqTrack *seq_track) override;
  uint16_t calc_latency(uint8_t tracknumber) override;
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber) override;
  void transition_send(uint8_t tracknumber, uint8_t slotnumber) override;
  bool transition_cache(uint8_t tracknumber, uint8_t slotnumber) override {
    return false;
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;

  uint16_t get_track_size() override { return _sizeof(); }
  uint8_t get_model() override { return p4_preset.p4_track_index; }
  uint8_t get_device_type() override { return TBD_TRACK_TYPE; }
  uint8_t get_parent_model() override { return EXT_TRACK_TYPE; }
  bool allow_cast_to_parent() override { return true; }
  void *get_sound_data_ptr() override { return &p4_preset; }
  size_t get_sound_data_size() override { return sizeof(TBDTrackData); }

  size_t _sizeof() const { return sizeof(TBDTrack) - sizeof(void *); }

private:
  void apply_preset(uint8_t fallback_tracknumber);
  void apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track);
};

#endif // PLATFORM_TBD
