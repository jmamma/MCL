/* Yatao Li yatao.li@live.com 2020 */

#pragma once

#include "ExtTrack.h"
#include "MNM.h"
#if !defined(__AVR__)
#include "MidiBackedDeviceTrack.h"
#endif

class ATTR_PACKED() MNMTrack : public ExtTrack {
public:
  MNMMachine machine;
  MNMTrack() {
    active = MNM_TRACK_TYPE;
    static_assert(sizeof(MNMTrack) <= GRID2_TRACK_LEN);
  }
  size_t _sizeof() const {
     return sizeof(MNMTrack) - sizeof(void*);
  }
  void init();
  uint16_t calc_latency(uint8_t tracknumber) override;

  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override { return false; }

  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  virtual void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;
  void get_machine_from_kit(uint8_t tracknumber);
  virtual bool store_in_grid(GridSlot column, GridRow row,
                             SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                             bool online = false, Grid *grid = nullptr) override;
  virtual uint16_t get_track_size() override { return _sizeof(); }
  uint16_t grid_slot_label(GridSlotLabelContext ctx) override;
  virtual uint8_t get_model() override { return machine.model; }
  virtual void *get_sound_data_ptr() override { return &machine; }
  virtual size_t get_sound_data_size() override { return sizeof(MNMMachine); }
#if !defined(__AVR__)
  bool can_materialize_as(uint8_t track_type) override;
#endif
  bool materialized_storage_range(uint8_t track_type,
                                  uint16_t &source_offset,
                                  uint16_t &target_offset,
                                  uint16_t &len) override;
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;

};

#if !defined(__AVR__)
class ATTR_PACKED() MNMMidiTrack : public MidiBackedDeviceTrack {
public:
  MNMMachine machine;
  MidiSeqTrackStorage seq_data;

  MNMMidiTrack() {
    active = MNM_MIDI_TRACK_TYPE;
  }

  size_t _sizeof() const {
     return sizeof(MNMMidiTrack) - sizeof(void *);
  }

  void init();
  uint16_t calc_latency(uint8_t tracknumber) override;

  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override {
    return false;
  }

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;
  void get_machine_from_kit(uint8_t tracknumber);
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;
  uint16_t get_track_size() override { return _sizeof(); }
  uint16_t grid_slot_label(GridSlotLabelContext ctx) override;
  uint8_t get_model() override { return machine.model; }
  void *get_sound_data_ptr() override { return &machine; }
  size_t get_sound_data_size() override { return sizeof(MNMMachine); }

  void import_legacy(const GridLink &old_link,
                     const ExtSeqTrackData &old_seq_data,
                     const SeqTrackModData &old_mod_data,
                     const MNMMachine &old_machine,
                     const TrackLoadFadeData &old_load_fade,
                     uint8_t tracknumber);

protected:
  MidiSeqTrackStorage &midi_seq_storage() override { return seq_data; }
};

static_assert(MEMORY_ALIGN(sizeof(MNMMidiTrack) - sizeof(void *)) <=
              GRID2_TRACK_LEN,
              "MNMMidiTrack outgrew GRID2_TRACK_LEN");
#endif
