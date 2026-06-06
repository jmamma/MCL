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

uint8_t stepseq_mask_for(uint8_t ui_mask) {
  switch (ui_mask) {
  case MASK_LOCK:
    return STEPSEQ_MASK_LOCK;
  case MASK_MUTE:
    return STEPSEQ_MASK_MUTE;
  case MASK_SLIDE:
    return STEPSEQ_MASK_SLIDE;
  case MASK_SWING:
    return STEPSEQ_MASK_SWING;
  case MASK_LOCKS_ON_STEP:
    return STEPSEQ_MASK_LOCKS_ON_STEP;
  case MASK_PATTERN:
  default:
    return STEPSEQ_MASK_PATTERN;
  }
}

} // namespace

void SeqStepTrackGenericBackend::set_live_param_update(bool enabled) const {
  if (!uses_kit_sound()) {
    return;
  }
  step_edit()->set_live_param_update(param_context(), enabled);
  seq_step_set_md_linked_param_update(enabled);
}

bool SeqStepTrackGenericBackend::request_speed_change(uint8_t new_speed) {
  if (kind_ == KIND_MD) {
    return tracks_.md->request_speed_change(new_speed);
  }
  StepSeqDataTrack *t = tracks_.stepseq;
  if (t->count_down || t->speed == new_speed) {
    return false;
  }
  t->set_speed(new_speed, t->speed, true);
  return true;
}

void SeqStepTrackGenericBackend::condition_label(uint8_t condition, bool plock,
                                                 bool marker, char *out) const {
  (void)kind_;
  seq_condition_label(condition, plock, marker, out);
}

void SeqStepTrackGenericBackend::set_pattern_step_from_edit(
    uint8_t step, uint8_t condition_knob, uint8_t timing_encoder) {
  bool cond_plock;
  uint8_t condition = step_conditional_from_knob(condition_knob, &cond_plock);
  if (kind_ == KIND_MD) {
    MDSeqTrack *t = tracks_.md;
    t->steps[step].trig = true;
    set_conditional(step, condition, cond_plock);
    set_timing_from_encoder(step, timing_encoder);
  } else {
    StepSeqDataTrack *t = tracks_.stepseq;
    t->set_step(step, STEPSEQ_MASK_PATTERN, true);
    set_conditional(step, condition, cond_plock);
    set_timing_from_encoder(step, timing_encoder);
  }
}

void SeqStepTrackGenericBackend::get_mask(uint64_t *mask,
                                          uint8_t ui_mask) const {
  if (kind_ == KIND_MD) {
    tracks_.md->get_mask(mask, ui_mask);
  } else {
    tracks_.stepseq->get_mask(mask, stepseq_mask_for(ui_mask));
  }
}

bool SeqStepTrackGenericBackend::get_step(uint8_t step,
                                          uint8_t ui_mask) const {
  return kind_ == KIND_MD
             ? tracks_.md->get_step(step, ui_mask)
             : tracks_.stepseq->get_step(step, stepseq_mask_for(ui_mask));
}

void SeqStepTrackGenericBackend::set_step(uint8_t step, uint8_t ui_mask,
                                          bool value) {
  if (kind_ == KIND_MD) {
    tracks_.md->set_step(step, ui_mask, value);
  } else {
    tracks_.stepseq->set_step(step, stepseq_mask_for(ui_mask), value);
  }
}

void SeqStepTrackGenericBackend::clear_step(uint8_t step) {
  if (kind_ == KIND_MD) {
    MDSeqStep empty_step;
    memset(&empty_step, 0, sizeof(empty_step));
    tracks_.md->paste_step(step, &empty_step);
  } else {
    tracks_.stepseq->clear_step(step);
  }
}

bool SeqStepTrackGenericBackend::preview_step(uint8_t step) {
  if (kind_ == KIND_MD) {
    MDSeqTrack *t = tracks_.md;
    t->send_parameter_locks(step, true);
#ifdef LFO_TRACKS
    mcl_seq.report_track_trig(DeviceIdx::Primary, t->track_number);
#endif
    MD.triggerTrack(t->track_number, 127);
    return true;
  }
  return tracks_.stepseq->preview_step(step);
}
#endif
