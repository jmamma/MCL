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

class A4Track_270 : public GridTrack {
public:
  ExtSeqTrackData_270 seq_data;
  A4Sound sound;

};


class A4Track : public GridTrack,
                public Bank1Object<A4Track, NUM_MD_TRACKS, BANK1_A4_TRACKS_START> {
public:
  ExtSeqTrackData seq_data;
  A4Sound sound;
  void load_seq_data(int tracknumber);
  bool get_track_from_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m = 0);
  bool store_track_in_grid(int32_t column, int32_t row, int track = 255, bool online = false);
  bool convert(A4Track_270 *old) {
    if (active == A4_TRACK_TYPE_270) {
      memcpy(&sound, &(old->sound), sizeof(old->sound));
      seq_data.convert(&(old->seq_data));
      active = A4_TRACK_TYPE;
      return true;
    }
   return false;
  }
};

#endif /* A4TRACK_H__ */
