/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MDSeqTrackData.h"

#define UART1_PORT 1

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

// forward declare MDTrack
class MDTrack;

class MDSeqTrack : public MDSeqTrackData {

public:
  uint8_t track_number;
  uint8_t step_count;
  uint8_t mod12_counter;

  //Conditional counters
  uint8_t iterations_5;
  uint8_t iterations_6;
  uint8_t iterations_7;
  uint8_t iterations_8;

  uint32_t start_clock32th;
  uint64_t oneshot_mask;

  uint8_t port = UART1_PORT;
  MidiUartParent *uart = &MidiUart;

  uint8_t locks_params_orig[4];
  bool load = false;
  //  uint8_t params[24];
  uint8_t trigGroup;
  uint32_t start_step;

  bool mute_until_start = false;

  uint8_t mute_state = SEQ_MUTE_OFF;
  void mute() { mute_state = SEQ_MUTE_ON; }
  void unmute() { mute_state = SEQ_MUTE_OFF; }

  ALWAYS_INLINE() void seq();
  void send_trig();
  ALWAYS_INLINE() void send_trig_inline();
  ALWAYS_INLINE() bool trig_conditional(uint8_t condition);
  ALWAYS_INLINE() void send_parameter_locks(uint8_t step);

  void set_track_pitch(uint8_t step, uint8_t pitch);
  void set_track_step(uint8_t step, uint8_t utiming, uint8_t velocity);
  void set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
  uint8_t get_track_lock(uint8_t step, uint8_t track_param);

  void record_track(uint8_t velocity);
  void record_track_locks(uint8_t track_param, uint8_t value);
  void record_track_pitch(uint8_t pitch);
  void clear_step_locks(uint8_t step);
  void clear_conditional();
  void clear_locks(bool reset_params = true);
  void clear_track(bool locks = true, bool reset_params = true);
  bool is_locks(uint8_t step);
  void clear_param_locks(uint8_t param_id);
  bool is_param(uint8_t param_id);
  void update_kit_params();
  void update_params();
  void update_param(uint8_t param_id, uint8_t value);
  void reset_params();
  void merge_from_md(MDTrack *md_track);

  void set_length(uint8_t len);

  void rotate_left();
  void rotate_right();
  void reverse();

  float get_scale_multiplier(uint8_t scale);
  void set_scale(uint8_t _scale);

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);
};

#endif /* MDSEQTRACK_H__ */
