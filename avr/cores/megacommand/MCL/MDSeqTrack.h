/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MDSeqTrackData.h"
#include "SeqTrack.h"
#include "MD.h"

#define UART1_PORT 1

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

class MDTrack;

class SlideData {
public:
  int16_t err;
  int8_t inc;
  int16_t dx;
  int16_t dy;
  int16_t x0;
  int8_t y0;
  int8_t y1;
  bool steep;
  int16_t x1;
  uint8_t yflip;
  ALWAYS_INLINE() void init() {
    dy = 0;
    dx = 0;
  }
};

class MDSeqTrack : public MDSeqTrackData, public SeqTrack {

private:
  FORCED_INLINE() uint8_t find_param(uint8_t param_id) const;
  FORCED_INLINE() uint16_t get_lockidx(uint8_t step) const;
  FORCED_INLINE() uint8_t get_lockidx(uint8_t step, uint8_t lock_idx) const;

public:
  uint64_t oneshot_mask;

  uint8_t locks_params_orig[NUM_MD_LOCKS];
  SlideData locks_slide_data[NUM_MD_LOCKS];
  uint8_t locks_slide_next_lock_idx[NUM_MD_LOCKS];
  uint8_t locks_slide_next_lock_step[NUM_MD_LOCKS];

  ALWAYS_INLINE() void reset() {
  SeqTrack::reset();
  oneshot_mask = 0;
  }

  void seq();

  void mute() { mute_state = SEQ_MUTE_ON; }
  void unmute() { mute_state = SEQ_MUTE_OFF; }

  void send_trig();
  ALWAYS_INLINE() void send_trig_inline();
  ALWAYS_INLINE() bool trig_conditional(uint8_t condition);
  ALWAYS_INLINE() void send_parameter_locks(uint8_t step, bool pattern_mask_step);

  ALWAYS_INLINE() void send_slides();
  ALWAYS_INLINE() void recalc_slides();
  ALWAYS_INLINE() void find_next_locks(uint8_t curidx, uint8_t step, uint8_t mask);

  void set_track_pitch(uint8_t step, uint8_t pitch);
  void set_track_step(uint8_t step, uint8_t utiming, uint8_t velocity);
  bool set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
  uint8_t get_track_lock(uint8_t step, uint8_t track_param);

  void record_track(uint8_t velocity);
  void record_track_locks(uint8_t track_param, uint8_t value);
  void record_track_pitch(uint8_t pitch);
  void clear_step_locks(uint8_t step);
  void clear_conditional();
  void clear_locks(bool reset_params = true);
  void clear_track(bool locks = true, bool reset_params = true);
  void clear_param_locks(uint8_t param_id);
  bool is_param(uint8_t param_id);
  void update_kit_params();
  void update_params();
  void update_param(uint8_t param_id, uint8_t value);
  void reset_params();
  void merge_from_md(uint8_t track_number, MDPattern *pattern);

  void set_length(uint8_t len);
  void re_sync();

  #define DIR_LEFT 0
  #define DIR_RIGHT 1
  #define DIR_REVERSE 2

  void rotate_left() { modify_track(DIR_LEFT); }
  void rotate_right() { modify_track(DIR_RIGHT); }
  void reverse() { modify_track(DIR_REVERSE); }

  void modify_track(uint8_t dir);

  void set_speed(uint8_t _speed);

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);

};

#endif /* MDSEQTRACK_H__ */
