/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef A4TRACK_H__
#define A4TRACK_H__

#include "ExtTrack.h"
// include full MDTrack specification to calculate size
#include "MDTrack.h"
#include "A4.h"
#include "Project.h"
#include "MCLMemory.h"
#include "Bank1Object.h"

class A4Track_270 : public GridTrack_270 {
public:
  ExtSeqTrackData_270 seq_data;
  A4Sound sound;

};


class A4Track : public GridTrack {
public:
  ExtSeqTrackData seq_data;
  A4Sound sound;
  void load_seq_data(int tracknumber);
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  bool store_track_in_grid(int32_t column, int32_t row, int track = 255, bool online = false);
  bool is() {
    return (active == A4_TRACK_TYPE);
  }
  bool convert(A4Track_270 *old) {
    if (active == A4_TRACK_TYPE_270) {
      chain.speed = old->speed;
      chain.length = old->length;
      memcpy(&sound, &(old->sound), sizeof(old->sound));
      seq_data.convert(&(old->seq_data));
      active = A4_TRACK_TYPE;
      return true;
    }
   return false;
  }
};

#endif /* A4TRACK_H__ */
