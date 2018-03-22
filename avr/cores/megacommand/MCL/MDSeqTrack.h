/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MidiActivePeering.h"

class MDSeqTrackData {
public:
  uint8_t length;
  uint8_t locks[4][64];
  uint8_t locks_params[4];
  uint64_t pattern_masks;
  uint64_t lock_masks;
  uint8_t conditional[64];
  uint8_t timing[64];
};

class MDSeqTrack : MDSeqTrackData {

public:
  uint8_t track_number;
  uint8_t port = UART1_PORT;
  MidiUartParent *uart = &MidiUart;

  void seq();
  void trig_conditional();
  void send_parameter_locks(uint8_t i, uint8_t step_count);

  void set_track_pitch(uint8_t track, uint8_t pitch);
  void set_track_step(uint8_t track, uint8_t step, uint8_t utiming,
                      uint8_t note_num, uint8_t velocity);

  void record_track(uint8_t track, uint8_t note_num, uint8_t velocity);
  void record_track_locks(uint8_t track, uint8_t track_param, uint8_t value);
  void clear_seq_conditional();
  void clear_seq_locks();
  void clear_seq_track();
};

#endif /* MDSEQTRACK_H__ */
