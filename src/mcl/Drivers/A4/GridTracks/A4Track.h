/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "A4.h"
#include "ExtTrack.h"
#if !defined(__AVR__)
#include "MidiBackedDeviceTrack.h"
#endif

// Use a more specific name to avoid conflict with Arduino's Print class

class ATTR_PACKED() A4Track : public ExtTrack {
public:
  A4Sound sound;
  A4Track() {
    active = A4_TRACK_TYPE;
  }
  size_t _sizeof() const {
    return sizeof(A4Track) - sizeof(void*);
  }
  uint16_t calc_latency(uint8_t tracknumber);
  void transition_send(uint8_t tracknumber, GridSlot slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber);
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) {
    return false;
  }
  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr);
  virtual uint16_t get_track_size() { return _sizeof(); }
  virtual uint8_t get_model() { return A4_TRACK_TYPE; } // TODO
  virtual uint8_t get_parent_model() { return EXT_TRACK_TYPE; }
  virtual bool allow_cast_to_parent() { return true; }
  virtual void *get_sound_data_ptr() { return &sound; }
  virtual size_t get_sound_data_size() { return sizeof(A4Sound); }
#if !defined(__AVR__)
  bool can_materialize_as(uint8_t track_type) override;
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;
#endif
};

#if !defined(__AVR__)
class ATTR_PACKED() A4MidiTrack : public MidiBackedDeviceTrack {
public:
  A4Sound sound;

  A4MidiTrack() {
    active = A4_MIDI_TRACK_TYPE;
  }

  size_t _sizeof() const {
    return sizeof(A4MidiTrack) - sizeof(void *);
  }

  uint16_t calc_latency(uint8_t tracknumber) override;
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override {
    return false;
  }
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;
  uint16_t get_track_size() override { return _sizeof(); }
  uint8_t get_model() override { return A4_TRACK_TYPE; }
  void *get_sound_data_ptr() override { return &sound; }
  size_t get_sound_data_size() override { return sizeof(A4Sound); }

  void import_legacy(const GridLink &old_link,
                     const ExtSeqTrackData &old_seq_data,
                     const SeqTrackModData &old_mod_data,
                     const A4Sound &old_sound,
                     uint8_t tracknumber);
};

static_assert(MEMORY_ALIGN(sizeof(A4MidiTrack) - sizeof(void *)) <=
              GRID2_TRACK_LEN,
              "A4MidiTrack outgrew GRID2_TRACK_LEN");
#endif

#endif /* A4TRACK_H__ */
