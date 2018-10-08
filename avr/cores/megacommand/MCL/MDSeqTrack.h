/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MDTrack.h"
#include "MDSeqTrackData.h"

#define UART1_PORT 1

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

class MDSeqTrack : public MDSeqTrackData {

public:
  uint8_t track_number;
  uint8_t step_count;
  uint32_t start_clock32th;

  uint8_t port = UART1_PORT;
  MidiUartParent *uart = &MidiUart;

  uint8_t locks_params_orig[4];
  bool load = false;
  uint8_t params[24];
  uint8_t trigGroup;
  bool send_params = false;
  bool mute_until_zero = false;

  uint8_t mute_state = SEQ_MUTE_OFF;
  void seq();
  void mute() { mute_state = SEQ_MUTE_ON; }
  void unmute() { mute_state = SEQ_MUTE_OFF; }

  inline void trig_conditional(uint8_t condition);
  inline void send_parameter_locks(uint8_t step);

  void set_track_pitch(uint8_t step, uint8_t pitch);
  void set_track_step(uint8_t step, uint8_t utiming, uint8_t note_num,
                      uint8_t velocity);
  void set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
  uint8_t get_track_lock(uint8_t step, uint8_t track_param);

  void record_track(uint8_t note_num, uint8_t velocity);
  void record_track_locks(uint8_t track_param, uint8_t value);
  void record_track_pitch(uint8_t pitch);
  void clear_step_locks(uint8_t step);
  void clear_conditional();
  void clear_locks();
  void clear_track(bool locks = true);

  bool is_param(uint8_t param_id);
  void update_params();
  void update_param(uint8_t param_id, uint8_t value);
  void reset_params();
  void merge_from_md(MDTrack *md_track);

  void set_length(uint8_t len);
};

#endif /* MDSEQTRACK_H__ */
