/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "A4.h"
#include "ExtTrack.h"

// Use a more specific name to avoid conflict with Arduino's Print class

class ATTR_PACKED() A4Track : public ExtTrack {
public:
  A4Sound sound;
  A4Track() {
    active = A4_TRACK_TYPE;
    static_assert(sizeof(A4Track) <= GRID2_TRACK_LEN);
  }
  uint16_t calc_latency(uint8_t tracknumber);
  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber);
  bool transition_cache(uint8_t tracknumber, uint8_t slotnumber) {
    return false;
  }
  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr);
  virtual uint16_t get_track_size() { return sizeof(A4Track); }
  virtual uint8_t get_model() { return A4_TRACK_TYPE; } // TODO
  virtual uint8_t get_device_type() { return A4_TRACK_TYPE; }
  virtual uint8_t get_parent_model() { return EXT_TRACK_TYPE; }
  virtual bool allow_cast_to_parent() { return true; }
  virtual void *get_sound_data_ptr() { return &sound; }
  virtual size_t get_sound_data_size() { return sizeof(A4Sound); }
};

#endif /* A4TRACK_H__ */
