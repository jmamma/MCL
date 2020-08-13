/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "ExtTrack.h"
// include full MDTrack specification to calculate size
#include "MDTrack.h"
#include "A4.h"
#include "MCLMemory.h"

class A4Track_270 : public GridTrack_270 {
public:
  ExtSeqTrackData_270 seq_data;
  A4Sound sound;

};


class A4Track : public ExtTrack {
public:
  A4Sound sound;
  bool get_track_from_sysex(uint8_t tracknumber);
  bool store_track_in_grid(uint8_t tracknumber, uint16_t row, bool online = false);
  bool is() {
    return (active == A4_TRACK_TYPE);
  }
  bool convert(A4Track_270 *old) {
    if (active == A4_TRACK_TYPE_270) {
      chain.speed = old->seq_data.speed;
      chain.length = old->seq_data.length;
      memcpy(&sound, &(old->sound), sizeof(old->sound));
      seq_data.convert(&(old->seq_data));
      active = A4_TRACK_TYPE;
      return true;
    }
   return false;
  }
};

#endif /* A4TRACK_H__ */
