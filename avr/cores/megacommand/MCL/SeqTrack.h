/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQTRACK_H__
#define SEQTRACK_H__

#include "MidiUartParent.hh"
#include "WProgram.h"

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define SEQ_SCALE_1X 0
#define SEQ_SCALE_2X 1
#define SEQ_SCALE_3_4X 2
#define SEQ_SCALE_3_2X 3
#define SEQ_SCALE_1_2X 4
#define SEQ_SCALE_1_4X 5
#define SEQ_SCALE_1_8X 6

class SeqTrack {

public:
  ALWAYS_INLINE() virtual uint8_t get_timing_mid(uint8_t scale) {
    uint8_t timing_mid;
    switch (scale) {
    default:
    case SEQ_SCALE_1X:
      timing_mid = 12;
      break;
    case SEQ_SCALE_2X:
      timing_mid = 6;
      break;
    case SEQ_SCALE_3_4X:
      timing_mid = 16; // 12 * (4.0/3.0);
      break;
    case SEQ_SCALE_3_2X:
      timing_mid = 8; // 12 * (2.0/3.0);
      break;
    case SEQ_SCALE_1_2X:
      timing_mid = 24;
      break;
    case SEQ_SCALE_1_4X:
      timing_mid = 48;
      break;
    case SEQ_SCALE_1_8X:
      timing_mid = 96;
      break;
    }
    return timing_mid;
  }
};

#endif /* SEQTRACK_H__ */
