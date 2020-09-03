/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "ExtTrack.h"
#include "A4.h"
#include "DiagnosticPage.h"

class A4Track_270 : public GridTrack_270 {
public:
  ExtSeqTrackData_270 seq_data;
  A4Sound_270 sound;
};


class A4Track : public ExtTrack {
public:
  A4Sound sound;
  A4Track() {
    active = A4_TRACK_TYPE;
    static_assert(sizeof(A4Track) <= GRID2_TRACK_LEN);
  }
  bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge, bool online = false);
  bool convert(A4Track_270 *old) {
    if (active == A4_TRACK_TYPE_270) {
      chain.speed = old->seq_data.speed;
      chain.length = old->seq_data.length;
      sound.convert(&old->sound);
      seq_data.convert(&old->seq_data);
      active = A4_TRACK_TYPE;
      return true;
    }
   return false;
  }
  virtual uint16_t get_track_size() { return sizeof(A4Track); }
  virtual uint8_t get_model() { return A4_TRACK_TYPE; } // TODO
  virtual uint8_t get_device_type() { return A4_TRACK_TYPE; }

  virtual void* get_sound_data_ptr() { return &sound; }
  virtual size_t get_sound_data_size() { return sizeof(A4Sound); }
};

#endif /* A4TRACK_H__ */
