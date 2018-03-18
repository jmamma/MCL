/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

class MDSeqTrack {

public:
  uint8_t track_number;
  uint8_t port = UART1_PORT;
  MidiuUart *uart = &MidiUart;
  uint8_t length;
  uint8_t locks[4][64];
  uint8_t locks_params[4];
  uint64_t pattern_masks;
  uint64_t lock_masks;
  uint8_t conditional[64];
  uint8_t timing[64];
};

#endif /* MDSEQTRACK_H__ */
