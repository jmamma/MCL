/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MD.h"
#include "MDSeqTrackData.h"
#include "SeqTrack.h"

#define UART1_PORT 1

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

class MDTrack;

class MDSeqTrack : public MDSeqTrackData, public SeqSlideTrack {

public:
  uint64_t oneshot_mask;
  uint8_t locks_params_orig[NUM_LOCKS];

  MDSeqTrack() : SeqSlideTrack() { active = MD_TRACK_TYPE; }
  ALWAYS_INLINE() void reset() {
    SeqTrack::reset();
    oneshot_mask = 0;
  }

  void get_mask(uint64_t *_pmask, uint8_t mask_type) const;

  bool get_step(uint8_t step, uint8_t mask_type) const;
  void set_step(uint8_t step, uint8_t mask_type, bool val);

  void seq();

  void mute() { mute_state = SEQ_MUTE_ON; }
  void unmute() { mute_state = SEQ_MUTE_OFF; }

  void send_trig();
  ALWAYS_INLINE() void send_trig_inline();
  ALWAYS_INLINE() bool trig_conditional(uint8_t condition);
  void send_parameter_locks(uint8_t step, bool trig, uint16_t lock_idx = 0xFFFF);
  ALWAYS_INLINE() void send_parameter_locks_inline(uint8_t step, bool trig, uint16_t lock_idx);
  void get_step_locks(uint8_t step, uint8_t *params);

  ALWAYS_INLINE() void recalc_slides();
  ALWAYS_INLINE()
  void find_next_locks(uint8_t curidx, uint8_t step, uint8_t mask);

  void set_track_pitch(uint8_t step, uint8_t pitch);
  void set_track_step(uint8_t step, uint8_t utiming, uint8_t velocity);
  // !! Note lockidx is lock index, not param id
  bool set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t velocity);
  // !! Note track_param is param_id, not lock index
  bool set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
  // !! Note lockidx is lock index, not param_id

  uint8_t get_track_lock(uint8_t step, uint8_t lockidx);
  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param);

  void record_track(uint8_t velocity);
  void record_track_locks(uint8_t track_param, uint8_t value);
  void record_track_pitch(uint8_t pitch);
  void clear_slide_data();
  void clear_step_locks(uint8_t step);
  // disable the step locks, but not remove them. so later can be re-activated.
  void disable_step_locks(uint8_t step);
  void enable_step_locks(uint8_t step);
  // access the step lock bitmap, masked by locks_enable bit.
  uint8_t get_step_locks(uint8_t step);
  void clear_conditional();
  void clear_step_lock(uint8_t step, uint8_t param_id);
  void clear_locks(bool reset_params = true);
  void clear_track(bool locks = true, bool reset_params = true);
  void clear_param_locks(uint8_t param_id);
  bool is_param(uint8_t param_id);
  void update_kit_params();
  void update_params();
  void update_param(uint8_t param_id, uint8_t value);
  void reset_params();
  void merge_from_md(uint8_t track_number, MDPattern *pattern);

  void set_length(uint8_t len, bool expand = false);
  void re_sync();

  void rotate_left() { modify_track(DIR_LEFT); }
  void rotate_right() { modify_track(DIR_RIGHT); }
  void reverse() { modify_track(DIR_REVERSE); }

  void modify_track(uint8_t dir);

  void set_speed(uint8_t _speed);

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);
};

#endif /* MDSEQTRACK_H__ */
