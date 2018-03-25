/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#define UART1_PORT 1

class MDSeqTrackData {
public:
  uint8_t length;
  uint8_t locks[4][64];
  uint8_t locks_params[4];
  uint64_t pattern_mask;
  uint64_t lock_mask;
  uint8_t conditional[64];
  uint8_t timing[64];
};

class MDSeqTrack : public MDSeqTrackData {

public:
  uint8_t track_number;
  uint8_t port = UART1_PORT;
  MidiUartParent *uart = &MidiUart;

  void seq();
  void trig_conditional(uint8_t condition);
  void send_parameter_locks(uint8_t step_count);

  void set_track_pitch(uint8_t step, uint8_t pitch);
  void set_track_step(uint8_t step, uint8_t utiming,
                      uint8_t note_num, uint8_t velocity);
  void set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
  void record_track(uint8_t note_num, uint8_t velocity);
  void record_track_locks(uint8_t track_param, uint8_t value);
  void record_track_pitch(uint8_t pitch);
  void clear_seq_conditional();
  void clear_seq_locks();
  void clear_seq_track();
};

#endif /* MDSEQTRACK_H__ */
