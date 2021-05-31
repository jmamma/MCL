/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "A4.h"
#include "ExtTrack.h"

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
  uint16_t calc_latency(uint8_t tracknumber);
  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber);
  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  bool get_track_from_sysex(uint8_t tracknumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false);
  bool convert(A4Track_270 *old) {
    link.row = old->link.row;
    link.loops = old->link.loops;
    if (link.row >= GRID_LENGTH) {
      link.row = GRID_LENGTH - 1;
    }

    if (old->active == A4_TRACK_TYPE_270) {
      link.speed = old->seq_data.speed;
      if (old->seq_data.speed == 0) {
        link.speed = SEQ_SPEED_2X;
      } else {
        link.speed = old->seq_data.speed - 1;
        if (link.speed == 0) {
          link.speed = SEQ_SPEED_2X;
        } else if (link.speed == 1) {
          link.speed = SEQ_SPEED_1X;
        }
      }

      link.length = old->seq_data.length;
      if (link.length == 0) {
        link.length = 16;
      }

      sound.convert(&old->sound);
      seq_data.convert(&old->seq_data);
      active = A4_TRACK_TYPE;
    } else {
      link.speed = SEQ_SPEED_1X;
      link.length = 16;
      active = EMPTY_TRACK_TYPE;
    }

    return true;
  }
  virtual uint16_t get_track_size() { return sizeof(A4Track); }
  virtual uint8_t get_model() { return A4_TRACK_TYPE; } // TODO
  virtual uint8_t get_device_type() { return A4_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return &sound; }
  virtual size_t get_sound_data_size() { return sizeof(A4Sound); }
};

#endif /* A4TRACK_H__ */
