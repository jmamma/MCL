/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MDSeqTrackData.h"
#include "SeqTrack.h"
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

class MDSeqTrack : public MDSeqTrackData, SeqTrack {

public:
  uint64_t oneshot_mask;
  uint64_t slide_mask;

  uint8_t locks_params_orig[NUM_MD_LOCKS];
  SlideData locks_slide_data[NUM_MD_LOCKS];

  bool load = false;
  //  uint8_t params[24];
  uint8_t trigGroup;
  uint32_t start_step;
  uint8_t start_step_offset;

  bool mute_until_start = false;
  bool mute_hold = false;

  uint8_t locks_slides_recalc = 255;

  uint8_t mute_state = SEQ_MUTE_OFF;
  void mute() { mute_state = SEQ_MUTE_ON; }
  void unmute() { mute_state = SEQ_MUTE_OFF; }

  ALWAYS_INLINE() void reset() {
    step_count = 0;
    iterations_5 = 1;
    iterations_6 = 1;
    iterations_7 = 1;
    iterations_8 = 1;
    oneshot_mask = 0;
    mod12_counter = 0;
    mute_until_start = false;
    mute_hold = false;
  }

  ALWAYS_INLINE() void seq();
  ALWAYS_INLINE() void step_count_inc() {
    if (step_count == length - 1) {
      step_count = 0;

      iterations_5++;
      iterations_6++;
      iterations_7++;
      iterations_8++;

      if (iterations_5 > 5) {
        iterations_5 = 1;
      }
      if (iterations_6 > 6) {
        iterations_6 = 1;
      }
      if (iterations_7 > 7) {
        iterations_7 = 1;
      }
      if (iterations_8 > 8) {
        iterations_8 = 1;
      }
    } else {
      step_count++;
    }
  }
  void send_trig();
  ALWAYS_INLINE() void send_trig_inline();
  ALWAYS_INLINE() bool trig_conditional(uint8_t condition);
  ALWAYS_INLINE() void send_parameter_locks(uint8_t step, bool pattern_mask_step);

  ALWAYS_INLINE() void send_slides();
  ALWAYS_INLINE() void recalc_slides();
  ALWAYS_INLINE() uint8_t find_next_lock(uint8_t step, uint8_t param);

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
  bool is_locks(uint8_t step);
  void clear_param_locks(uint8_t param_id);
  bool is_param(uint8_t param_id);
  void update_kit_params();
  void update_params();
  void update_param(uint8_t param_id, uint8_t value);
  void reset_params();
  void merge_from_md(uint8_t track_number, MDPattern *pattern, MDKit *kit);

  void set_length(uint8_t len);
  void re_sync();

  #define DIR_LEFT 0
  #define DIR_RIGHT 1
  #define DIR_REVERSE 2

  void rotate_left() { modify_track(DIR_LEFT); }
  void rotate_right() { modify_track(DIR_RIGHT); }
  void reverse() { modify_track(DIR_REVERSE); }

  void modify_track(uint8_t dir);

  float get_speed_multiplier();
  float get_speed_multiplier(uint8_t speed);
  void set_speed(uint8_t _speed);

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);

};

#endif /* MDSEQTRACK_H__ */
