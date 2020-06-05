/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MDSeqTrackData.h"
#include "SeqTrack.h"
#define UART1_PORT 1

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

// MD Pattern scale
//[0 = 1x, 1=2x, 2=3/4x, 3=3/2x]

// MCL Track scale
#define MD_SCALE_1X 64
#define MD_SCALE_2X 65
#define MD_SCALE_3_4X 66
#define MD_SCALE_3_2X 67
#define MD_SCALE_1_2X 68
#define MD_SCALE_1_4X 69
#define MD_SCALE_1_8X 70

const uint8_t md_scales[7] PROGMEM = {MD_SCALE_1X,   MD_SCALE_2X,
                                      MD_SCALE_3_4X, MD_SCALE_3_2X,
                                      MD_SCALE_1_2X,
                                      MD_SCALE_1_4X, MD_SCALE_1_8X};
// forward declare MDTrack
class MDTrack;

class MDSeqTrack : public MDSeqTrackData {

public:
  uint8_t track_number;
  uint8_t step_count;
  uint8_t mod12_counter;

  // Conditional counters
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

  ALWAYS_INLINE() void init() {
    step_count = 0;
    iterations_5 = 1;
    iterations_6 = 1;
    iterations_7 = 1;
    iterations_8 = 1;
    oneshot_mask = 0;
    mod12_counter = 0;
    mute_until_start = false;
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
        iterations_8 = 1;
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

  float get_scale_multiplier(bool inverse = false);
  float get_scale_multiplier(uint8_t scale, bool inverse = false);
  void set_scale(uint8_t _scale);

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);

  ALWAYS_INLINE() uint8_t get_timing_mid() {
    uint8_t timing_mid;
    switch (scale) {
    default:
    case MD_SCALE_1X:
      timing_mid = 12;
      break;
    case MD_SCALE_2X:
      timing_mid = 6;
      break;
    case MD_SCALE_3_4X:
      timing_mid = 16; // 12 * (4.0/3.0);
      break;
    case MD_SCALE_3_2X:
      timing_mid = 8; // 12 * (2.0/3.0);
      break;
    case MD_SCALE_1_2X:
      timing_mid = 24;
      break;
    case MD_SCALE_1_4X:
      timing_mid = 48;
      break;
    case MD_SCALE_1_8X:
      timing_mid = 96;
      break;
    }
    return timing_mid;
  }
};

#endif /* MDSEQTRACK_H__ */
