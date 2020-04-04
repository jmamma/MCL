/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "MDSeqTrackData.h"
#include "AuxPages.h"

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
  uint8_t iterations;
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

  ALWAYS_INLINE() void seq() {
    if (mute_until_start) {

      if (clock_diff(MidiClock.div16th_counter, start_step) == 0) {
        DEBUG_PRINTLN("unmuting");
        DEBUG_PRINTLN(track_number);
        DEBUG_PRINTLN(MidiClock.div16th_counter);
        DEBUG_PRINTLN(start_step);
        DEBUG_PRINTLN(MidiClock.mod12_counter);
        step_count = 0;
        iterations = 1;
        mute_until_start = false;
      }
    }
    if ((MidiUart.uart_block == 0) && (mute_until_start == false) &&
        (mute_state == SEQ_MUTE_OFF)) {

      uint8_t next_step = 0;
      if (step_count == (length - 1)) {
        next_step = 0;
      } else {
        next_step = step_count + 1;
      }

      if ((timing[step_count] >= 12) &&
          (timing[step_count] - 12 == MidiClock.mod12_counter)) {

        // Dont transmit locks if MDExploit is on.
        send_parameter_locks(step_count);

        if (IS_BIT_SET64(pattern_mask, step_count)) {
          trig_conditional(conditional[step_count]);
        }
      }
      if ((timing[next_step] < 12) &&
          ((timing[next_step]) == MidiClock.mod12_counter)) {

        send_parameter_locks(next_step);

        if (IS_BIT_SET64(pattern_mask, next_step)) {
          trig_conditional(conditional[next_step]);
        }
      }
    }
    if (MidiClock.mod12_counter == 11) {
      if (step_count == length - 1) {
        step_count = 0;
        iterations++;
        if (iterations > 8) {
          iterations = 1;
        }
      } else {
        step_count++;
      }
      //       DEBUG_PRINT(step_count);
      //       DEBUG_PRINT(" ");
    }
  }

  void send_trig();
  ALWAYS_INLINE() void send_trig_inline() {
    mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
    if (MD.kit.trigGroups[track_number] < 16) {
      mixer_page.disp_levels[MD.kit.trigGroups[track_number]] =
          MD.kit.levels[MD.kit.trigGroups[track_number]];
    }
    MD.triggerTrack(track_number, 127);
  }

  ALWAYS_INLINE() void trig_conditional(uint8_t condition) {
    bool send_trig = false;
    switch (condition) {
    case 0:
    case 1:
      if (!IS_BIT_SET64(oneshot_mask, step_count)) {
        send_trig = true;
      }
      break;
    case 2:
      if (!IS_BIT_SET(iterations, 0)) {
        send_trig = true;
      }
    case 4:
      if ((iterations == 4) || (iterations == 8)) {
        send_trig = true;
      }
    case 8:
      if ((iterations == 8)) {
        send_trig = true;
      }
      break;
    case 3:
      if ((iterations == 3) || (iterations == 6)) {
        send_trig = true;
      }
      break;
    case 5:
      if (iterations == 5) {
        send_trig = true;
      }
      break;
    case 7:
      if (iterations == 7) {
        send_trig = true;
      }
      break;
    case 9:
      if (get_random_byte() <= 13) {
        send_trig = true;
      }
      break;
    case 10:
      if (get_random_byte() <= 32) {
        send_trig = true;
      }
      break;
    case 11:
      if (get_random_byte() <= 64) {
        send_trig = true;
      }
      break;
    case 12:
      if (get_random_byte() <= 96) {
        send_trig = true;
      }
      break;
    case 13:
      if (get_random_byte() <= 115) {
        send_trig = true;
      }
      break;
    case 14:
      if (!IS_BIT_SET64(oneshot_mask, step_count)) {
        SET_BIT64(oneshot_mask, step_count);
        send_trig = true;
      }
    }
    if (send_trig) {
      send_trig_inline();
    }
  }

  ALWAYS_INLINE() void send_parameter_locks(uint8_t step) {
    uint8_t c;
    bool lock_mask_step = IS_BIT_SET64(lock_mask, step);
    bool pattern_mask_step = IS_BIT_SET64(pattern_mask, step);
    uint8_t send_param = 255;

    if (lock_mask_step && pattern_mask_step) {
      for (c = 0; c < 4; c++) {
        if (locks_params[c] > 0) {
          if (locks[c][step] > 0) {
            send_param = locks[c][step] - 1;
          } else {
            send_param = locks_params_orig[c];
          }
          MD.setTrackParam_inline(track_number, locks_params[c] - 1,
                                  send_param);
        }
      }
    }

    else if (lock_mask_step) {
      for (c = 0; c < 4; c++) {
        if (locks[c][step] > 0 && locks_params[c] > 0) {
          send_param = locks[c][step] - 1;
          MD.setTrackParam_inline(track_number, locks_params[c] - 1,
                                  send_param);
        }
      }
    }

    else if (pattern_mask_step) {

      for (c = 0; c < 4; c++) {
        if (locks_params[c] > 0) {
          send_param = locks_params_orig[c];
          MD.setTrackParam_inline(track_number, locks_params[c] - 1,
                                  send_param);
        }
      }
    }
  }

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

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);
};

#endif /* MDSEQTRACK_H__ */
