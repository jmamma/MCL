/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACK_H__
#define MDSEQTRACK_H__

#include "DeviceTrack.h"
#include "MD.h"
#include "MDSeqTrackData.h"
#include "SeqTrack.h"

#define UART1_PORT 1

#define SEQ_NOTEBUF_SIZE 8
#define SEQ_MUTE_ON 1
#define SEQ_MUTE_OFF 0

class MDTrack;

class TrigNotes {
public:
  uint8_t note1;
  uint8_t note2;
  uint8_t note3;
  uint8_t len;
  uint8_t vel;
  uint8_t prog = 255;

  uint16_t count_down;
  bool first_trig = false;
};

class MDSeqTrack : public MDSeqTrackData, public SeqSlideTrack {

public:
  uint64_t oneshot_mask;
  uint64_t mute_mask;

  static uint16_t gui_update;
  static uint16_t md_trig_mask;
  static uint32_t load_machine_cache;

  TrigNotes notes;

  const uint8_t number_midi_cc = 6 * 2 + 4;
  const uint8_t midi_cc_array_size = 6 * 2 + 4;

  MDSeqTrack() : SeqSlideTrack() { active = MD_TRACK_TYPE; }
  ALWAYS_INLINE() void reset() {
    SeqSlideTrack::reset();
    oneshot_mask = 0;
    record_mutes = false;
    send_notes_off();
  }

  void get_mask(uint64_t *_pmask, uint8_t mask_type) const;

  bool get_step(uint8_t step, uint8_t mask_type) const;
  void set_step(uint8_t step, uint8_t mask_type, bool val);

  void seq(MidiUartClass *uart_, MidiUartClass *uart2_);

  void mute() { mute_state = SEQ_MUTE_ON; }
  void unmute() { mute_state = SEQ_MUTE_OFF; }

  void send_trig();
  void send_trig_inline();
  uint8_t trig_conditional(uint8_t condition);
  void send_parameter_locks(uint8_t step, bool trig,
                            uint16_t lock_idx = 0xFFFF);
  void send_parameter_locks_inline(uint8_t step, bool trig, uint16_t lock_idx);
  void reset_params();
  void get_step_locks(uint8_t step, uint8_t *params,
                      bool ignore_locks_disabled = false);

  void recalc_slides();
  void find_next_locks(uint8_t curidx, uint8_t step, uint8_t mask);

  void set_track_pitch(uint8_t step, uint8_t pitch);
  void set_track_step(uint8_t step, uint8_t utiming,
                      uint8_t velocity = 127);
  // !! Note lockidx is lock index, not param id
  bool set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t velocity);
  // !! Note track_param is param_id, not lock index
  bool set_track_locks(uint8_t step, uint8_t track_param,
                       uint8_t velocity);
  // !! Note lockidx is lock index, not param_id

  uint8_t get_track_lock(uint8_t step, uint8_t lockidx);
  uint8_t get_track_lock_implicit(uint8_t step, uint8_t param);

  void record_track(uint8_t velocity);
  void record_track_locks(uint8_t track_param, uint8_t value);
  void record_track_pitch(uint8_t pitch);
  void clear_mute();
  void clear_mutes();
  void clear_slide_data();
  void clear_step_locks(uint8_t step);
  // disable the step locks, but not remove them. so later can be re-activated.
  void disable_step_locks(uint8_t step);
  void enable_step_locks(uint8_t step);
  // access the step lock bitmap, masked by locks_enable bit.
  uint8_t get_step_locks(uint8_t step);
  void clear_conditional();
  void clear_step_lock(uint8_t step, uint8_t param_id);
  void clear_locks();
  void clear_track(bool locks = true);
  void clear_param_locks(uint8_t param_id);
  bool is_param(uint8_t param_id);

  void merge_from_md(uint8_t track_number, MDPattern *pattern);

  void set_length(uint8_t len, bool expand = false);
  void re_sync();

  void rotate_left() { modify_track(DIR_LEFT); }
  void rotate_right() { modify_track(DIR_RIGHT); }
  void reverse() { modify_track(DIR_REVERSE); }

  void modify_track(uint8_t dir);

  void set_speed(uint8_t new_speed, uint8_t old_speed = 255,
                 bool timing_adjust = true);

  void store_mute_state();

  void copy_step(uint8_t n, MDSeqStep *step);
  void paste_step(uint8_t n, MDSeqStep *step);
  void load_cache();

  void init_notes() {
    // Copy 3 notes, len and vel from kit to notes structure;
    memcpy(&notes.note1, MD.kit.params[track_number], 5);
    notes.count_down = 0;
  }
  void process_note_locks(uint8_t param, uint8_t val, uint8_t *ccs, bool is_lock = false);
  void send_notes_ccs(uint8_t *ccs, bool send_ccs);
  void send_notes(uint8_t first_note = 255, MidiUartClass *uart2_ = nullptr);
  void send_notes_on(MidiUartClass *uart2_ = nullptr);
  void send_notes_off(MidiUartClass *uart2_ = nullptr);

  uint8_t transpose_pitch(uint8_t pitch, int8_t offset);
  void transpose(int8_t offset);
  void onControlChangeCallback_Midi(uint8_t track_param,uint8_t value);
};

#endif /* MDSEQTRACK_H__ */
