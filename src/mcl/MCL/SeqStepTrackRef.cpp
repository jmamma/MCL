/* Justin Mammarella jmamma@gmail.com 2018 */

#include "SeqStepTrackRef.h"
#include "../Drivers/MD/UI/Pages/RAMPage.h"
#include "SeqPages.h"
#include <string.h>

void seq_step_set_md_linked_param_update(bool enabled) {
  seq_ptc_page.cc_link_enable = enabled;
  RAMPage::cc_link_enable = enabled;
}

#if !defined(__AVR__)
namespace {

class MdStepOps {
public:
  static bool uses_kit_sound(const void *track) {
    (void)track;
    return true;
  }

  static bool selects_track_locally(const void *track) {
    (void)track;
    return false;
  }

  static uint8_t lock_slot_count(const void *track) {
    (void)track;
    return NUM_LOCKS;
  }

  static uint8_t length(const void *track) { return md(track)->length; }
  static uint8_t speed(const void *track) { return md(track)->speed; }
  static uint8_t step_count(const void *track) {
    return md(track)->step_count;
  }
  static uint8_t track_index(const void *track) {
    return md(track)->track_number;
  }
  static uint8_t mute_state(const void *track) {
    return md(track)->mute_state;
  }
  static void set_mute_state(void *track, uint8_t state) {
    md(track)->mute_state = state;
  }
  static void set_length(void *track, uint8_t len, bool expand) {
    md(track)->set_length(len, expand);
  }
  static bool request_speed_change(void *track, uint8_t new_speed) {
    return md(track)->request_speed_change(new_speed);
  }

  static uint8_t condition_count(const void *track) {
    (void)track;
    return NUM_TRIG_CONDITIONS;
  }

  static void condition_label(const void *track, uint8_t condition, bool plock,
                              bool marker, char *out) {
    (void)track;
    if (out == nullptr) {
      return;
    }
    static const char PROGMEM ptab[] = "12579";

    char a = 'L';
    char b = '1';
    if (condition != 0) {
      if (condition <= 8) {
        b = '0' + condition;
      } else if (condition <= 13) {
        a = 'P';
        b = pgm_read_byte(&ptab[condition - 9]);
      } else if (condition == 14) {
        a = '1';
        b = 'S';
      }
    }

    out[0] = a;
    out[1] = b;
    uint8_t i = 2;
    if (plock) {
      out[i++] = marker ? '+' : '^';
    }
    out[i] = '\0';
  }

  static uint8_t timing_encoder_min(const void *track) {
    (void)track;
    return 1;
  }
  static uint8_t timing_encoder_center(const void *track) {
    return timing_mid(md(track));
  }
  static uint8_t timing_encoder_max(const void *track) {
    return timing_mid(md(track)) * 2 - 1;
  }
  static uint8_t timing_display_mid(const void *track) {
    return timing_mid(md(track));
  }
  static uint8_t timing_encoder_for_step(const void *track, uint8_t step) {
    const MDSeqTrack *t = md(track);
    uint8_t timing = t->timing[step];
    return timing == 0 ? timing_mid(t) : timing;
  }
  static int8_t microtiming_for_step(const void *track, uint8_t step) {
    (void)track;
    (void)step;
    return 0;
  }

  static void rotate_left(void *track) { md(track)->rotate_left(); }
  static void rotate_right(void *track) { md(track)->rotate_right(); }
  static void reverse(void *track) { md(track)->reverse(); }
  static void transpose(void *track, int8_t offset) {
    md(track)->transpose(offset);
  }
  static void get_mask(const void *track, uint64_t *mask, uint8_t ui_mask) {
    md(track)->get_mask(mask, ui_mask);
  }
  static bool get_step(const void *track, uint8_t step, uint8_t ui_mask) {
    return md(track)->get_step(step, ui_mask);
  }
  static void set_step(void *track, uint8_t step, uint8_t ui_mask,
                       bool value) {
    md(track)->set_step(step, ui_mask, value);
  }

  static uint8_t conditional_id(const void *track, uint8_t step) {
    return md(track)->steps[step].cond_id;
  }
  static bool conditional_plock(const void *track, uint8_t step) {
    return md(track)->steps[step].cond_plock;
  }
  static void set_conditional(void *track, uint8_t step, uint8_t condition,
                              bool plock) {
    md(track)->steps[step].cond_id = condition;
    md(track)->steps[step].cond_plock = plock;
  }
  static void set_timing_from_encoder(void *track, uint8_t step,
                                      uint8_t encoder_value) {
    md(track)->timing[step] = encoder_value;
  }
  static void set_pattern_step_from_edit(void *track, uint8_t step,
                                         uint8_t condition, bool cond_plock,
                                         uint8_t timing_encoder) {
    MDSeqTrack *t = md(track);
    t->steps[step].trig = true;
    t->steps[step].cond_id = condition;
    t->steps[step].cond_plock = cond_plock;
    t->timing[step] = timing_encoder;
  }
  static void reset_timing(void *track, uint8_t step) {
    MDSeqTrack *t = md(track);
    t->timing[step] = t->get_timing_mid();
  }

  static void clear_mute(void *track, uint8_t step) {
    md(track)->mute_mask &= ~(1ULL << step);
  }
  static void toggle_mute(void *track, uint8_t step) {
    md(track)->mute_mask ^= (1ULL << step);
  }
  static uint64_t mute_mask(const void *track) {
    return md(track)->mute_mask;
  }

  static void enable_step_locks(void *track, uint8_t step) {
    md(track)->enable_step_locks(step);
  }
  static void clear_step_lock(void *track, uint8_t step, uint8_t param_id) {
    md(track)->clear_step_lock(step, param_id);
  }
  static void clear_step_locks(void *track, uint8_t step) {
    md(track)->clear_step_locks(step);
  }
  static void clear_param_locks(void *track, uint8_t param_id) {
    md(track)->clear_param_locks(param_id);
  }
  static void clear_locks(void *track) { md(track)->clear_locks(); }
  static bool set_track_locks(void *track, uint8_t step, uint8_t param_id,
                              uint8_t value) {
    return md(track)->set_track_locks(step, param_id, value);
  }
  static bool step_has_lock(const void *track, uint8_t step,
                            uint8_t lock_idx) {
    return md(track)->steps[step].is_lock(lock_idx);
  }
  static uint8_t find_param(const void *track, uint8_t param_id) {
    return md(track)->find_param(param_id);
  }
  static uint8_t get_track_lock_implicit(void *track, uint8_t step,
                                         uint8_t param_id) {
    return md(track)->get_track_lock_implicit(step, param_id);
  }

  static void clear_track(void *track, bool locks) {
    md(track)->clear_track(locks);
  }
  static void clear_step(void *track, uint8_t step) {
    MDSeqStep empty_step;
    memset(&empty_step, 0, sizeof(empty_step));
    md(track)->paste_step(step, &empty_step);
  }
  static void clean_params(void *track) { md(track)->clean_params(); }
  static void copy_step(void *track, uint8_t step, MDSeqStep *md_step,
                        void *stepseq_step) {
    (void)stepseq_step;
    md(track)->copy_step(step, md_step);
  }
  static void paste_step(void *track, uint8_t step, MDSeqStep *md_step,
                         void *stepseq_step) {
    (void)stepseq_step;
    md(track)->paste_step(step, md_step);
  }
  static void set_track_pitch(void *track, uint8_t step, uint8_t pitch) {
    md(track)->set_track_pitch(step, pitch);
  }
  static void get_step_locks(void *track, uint8_t step, uint8_t *params,
                             bool ignore_locks_disabled) {
    md(track)->get_step_locks(step, params, ignore_locks_disabled);
  }
  static void record_track(void *track, uint8_t velocity) {
    md(track)->record_track(velocity);
  }
  static void record_track_locks(void *track, uint8_t param_id,
                                 uint8_t value) {
    md(track)->record_track_locks(param_id, value);
  }
  static bool preview_step(void *track, uint8_t step) {
    MDSeqTrack *t = md(track);
    t->send_parameter_locks(step, true);
    MD.triggerTrack(t->track_number, 127);
    return true;
  }
  static void set_linked_param_update(void *track, bool enabled) {
    (void)track;
    seq_step_set_md_linked_param_update(enabled);
  }

private:
  static MDSeqTrack *md(void *track) {
    return static_cast<MDSeqTrack *>(track);
  }

  static const MDSeqTrack *md(const void *track) {
    return static_cast<const MDSeqTrack *>(track);
  }

  static uint8_t timing_mid(const MDSeqTrack *track) {
    return SeqTrack::get_timing_mid(track->speed);
  }
};

class StepSeqOps {
public:
  static bool uses_kit_sound(const void *track) {
    return !stepseq(track)->owns_sound_data();
  }

  static bool selects_track_locally(const void *track) {
    return stepseq(track)->owns_sound_data();
  }

  static uint8_t lock_slot_count(const void *track) {
    (void)track;
    return STEPSEQ_NUM_LOCKS;
  }
  static uint8_t length(const void *track) { return stepseq(track)->length; }
  static uint8_t speed(const void *track) { return stepseq(track)->speed; }
  static uint8_t step_count(const void *track) {
    return stepseq(track)->step_count;
  }
  static uint8_t track_index(const void *track) {
    return stepseq(track)->track_number;
  }
  static uint8_t mute_state(const void *track) {
    return stepseq(track)->mute_state;
  }
  static void set_mute_state(void *track, uint8_t state) {
    stepseq(track)->mute_state = state;
  }
  static void set_length(void *track, uint8_t len, bool expand) {
    stepseq(track)->set_length(len, expand);
  }
  static bool request_speed_change(void *track, uint8_t new_speed) {
    StepSeqDataTrack *t = stepseq(track);
    if (t->count_down || t->speed == new_speed) {
      return false;
    }
    t->set_speed(new_speed, t->speed, true);
    return true;
  }

  static uint8_t condition_count(const void *track) {
    (void)track;
    return STEPSEQ_NUM_TRIG_CONDITIONS - 1;
  }

  static void condition_label(const void *track, uint8_t condition, bool plock,
                              bool marker, char *out) {
    (void)track;
    if (out == nullptr) {
      return;
    }

    char a = '-';
    char b = '-';
    char d = '-';
    if (condition == STEPSEQ_COND_100PCT) {
      a = '-';
      b = '-';
      d = '-';
    } else if (condition <= STEPSEQ_COND_10PCT) {
      static const uint8_t pcts[] = {90, 75, 66, 50, 33, 25, 10};
      uint8_t pct = pcts[condition - 1];
      a = '0' + (pct / 10);
      b = '0' + (pct % 10);
      d = '%';
    } else if (condition == STEPSEQ_COND_ONESHOT) {
      a = '1';
      b = 'S';
      d = 'H';
    } else if (condition == STEPSEQ_COND_FIRST) {
      a = '1';
      b = 'S';
      d = 'T';
    } else if (condition == STEPSEQ_COND_NOT_FIRST) {
      a = '!';
      b = '1';
      d = 'S';
    } else if (condition == STEPSEQ_COND_FILL) {
      a = 'F';
      b = 'I';
      d = 'L';
    } else if (condition == STEPSEQ_COND_NOT_FILL) {
      a = '!';
      b = 'F';
      d = 'L';
    } else if (condition == STEPSEQ_COND_PRE) {
      a = 'P';
      b = 'R';
      d = 'E';
    } else if (condition == STEPSEQ_COND_NOT_PRE) {
      a = '!';
      b = 'P';
      d = 'R';
    } else if (condition == STEPSEQ_COND_NEI) {
      a = 'N';
      b = 'E';
      d = 'I';
    } else if (condition == STEPSEQ_COND_NOT_NEI) {
      a = '!';
      b = 'N';
      d = 'E';
    } else if (condition >= STEPSEQ_COND_ITER_BASE &&
               condition <= STEPSEQ_COND_ITER_MAX) {
      uint8_t x = 0;
      uint8_t y = 0;
      if (stepseq_cond_iter_decode(condition, x, y)) {
        a = '0' + x;
        b = '/';
        d = '0' + y;
      }
    }
    out[0] = a;
    out[1] = b;
    out[2] = plock ? (marker ? '+' : '^') : d;
    out[3] = '\0';
  }

  static uint8_t timing_encoder_min(const void *track) {
    (void)track;
    return 0;
  }
  static uint8_t timing_encoder_center(const void *track) {
    (void)track;
    return 127;
  }
  static uint8_t timing_encoder_max(const void *track) {
    (void)track;
    return 254;
  }
  static uint8_t timing_display_mid(const void *track) {
    (void)track;
    return 0;
  }
  static uint8_t timing_encoder_for_step(const void *track, uint8_t step) {
    return (uint8_t)(stepseq(track)->microtiming[step] + 127);
  }
  static int8_t microtiming_for_step(const void *track, uint8_t step) {
    return stepseq(track)->microtiming[step];
  }

  static void rotate_left(void *track) { stepseq(track)->rotate_left(); }
  static void rotate_right(void *track) { stepseq(track)->rotate_right(); }
  static void reverse(void *track) { stepseq(track)->reverse(); }
  static void transpose(void *track, int8_t offset) {
    stepseq(track)->transpose(offset);
  }
  static void get_mask(const void *track, uint64_t *mask, uint8_t ui_mask) {
    stepseq(track)->get_mask(mask, mask_for(ui_mask));
  }
  static bool get_step(const void *track, uint8_t step, uint8_t ui_mask) {
    return stepseq(track)->get_step(step, mask_for(ui_mask));
  }
  static void set_step(void *track, uint8_t step, uint8_t ui_mask,
                       bool value) {
    stepseq(track)->set_step(step, mask_for(ui_mask), value);
  }

  static uint8_t conditional_id(const void *track, uint8_t step) {
    return stepseq(track)->steps[step].cond_id;
  }
  static bool conditional_plock(const void *track, uint8_t step) {
    return stepseq(track)->steps[step].cond_plock;
  }
  static void set_conditional(void *track, uint8_t step, uint8_t condition,
                              bool plock) {
    StepSeqDataTrack *t = stepseq(track);
    t->steps[step].cond_id = condition;
    t->steps[step].cond_plock = plock;
  }
  static void set_timing_from_encoder(void *track, uint8_t step,
                                      uint8_t encoder_value) {
    stepseq(track)->microtiming[step] =
        (int8_t)((int16_t)encoder_value - 127);
  }
  static void set_pattern_step_from_edit(void *track, uint8_t step,
                                         uint8_t condition, bool cond_plock,
                                         uint8_t timing_encoder) {
    StepSeqDataTrack *t = stepseq(track);
    t->set_step(step, STEPSEQ_MASK_PATTERN, true);
    t->steps[step].cond_id = condition;
    t->steps[step].cond_plock = cond_plock;
    t->microtiming[step] = (int8_t)((int16_t)timing_encoder - 127);
  }
  static void reset_timing(void *track, uint8_t step) {
    stepseq(track)->microtiming[step] = 0;
  }

  static void clear_mute(void *track, uint8_t step) {
    stepseq(track)->mute_mask &= ~(1ULL << step);
  }
  static void toggle_mute(void *track, uint8_t step) {
    stepseq(track)->mute_mask ^= (1ULL << step);
  }
  static uint64_t mute_mask(const void *track) {
    return stepseq(track)->mute_mask;
  }

  static void enable_step_locks(void *track, uint8_t step) {
    stepseq(track)->enable_step_locks(step);
  }
  static void clear_step_lock(void *track, uint8_t step, uint8_t param_id) {
    stepseq(track)->clear_step_lock(step, param_id);
  }
  static void clear_step_locks(void *track, uint8_t step) {
    stepseq(track)->clear_step_locks(step);
  }
  static void clear_param_locks(void *track, uint8_t param_id) {
    stepseq(track)->clear_param_locks(param_id);
  }
  static void clear_locks(void *track) { stepseq(track)->clear_locks(); }
  static bool set_track_locks(void *track, uint8_t step, uint8_t param_id,
                              uint8_t value) {
    return stepseq(track)->set_track_locks(step, param_id, value);
  }
  static bool step_has_lock(const void *track, uint8_t step,
                            uint8_t lock_idx) {
    return stepseq(track)->steps[step].is_lock(lock_idx);
  }
  static uint8_t find_param(const void *track, uint8_t param_id) {
    return stepseq(track)->find_param(param_id);
  }
  static uint8_t get_track_lock_implicit(void *track, uint8_t step,
                                         uint8_t param_id) {
    return stepseq(track)->get_track_lock_implicit(step, param_id);
  }

  static void clear_track(void *track, bool locks) {
    stepseq(track)->clear_track(locks);
  }
  static void clear_step(void *track, uint8_t step) {
    stepseq(track)->clear_step(step);
  }
  static void clean_params(void *track) { stepseq(track)->clean_params(); }
  static void copy_step(void *track, uint8_t step, MDSeqStep *md_step,
                        void *stepseq_step) {
    (void)md_step;
    stepseq(track)->copy_step(step, static_cast<StepSeqStep *>(stepseq_step));
  }
  static void paste_step(void *track, uint8_t step, MDSeqStep *md_step,
                         void *stepseq_step) {
    (void)md_step;
    stepseq(track)->paste_step(step, static_cast<StepSeqStep *>(stepseq_step));
  }
  static void set_track_pitch(void *track, uint8_t step, uint8_t pitch) {
    stepseq(track)->set_track_pitch(step, pitch);
  }
  static void get_step_locks(void *track, uint8_t step, uint8_t *params,
                             bool ignore_locks_disabled) {
    stepseq(track)->get_step_locks(step, params, ignore_locks_disabled);
  }
  static void record_track(void *track, uint8_t velocity) {
    stepseq(track)->record_track(velocity);
  }
  static void record_track_locks(void *track, uint8_t param_id,
                                 uint8_t value) {
    stepseq(track)->record_track_locks(param_id, value);
  }
  static bool preview_step(void *track, uint8_t step) {
    return stepseq(track)->preview_step(step);
  }
  static void set_linked_param_update(void *track, bool enabled) {
    if (uses_kit_sound(track)) {
      seq_step_set_md_linked_param_update(enabled);
    }
  }

private:
  static StepSeqDataTrack *stepseq(void *track) {
    return static_cast<StepSeqDataTrack *>(track);
  }

  static const StepSeqDataTrack *stepseq(const void *track) {
    return static_cast<const StepSeqDataTrack *>(track);
  }

  static uint8_t mask_for(uint8_t ui_mask) {
    switch (ui_mask) {
    case MASK_LOCK:
      return STEPSEQ_MASK_LOCK;
    case MASK_MUTE:
      return STEPSEQ_MASK_MUTE;
    case MASK_SLIDE:
      return STEPSEQ_MASK_SLIDE;
    case MASK_LOCKS_ON_STEP:
      return STEPSEQ_MASK_LOCKS_ON_STEP;
    case MASK_PATTERN:
    default:
      return STEPSEQ_MASK_PATTERN;
    }
  }
};

} // namespace

const SeqStepTrackOps *seq_step_md_ops() {
  static const SeqStepTrackOps ops = {
      false,
      false,
      true,
      MdStepOps::uses_kit_sound,
      MdStepOps::selects_track_locally,
      MdStepOps::lock_slot_count,
      MdStepOps::length,
      MdStepOps::speed,
      MdStepOps::step_count,
      MdStepOps::track_index,
      MdStepOps::mute_state,
      MdStepOps::set_mute_state,
      MdStepOps::set_length,
      MdStepOps::request_speed_change,
      MdStepOps::condition_count,
      MdStepOps::condition_label,
      MdStepOps::timing_encoder_min,
      MdStepOps::timing_encoder_center,
      MdStepOps::timing_encoder_max,
      MdStepOps::timing_display_mid,
      MdStepOps::timing_encoder_for_step,
      MdStepOps::microtiming_for_step,
      MdStepOps::rotate_left,
      MdStepOps::rotate_right,
      MdStepOps::reverse,
      MdStepOps::transpose,
      MdStepOps::get_mask,
      MdStepOps::get_step,
      MdStepOps::set_step,
      MdStepOps::conditional_id,
      MdStepOps::conditional_plock,
      MdStepOps::set_conditional,
      MdStepOps::set_timing_from_encoder,
      MdStepOps::set_pattern_step_from_edit,
      MdStepOps::reset_timing,
      MdStepOps::clear_mute,
      MdStepOps::toggle_mute,
      MdStepOps::mute_mask,
      MdStepOps::enable_step_locks,
      MdStepOps::clear_step_lock,
      MdStepOps::clear_step_locks,
      MdStepOps::clear_param_locks,
      MdStepOps::clear_locks,
      MdStepOps::set_track_locks,
      MdStepOps::step_has_lock,
      MdStepOps::find_param,
      MdStepOps::get_track_lock_implicit,
      MdStepOps::clear_track,
      MdStepOps::clear_step,
      MdStepOps::clean_params,
      MdStepOps::copy_step,
      MdStepOps::paste_step,
      MdStepOps::set_track_pitch,
      MdStepOps::get_step_locks,
      MdStepOps::record_track,
      MdStepOps::record_track_locks,
      MdStepOps::preview_step,
      MdStepOps::set_linked_param_update,
  };
  return &ops;
}

const SeqStepTrackOps *seq_step_stepseq_ops() {
  static const SeqStepTrackOps ops = {
      true,
      true,
      false,
      StepSeqOps::uses_kit_sound,
      StepSeqOps::selects_track_locally,
      StepSeqOps::lock_slot_count,
      StepSeqOps::length,
      StepSeqOps::speed,
      StepSeqOps::step_count,
      StepSeqOps::track_index,
      StepSeqOps::mute_state,
      StepSeqOps::set_mute_state,
      StepSeqOps::set_length,
      StepSeqOps::request_speed_change,
      StepSeqOps::condition_count,
      StepSeqOps::condition_label,
      StepSeqOps::timing_encoder_min,
      StepSeqOps::timing_encoder_center,
      StepSeqOps::timing_encoder_max,
      StepSeqOps::timing_display_mid,
      StepSeqOps::timing_encoder_for_step,
      StepSeqOps::microtiming_for_step,
      StepSeqOps::rotate_left,
      StepSeqOps::rotate_right,
      StepSeqOps::reverse,
      StepSeqOps::transpose,
      StepSeqOps::get_mask,
      StepSeqOps::get_step,
      StepSeqOps::set_step,
      StepSeqOps::conditional_id,
      StepSeqOps::conditional_plock,
      StepSeqOps::set_conditional,
      StepSeqOps::set_timing_from_encoder,
      StepSeqOps::set_pattern_step_from_edit,
      StepSeqOps::reset_timing,
      StepSeqOps::clear_mute,
      StepSeqOps::toggle_mute,
      StepSeqOps::mute_mask,
      StepSeqOps::enable_step_locks,
      StepSeqOps::clear_step_lock,
      StepSeqOps::clear_step_locks,
      StepSeqOps::clear_param_locks,
      StepSeqOps::clear_locks,
      StepSeqOps::set_track_locks,
      StepSeqOps::step_has_lock,
      StepSeqOps::find_param,
      StepSeqOps::get_track_lock_implicit,
      StepSeqOps::clear_track,
      StepSeqOps::clear_step,
      StepSeqOps::clean_params,
      StepSeqOps::copy_step,
      StepSeqOps::paste_step,
      StepSeqOps::set_track_pitch,
      StepSeqOps::get_step_locks,
      StepSeqOps::record_track,
      StepSeqOps::record_track_locks,
      StepSeqOps::preview_step,
      StepSeqOps::set_linked_param_update,
  };
  return &ops;
}
#endif
