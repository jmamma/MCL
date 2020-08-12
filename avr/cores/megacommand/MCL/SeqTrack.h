/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQTRACK_H__
#define SEQTRACK_H__

#include "MidiUartParent.hh"
#include "WProgram.h"
#include "MidiActivePeering.h"

#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

#define SEQ_SPEED_1X 0
#define SEQ_SPEED_2X 1
#define SEQ_SPEED_3_4X 2
#define SEQ_SPEED_3_2X 3
#define SEQ_SPEED_1_2X 4
#define SEQ_SPEED_1_4X 5
#define SEQ_SPEED_1_8X 6

class SeqTrack_270 {};

class SeqTrack {

public:
  uint8_t active;
  uint8_t length;
  uint8_t speed;
  uint8_t track_number;
  uint8_t step_count;
  uint8_t mod12_counter;

  // Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  uint8_t port = UART1_PORT;
  MidiUartParent *uart = &MidiUart;

  ALWAYS_INLINE() virtual uint8_t get_timing_mid(uint8_t speed) {
    uint8_t timing_mid;
    switch (speed) {
    default:
    case SEQ_SPEED_1X:
      timing_mid = 12;
      break;
    case SEQ_SPEED_2X:
      timing_mid = 6;
      break;
    case SEQ_SPEED_3_4X:
      timing_mid = 16; // 12 * (4.0/3.0);
      break;
    case SEQ_SPEED_3_2X:
      timing_mid = 8; // 12 * (2.0/3.0);
      break;
    case SEQ_SPEED_1_2X:
      timing_mid = 24;
      break;
    case SEQ_SPEED_1_4X:
      timing_mid = 48;
      break;
    case SEQ_SPEED_1_8X:
      timing_mid = 96;
      break;
    }
    return timing_mid;
  }

  virtual float set_speed_multiplier() { return get_speed_multiplier(speed); }

  virtual float get_speed_multiplier(uint8_t speed) {
    float multi;
    switch (speed) {
    default:
    case SEQ_SPEED_1X:
      multi = 1;
      break;
    case SEQ_SPEED_2X:
      multi = 0.5;
      break;
    case SEQ_SPEED_3_4X:
      multi = (4.0 / 3.0);
      break;
    case SEQ_SPEED_3_2X:
      multi = (2.0 / 3.0);
      break;
    case SEQ_SPEED_1_2X:
      multi = 2.0;
      break;
    case SEQ_SPEED_1_4X:
      multi = 4.0;
      break;
    case SEQ_SPEED_1_8X:
      multi = 8.0;
      break;
    }
    return multi;
  }
};

#endif /* SEQTRACK_H__ */
