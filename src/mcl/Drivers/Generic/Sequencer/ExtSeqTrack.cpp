#include "ExtTrack.h"
#include "GUI/Pages/CommonPages.h"
#include "Sequencer/MCLSeq.h"
#include "MCLSysConfig.h"
#include "PtcVoiceRouter.h"
#include "SeqTrackTransition.h"
#include "platform.h"

uint8_t ExtSeqTrack::epoch = 0;

namespace {

static bool ext_delete_marked(const uint8_t *mask, uint16_t idx) NOINLINE();
static bool ext_delete_marked(const uint8_t *mask, uint16_t idx) {
  return mask[idx >> 3] & (1 << (idx & 7));
}

static void ext_delete_mark(uint8_t *mask, uint16_t idx) NOINLINE();
static void ext_delete_mark(uint8_t *mask, uint16_t idx) {
  mask[idx >> 3] |= (1 << (idx & 7));
}

static inline bool ext_note_overlaps_range(int16_t note_start,
                                           int16_t note_end,
                                           int16_t range_start,
                                           int16_t range_end,
                                           int16_t roll_length) {
  if (note_start <= range_end && note_end > range_start) {
    return true;
  }
  if (note_end > roll_length) {
    return note_end - roll_length > range_start;
  }
  return false;
}

static inline int16_t ext_microtiming_ticks(int8_t microtiming,
                                            uint16_t ticks_per_step) {
  return SeqTrack::microtiming_to_ticks(microtiming, ticks_per_step);
}

static int16_t ext_event_tick(uint8_t step, int8_t microtiming,
                              uint16_t ticks_per_step) NOINLINE();
static int16_t ext_event_tick(uint8_t step, int8_t microtiming,
                              uint16_t ticks_per_step) {
  return (int16_t)step * ticks_per_step +
         ext_microtiming_ticks(microtiming, ticks_per_step);
}

static int8_t ext_page_timing_to_microtiming(uint16_t timing,
                                             uint16_t ticks_per_step)
    NOINLINE();
static int8_t ext_page_timing_to_microtiming(uint16_t timing,
                                             uint16_t ticks_per_step) {
  return SeqTrack::timing_to_microtiming(timing, ticks_per_step);
}

static int8_t ext_reverse_microtiming(int8_t microtiming) NOINLINE();
static int8_t ext_reverse_microtiming(int8_t microtiming) {
  return microtiming == -128 ? 127 : (int8_t)-microtiming;
}

static void ext_rotate_events(ext_event_t *events, uint16_t ev_end,
                              uint8_t n_cur, uint8_t dir) NOINLINE();
static void ext_rotate_events(ext_event_t *events, uint16_t ev_end,
                              uint8_t n_cur, uint8_t dir) {
  ext_event_t ev_cur[16];
  if (dir == DIR_LEFT) {
    memcpy(ev_cur, events, sizeof(ext_event_t) * n_cur);
    memmove(events, events + n_cur, sizeof(ext_event_t) * (ev_end - n_cur));
    memcpy(events + ev_end - n_cur, ev_cur, sizeof(ext_event_t) * n_cur);
  } else {
    memcpy(ev_cur, events + ev_end - n_cur, sizeof(ext_event_t) * n_cur);
    memmove(events + n_cur, events, sizeof(ext_event_t) * (ev_end - n_cur));
    memcpy(events, ev_cur, sizeof(ext_event_t) * n_cur);
  }
}

static void ext_rotate_bytes(uint8_t *values, uint8_t length, uint8_t dir)
    NOINLINE();
static void ext_rotate_bytes(uint8_t *values, uint8_t length, uint8_t dir) {
  uint8_t vel_tmp;
  if (dir == DIR_LEFT) {
    vel_tmp = values[0];
    memmove(values, values + 1, length - 1);
    values[length - 1] = vel_tmp;
  } else {
    vel_tmp = values[length - 1];
    memmove(values + 1, values, length - 1);
    values[0] = vel_tmp;
  }
}

} // namespace


void ExtSeqTrack::set_speed(uint8_t new_speed, uint8_t old_speed,
                            bool timing_adjust) {
  (void)old_speed;
  (void)timing_adjust;
  speed = new_speed;
  uint8_t ticks_per_step = get_ticks_per_step();
  if (ticks_per_step && mod12_counter >= ticks_per_step) {
    mod12_counter = mod12_counter % ticks_per_step;
    // step_count_inc();
  }
}

void ExtSeqTrack::set_length(uint8_t len, bool expand) {
  if (len == 0) {
    len = 1;
  }
  if (len > 128) {
    len = 16;
  }
  uint8_t old_length = length;
  length = len;
  uint8_t step = step_count;
  if (step >= length && length > 0) {
    step = step % length;
  }

  uint16_t idx, end;
  locate(step, idx, end);
  USE_LOCK();
  SET_LOCK();
  step_count = step;
  cur_event_idx = idx;
  CLEAR_LOCK();

  if (expand && old_length <= 16 && length >= 16) {
    for (uint8_t n = old_length; n < length; n++) {
      if (event_buckets.get(n) != 0) {
        expand = false;
        return;
      }
    }
    ext_event_t empty_events[MAX_EVENTS_PER_STEP];
    uint8_t a = 0;
    for (uint8_t n = old_length; n < 128; n++) {
      uint16_t ev_idx, ev_end;
      locate(a, ev_idx, ev_end);
      uint8_t bucket_size = ev_end - ev_idx;
      if (bucket_size > MAX_EVENTS_PER_STEP) {
        bucket_size = MAX_EVENTS_PER_STEP;
      }
      memcpy(empty_events, &events[ev_idx],
             bucket_size * sizeof(ext_event_t));
      for (uint8_t m = 0; m < bucket_size; m++) {
        add_event(n, &empty_events[m]);
      }
      velocities[n] = velocities[a];
      a++;
      if (a == old_length) {
        a = 0;
      }
    }
  } else if (length < old_length) {
      notesoff_pending = true;
  }
}

void ExtSeqTrack::re_sync() {
  //  uint32_t q = length * 12;
  //  start_step = (MidiClock.div16th_counter / q) * q + q;
}

void ExtSeqTrack::remove_event(uint16_t index) {
  uint16_t ev_idx = 0;
  uint8_t step;
  uint8_t bucket = 0;
  if (index >= event_count) {
    // bucket empty
    return;
  }
  for (step = 0; step < NUM_EXT_STEPS && ev_idx <= index; ++step) {
    bucket = event_buckets.get(step);
    ev_idx += bucket;
  }
  --step;
  event_buckets.set(step, bucket - 1);
  // move [index+1...event_count-1] to [index...event_count-2]
  memmove(events + index, events + index + 1,
          sizeof(ext_event_t) * (event_count - index - 1));
  if (step < step_count) {
    cur_event_idx--;
  }
  epoch++;
  --event_count;
}

uint16_t ExtSeqTrack::add_event(uint8_t step, ext_event_t *e) {
  uint8_t u = event_buckets.get(step);
  if (15 == u || event_count >= NUM_EXT_EVENTS) {
    // bucket full or track full
    return 0xFFFF;
  }

  uint16_t idx, end;
  locate(step, idx, end);

  // Insertion sort
  while (idx < end && events[idx] < *e) {
    ++idx;
  }
  event_buckets.set(step, u + 1);
  // move [idx...event_count-1] to [idx+1...event_count]
  memmove(events + idx + 1, events + idx,
          sizeof(ext_event_t) * (event_count - idx));
  events[idx] = *e;
  if (step < step_count) {
    cur_event_idx++;
  }
  ++event_count;
  epoch++;
  return idx;
}

void ExtSeqTrack::recalc_slides() {
  if (locks_slides_recalc == 255) {
    return;
  }
  DEBUG_PRINT_FN();
  int16_t x0, x1;
  int8_t y0, y1;
  uint8_t step = locks_slides_recalc;
  uint8_t ticks_per_step = get_ticks_per_step_inline();

  uint8_t find_array[NUM_LOCKS] = {0};

  uint16_t curidx;

  curidx = locks_slides_idx;

  // Because, we support two lock values of same lock_idx per step.
  // Slide -> from last lock event on start step to first lock event on
  // destination step
  bool find = false;
  for (int8_t n = event_buckets.get(step) - 1; n >= 0; n--) {
    auto &e = events[curidx + n];
    if (e.is_lock && e.event_on && e.lock_idx < NUM_LOCKS &&
        locks_params[e.lock_idx]) {
      find_array[e.lock_idx] = 1;
      find = true;
    }
  }

  if (!find) {
    goto end;
  }
  find_next_locks(curidx, step, find_array);

  ext_event_t *e;
  for (int8_t n = event_buckets.get(step) - 1; n >= 0; n--) {
    e = &events[curidx + n];
    if (!e->is_lock || !e->event_on || e->lock_idx >= NUM_LOCKS ||
        !locks_params[e->lock_idx]) {
      continue;
    }
    uint8_t c = e->lock_idx;
    uint8_t param = locks_params[c] - 1;
    if (param == PARAM_PRG || param > PARAM_CHP) continue;

    // Skip params where find_next_locks found no target
    if (find_array[c]) {
      locks_slide_data[c].init();
      continue;
    }
    uint8_t next_lockstep = locks_slide_next_lock_step[c];

    if (step == next_lockstep) {
      locks_slide_data[c].init();
      continue;
    }
    x0 = ext_event_tick(step, e->micro_timing, ticks_per_step) + 1;
    uint8_t x1_step = next_lockstep + (next_lockstep < step ? length : 0);
    x1 = x1_step * ticks_per_step +
         ext_microtiming_ticks(locks_slide_next_lock_utiming[c],
                               ticks_per_step) -
         1;
    y0 = e->event_value;
    y1 = locks_slide_next_lock_val[c];
    /*
     DEBUG_DUMP("prepare slide");
     DEBUG_DUMP(c);
     DEBUG_DUMP(x0);
     DEBUG_DUMP(x1);
     DEBUG_DUMP(y0);
     DEBUG_DUMP(y1);
     */
    prepare_slide(c, x0, x1, y0, y1);
  }
end:
  locks_slides_recalc = 255;
}

uint8_t ExtSeqTrack::find_next_lock(uint8_t step, uint8_t lock_idx,
                                    uint16_t &curidx, uint16_t &end) {
  uint8_t next_step = step + 1;
  locate(next_step, curidx, end);
  uint8_t max_len = length;
  curidx = end;

again:
  for (; next_step < max_len; next_step++) {
    end += event_buckets.get(next_step);
    for (; curidx < end; curidx++) {
      auto &e = events[curidx];
      if (!e.is_lock || !e.event_on)
        continue;
      return next_step;
    }
  }

  if (next_step >= length) {
    next_step = 0;
    curidx = 0;
    max_len = step;
    goto again;
  }
  return step;
}

void ExtSeqTrack::find_next_locks(uint16_t curidx, uint8_t step,
                                  uint8_t *find_array) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(step);

  uint8_t count = 0;
  for (uint8_t n = 0; n < NUM_LOCKS; n++) {
    if (find_array[n] == 1) {
      count++;
    }
  }
  if (count == 0) {
    return;
  }
  // caller ensures step < length

  curidx += event_buckets.get(step);
  uint8_t next_step = step + 1;

  uint16_t end = curidx;
  uint8_t max_len = length;

again:
  for (; next_step < max_len; next_step++) {
    end += event_buckets.get(next_step);
    for (; curidx < end; curidx++) {
      if (count == 0) {
        return;
      }
      auto &e = events[curidx];
      if (!e.is_lock || e.lock_idx >= NUM_LOCKS || !locks_params[e.lock_idx])
        continue;

      uint8_t i = e.lock_idx;
      DEBUG_DUMP(e.lock_idx);
      // if (!e.event_on) { find_array[i] = 0; }
      if (find_array[i] == 1) {
        /*
  DEBUG_DUMP("found lock");
  DEBUG_DUMP(i);
  DEBUG_DUMP(next_step);
  DEBUG_DUMP(e.event_value);
  */
        locks_slide_next_lock_val[i] = e.event_value;
        locks_slide_next_lock_step[i] = next_step;
        locks_slide_next_lock_utiming[i] = e.micro_timing;
        find_array[i] = 0;
        count--;
        /* } else if (e.trig) {
           locks_slide_next_lock_val[i] = locks_params_orig[i];
           locks_slide_next_lock_step[i] = next_step;
           locks_slide_next_lock_utiming[i] = e.micro_timing;
         } */
      }
    }
  }

  if (next_step == length) {
    next_step = 0;
    curidx = 0;
    end = 0;
    max_len = step;
    goto again;
  }
}

uint16_t ExtSeqTrack::find_lock(uint8_t step, uint8_t lock_idx,
                                uint16_t &start_idx) {
  uint16_t end;
  locate(step, start_idx, end);
  for (uint16_t i = start_idx; i != end; ++i) {
    if (!events[i].is_lock || events[i].lock_idx != lock_idx) {
      continue;
    }
    return i;
  }
  return 0xFFFF;
}

uint16_t ExtSeqTrack::find_midi_note(uint8_t step, uint8_t note_num,
                                     uint16_t &start_idx, bool event_on) {
  uint16_t end;
  locate(step, start_idx, end);
  for (uint16_t i = start_idx; i != end; ++i) {
    if (events[i].is_lock || events[i].event_value != note_num ||
        events[i].event_on != event_on) {
      continue;
    }
    return i;
  }
  return 0xFFFF;
}

uint16_t ExtSeqTrack::find_midi_note(uint8_t step, uint8_t note_num,
                                     uint16_t &start_idx) {
  uint16_t end;
  locate(step, start_idx, end);
  for (uint16_t i = start_idx; i != end; ++i) {
    if (events[i].is_lock || events[i].event_value != note_num) {
      continue;
    }
    return i;
  }
  return 0xFFFF;
}

uint8_t ExtSeqTrack::search_lock_idx(uint8_t lock_idx, uint8_t step,
                                     uint16_t &ev_idx, uint16_t &ev_end) {
  // Scan for matching note off;
  uint8_t j = step;
  ++ev_idx;

  do {
    for (; ev_idx != ev_end; ++ev_idx) {
      auto &ev = events[ev_idx];
      if (!ev.is_lock || ev.lock_idx != lock_idx) {
        continue;
      }
      // if (!ev.event_on) { goto end; }
      return j;
    }
    ++j;
    if (j >= length) {
      // wrap around
      j = 0;
      ev_end = 0;
    }
    ev_idx = ev_end;
    ev_end += event_buckets.get(j);
  } while (j != step);
  ev_idx = 0xFFFF;
  return step;
}

uint8_t ExtSeqTrack::search_note_off(int8_t note_val, uint8_t step,
                                     uint16_t &ev_idx, uint16_t ev_end, uint8_t _length) {
  // Scan for matching note off;
  if (_length == 0) {
    _length = length;
  }
  if (_length == 0) {
    ev_idx = 0xFFFF;
    return step;
  }
  uint8_t j = step;
  ++ev_idx;

  do {
    for (; ev_idx < ev_end; ++ev_idx) {
      auto &ev = events[ev_idx];
      if (ev.is_lock || ev.event_value != note_val || ev.event_on) {
        continue;
      }
      return j;
    }
    ++j;
    if (j >= _length) {
      // wrap around
      j = 0;
      ev_end = 0;
    }
    ev_idx = ev_end;
    ev_end += event_buckets.get(j);
  } while (j != step);

  ev_idx = 0xFFFF;
  return step;
}

void ExtSeqTrack::init_notes_on() {
  notes_on_count = 0;
  for (uint8_t n = 0; n < NUM_NOTES_ON; n++) {
    notes_on[n].value = 255;
  }
}

void ExtSeqTrack::add_notes_on(uint8_t step, int8_t utiming, uint8_t value,
                               uint8_t velocity) {
  if (notes_on_count >= NUM_NOTES_ON) {
    return;
  }

  uint8_t slot = 255;
  uint8_t match = 255;
  /*
  DEBUG_DUMP("adding notes on");
  DEBUG_DUMP(value);
  */
  for (uint8_t n = 0; n < NUM_NOTES_ON; n++) {
    if (notes_on[n].value == 255 && slot == 255) {
      slot = n;
    } else if (notes_on[n].value == value) {
      match = n;
      break;
    }
  }
  if (match != 255) {
    slot = match;
  } else {
    if (slot == 255)
      return;
    notes_on_count++;
  }
  notes_on[slot].value = value;
  notes_on[slot].step = step;
  notes_on[slot].utiming = utiming;
  notes_on[slot].velocity = velocity;

  return;
}

uint8_t ExtSeqTrack::find_notes_on(uint8_t value) {
  for (uint8_t n = 0; n < NUM_NOTES_ON; n++) {
    if (notes_on[n].value == value) {
      return n;
    }
  }
  return 255;
}

void ExtSeqTrack::remove_notes_on(uint8_t value) {
  uint8_t n = find_notes_on(value);
  if (n == 255)
    return;
  notes_on[n].value = 255;
  if (notes_on_count == 0)
    return;
  notes_on_count--;
}

void ExtSeqTrack::add_note(uint16_t cur_x, uint16_t cur_w, uint8_t cur_y,
                           uint8_t velocity, uint8_t cond) {

  uint8_t ticks_per_step = get_ticks_per_step();

  uint8_t step = (cur_x / ticks_per_step);
  uint8_t start_utiming = ticks_per_step + cur_x - (step * ticks_per_step);

  DEBUG_DUMP(step);
  DEBUG_DUMP(start_utiming);

  uint8_t end_step = ((cur_x + cur_w) / ticks_per_step);
  DEBUG_DUMP(end_step);

  if (end_step == step) {
    DEBUG_PRINTLN(F("ALERT start == end"));
    end_step = end_step + 1;
  }
  uint8_t end_utiming = ticks_per_step + (cur_x + cur_w) - (end_step * ticks_per_step);
  DEBUG_DUMP(end_utiming);

  if (end_step >= length) {
    end_step = end_step - length;
  }
  uint16_t ev_idx;
  uint16_t note_idx = find_midi_note(step, cur_y, ev_idx, /*event_on*/ true);
  if (note_idx != 0xFFFF) {
    DEBUG_DUMP(F("abort note on"));
    return;
  }

  ev_idx = 0;
  note_idx = find_midi_note(end_step, cur_y, ev_idx, /*event_on*/ false);
  if (note_idx != 0xFFFF) {
    DEBUG_DUMP(F("abort note off"));
    return;
  }

  set_track_step(step, start_utiming, cur_y, true, velocity, cond);
  set_track_step(end_step, end_utiming, cur_y, false, velocity, 0);
}

bool ExtSeqTrack::del_note(uint16_t cur_x, uint16_t cur_w, uint8_t cur_y) {
  DEBUG_DUMP(F("del_note"));
  DEBUG_DUMP(cur_x);
  DEBUG_DUMP(cur_w);
  uint8_t ticks_per_step = get_ticks_per_step();

  // uint8_t end_step = ((cur_x + cur_w) / ticks_per_step);
  // uint8_t end_utiming = ticks_per_step + (cur_x + cur_w) - (end_step *
  // ticks_per_step);

  uint16_t note_idx_off, note_idx_on;
  bool note_on_found = false;
  uint16_t ev_idx, ev_end;
  int16_t note_start, note_end;
  const int16_t selection_start = (int16_t)cur_x;
  const int16_t selection_end = (int16_t)(cur_x + cur_w);
  bool ret = false;

  for (uint8_t i = 0; i < length; i++) {

  again:
    note_idx_on = find_midi_note(i, cur_y, ev_idx, /*event_on*/ true);
    ev_end = ev_idx + event_buckets.get(i);

    if (note_idx_on != 0xFFFF) {
      note_on_found = true;
      note_idx_off = note_idx_on;
      uint8_t j = search_note_off(cur_y, i, note_idx_off, ev_end);
      if (note_idx_off != 0xFFFF) {
        auto &ev = events[note_idx_on];
        auto &ev_j = events[note_idx_off];
        note_start = ext_event_tick(i, ev.micro_timing, ticks_per_step);
        note_end = ext_event_tick(j, ev_j.micro_timing, ticks_per_step);
        if (note_end < note_start) {
          note_end += length * ticks_per_step;
        }
        if ((note_start <= selection_end) && (note_end > selection_start)) {
          remove_event(note_idx_off);
          note_idx_on = find_midi_note(i, cur_y, ev_idx, /*event_on*/ true);
          remove_event(note_idx_on);
          note_off(cur_y);
          ret = true;
          goto again;
        }
      }
    }
    if (note_on_found) {
      continue;
    }
    note_idx_off = find_midi_note(i, cur_y, ev_idx, /*event_on*/ false);

    if (note_idx_off != 0xFFFF) {
      // Remove wrap around notes
      auto &ev = events[note_idx_off];
      int32_t note_end = ext_event_tick(i, ev.micro_timing, ticks_per_step);
      if (note_end > selection_start) {
        remove_event(note_idx_off);
        for (uint8_t j = length - 1; j > i; j--) {
          note_idx_on = find_midi_note(j, cur_y, ev_idx, true);
          if (note_idx_on != 0xFFFF) {
            remove_event(note_idx_on);
            break;
          }
        }
        note_off(cur_y);
        ret = true;
        goto again;
      }
    }
  }

  return ret;
}

bool ExtSeqTrack::del_notes(uint16_t cur_x, uint16_t cur_w,
                            uint8_t note_min, uint8_t note_max) {
  if (note_min > note_max || length == 0 || event_count == 0) {
    return false;
  }

  uint8_t ticks_per_step = get_ticks_per_step();
  if (ticks_per_step == 0) {
    return false;
  }

  uint8_t note_mask[16];
  memset(note_mask, 0, sizeof(note_mask));

  const int16_t range_start = (int16_t)cur_x;
  const int16_t range_end = (int16_t)(cur_x + cur_w);
  const int16_t roll_ticks = (int16_t)length * ticks_per_step;

  uint16_t ev_idx = 0;
  uint16_t ev_end = 0;
  for (uint8_t step = 0; step < length; step++) {
    ev_end += event_buckets.get(step);
    for (; ev_idx != ev_end; ++ev_idx) {
      ext_event_t &ev = events[ev_idx];
      if (ev.is_lock || !ev.event_on || ev.event_value < note_min ||
          ev.event_value > note_max ||
          ext_delete_marked(note_mask, ev.event_value)) {
        continue;
      }

      uint16_t note_off_idx = ev_idx;
      uint8_t off_step = search_note_off(ev.event_value, step, note_off_idx,
                                         ev_end);
      if (note_off_idx == 0xFFFF) {
        continue;
      }

      ext_event_t &ev_off = events[note_off_idx];
      int16_t note_start = ext_event_tick(step, ev.micro_timing, ticks_per_step);
      int16_t note_end =
          ext_event_tick(off_step, ev_off.micro_timing, ticks_per_step);
      if (note_end < note_start) {
        note_end += roll_ticks;
      }

      if (ext_note_overlaps_range(note_start, note_end, range_start,
                                  range_end, roll_ticks)) {
        ext_delete_mark(note_mask, ev.event_value);
      }
    }
  }

  bool changed = false;
  for (uint8_t note = note_min; note <= note_max; note++) {
    if (ext_delete_marked(note_mask, note)) {
      changed |= del_note(cur_x, cur_w, note);
    }
    if (note == note_max) break;
  }
  return changed;
}

void ExtSeqTrack::reset_params() {
  if (ptc_route_channel_is_primary(channel)) {
    return;
  }
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      uint8_t param = locks_params[c] - 1;
      if (param < 128) {
        uart->sendCC(channel, param, locks_params_orig[c]);
      }
      if (param == PARAM_PB) {
        uart->sendPitchBend(channel, 8192);
      }
      if (param == PARAM_CHP) {
        uart->sendChannelPressure(channel, 0);
      }
    }
  }
}

void ExtSeqTrack::handle_event(uint16_t index, uint8_t step) {
  auto &ev = events[index];
  if (ev.is_lock) {
    if (ptc_route_channel_is_primary(channel)) {
      return;
    }
    if (ev.lock_idx >= NUM_LOCKS || !locks_params[ev.lock_idx]) {
      return;
    }
    // plock
    uint8_t param = locks_params[ev.lock_idx] - 1;
    if (param == PARAM_PRG) {
      if (!pgm_oneshot) {
        pgm_oneshot = true;
        uart->sendProgramChange(channel, ev.event_value);
      }
    } else {
      if (param == PARAM_PB) {
        uart->sendPitchBend(channel, ev.event_value << 7);
      } else if (param == PARAM_CHP) {
        uart->sendChannelPressure(channel, ev.event_value);
      } else {
        uart->sendCC(channel, param, ev.event_value);
      }

      // event_on == lock slide
      if (ev.event_on) {
        locks_slides_recalc = step;
      }
    }
  } else {
    // midi note
    if (step == ignore_step && IS_BIT_SET128_P(ignore_notes, ev.event_value))
      return;

    if (ev.event_on) {
      noteon_conditional(ext_event_condition(ev), ev.event_value,
                         velocities[step]);
    } else {
      if (!mcl_seq.ext_arp_tracks[track_number].consumes_seq_trig()) {
        note_off(ev.event_value, 0);
      }
    }
  }
}

void ExtSeqTrack::load_cache() {
  // Save on stack space, break ExtTrack in to chunks.
  //  GridTrack g;
  //  g.load_from_mem(track_number, BANK1_A4_TRACKS_START);
  //  g.load_link_data(this);
  buffer_notesoff();
  ExtTrackChunk t;

  for (uint8_t n = 0; n < t.get_chunk_count(); n++) {
    t.load_from_mem_chunk(track_number, n);
    t.load_chunk(data(), n);
  }
  t.load_link_from_mem(track_number);
  t.load_link_data(this);
}

void ExtSeqTrack::seq(MidiUartClass *uart_) {
  MidiUartClass *uart_old = uart;
  uart = uart_;

  if (mod12_counter == 0) {
    uint8_t pending_speed;
    if (consume_pending_speed_change(pending_speed)) {
      set_speed(pending_speed, speed, true);
    }
  }

  uint8_t ticks_per_step = get_ticks_per_step_inline();

  mod12_counter++;

  if (mod12_counter == ticks_per_step) {
    cur_event_idx += event_buckets.get(step_count);
    mod12_counter = 0;
    if (ignore_step == step_count) {
      ignore_step = 255;
      memset(ignore_notes, 0, sizeof(ignore_notes));
    }
    step_count_inc();
  }

  if (count_down) {
    count_down--;
    if (count_down == 0) {
      reset();
      mod12_counter = 0;
    } else if (is_generic_midi &&
               SeqTrackTransition::in_cache_window(
                   SEQ_TRANSITION_CACHE_MIDI_LINEAR, count_down,
                   track_number)) {
      if (!cache_loaded) {
        load_cache();
        cache_loaded = true;
      }
      goto end;
    }
  }

  uint16_t ev_idx, ev_end;

  if (record_mutes) {
    uint8_t u = 0;
    uint8_t q = 0;
    uint8_t s = get_quantized_step(u, q);
    SET_BIT128_P(mute_mask, s);
  }

  apply_pending_mute();

  if (notesoff_pending) {
    notesoff_pending = false;
    buffer_notesoff();
  }

  if ((is_generic_midi || (!is_generic_midi && count_down == 0)) &&
      (mute_state == SEQ_MUTE_OFF)) {
    // SEQ_MUTE_OFF)) {
    // the range we're interested in:
    // [current timing bucket, micro >= ticks_per_step ... next timing bucket, micro
    // < ticks_per_step]

    ev_idx = cur_event_idx;
    ev_end = cur_event_idx + event_buckets.get(step_count);

    if (!ptc_route_channel_is_primary(channel)) {
      send_slides(locks_params, channel);
    }

    // Go over CURRENT
    for (; ev_idx != ev_end; ++ev_idx) {
      int16_t timing_offset =
          ext_microtiming_ticks(events[ev_idx].micro_timing, ticks_per_step);
      if (timing_offset >= 0 && timing_offset == mod12_counter) {
        handle_event(ev_idx, step_count);
      }
    }

    // Locate NEXT
    uint8_t next_step = 0;
    if (step_count == length - 1) {
      next_step = 0;
      ev_idx = 0;
    } else {
      next_step = step_count + 1;
    }
    ev_end = ev_idx + event_buckets.get(next_step);

    // Go over NEXT
    for (; ev_idx != ev_end; ++ev_idx) {
      int16_t timing_offset =
          ext_microtiming_ticks(events[ev_idx].micro_timing, ticks_per_step);
      int16_t trigger_tick = (int16_t)ticks_per_step + timing_offset;
      if (timing_offset < 0 && trigger_tick == mod12_counter) {
        handle_event(ev_idx, next_step);
      }
    }
  }

end:
  locks_slides_idx = cur_event_idx;
  uart = uart_old;
}

MidiUartClass *ExtSeqTrack::resolve_uart(MidiUartClass *uart_) {
  return uart_ == nullptr ? uart : uart_;
}

void ExtSeqTrack::note_on(uint8_t note, uint8_t velocity,
                          MidiUartClass *uart_) {
  uart_ = resolve_uart(uart_);
#ifdef LFO_TRACKS
  mcl_seq.report_track_trig(DeviceIdx::Secondary, track_number);
#endif
  mixer_page.track_trig(DeviceIdx::Secondary, track_number, 127);
  if (ptc_route_channel_is_primary(channel)) {
    ptc_voice_router.note_on(channel, note, uart_);
  } else {
    uart_->sendNoteOn(channel, note, velocity);
  }
  SET_BIT128_P(note_buffer, note);
}

void ExtSeqTrack::note_off(uint8_t note, uint8_t velocity,
                           MidiUartClass *uart_) {
  uart_ = resolve_uart(uart_);
  if (ptc_route_channel_is_primary(channel)) {
    ptc_voice_router.note_off(channel, note);
  } else {
    uart_->sendNoteOff(channel, note);
  }
  CLEAR_BIT128_P(note_buffer, note);
}

void ExtSeqTrack::buffer_notesoff() {
  init_notes_on();
  uint8_t *buf = (uint8_t *)note_buffer;
  for (uint8_t i = 0; i < sizeof(note_buffer); ++i) {
    if (buf[i]) {
      buffer_notesoff8(&buf[i], i << 3);
    }
  }
}

void ExtSeqTrack::buffer_notesoff8(uint8_t *buf, uint8_t offset) {
  uint8_t count = 0;
  while (*buf) {
    if (*buf & 1) {
      if (ptc_route_channel_is_primary(channel)) {
        ptc_voice_router.note_off(channel, offset + count);
      } else {
        uart->sendNoteOff(channel, offset + count);
      }
    }
    count++;
    *buf >>= 1;
  }
}

void ExtSeqTrack::noteon_conditional(uint8_t condition, uint8_t note,
                                     uint8_t velocity) {
  if (IS_BIT_SET128_P(oneshot_mask, step_count) ||
      IS_BIT_SET128_P(mute_mask, step_count)) {
    record_trig_result(false);
    return;
  }
  bool send_note = false;
  if (condition == SEQ_COND_ONESHOT) {
    SET_BIT128_P(oneshot_mask, step_count);
    send_note = true;
  } else {
    send_note = conditional(condition,
                            mcl_seq.fill_mask_for(DeviceIdx::Secondary));
  }

  if (send_note) {
    if (!mcl_seq.ext_arp_tracks[track_number].trigger()) {
      note_on(note, velocity);
    }
  }
  record_trig_result(send_note);
}

void ExtSeqTrack::pitch_bend(uint16_t value, MidiUartClass *uart_) {
  if (ptc_route_channel_is_primary(channel)) {
    return;
  }
  resolve_uart(uart_)->sendPitchBend(channel, value);
}

void ExtSeqTrack::channel_pressure(uint8_t pressure, MidiUartClass *uart_) {
  if (ptc_route_channel_is_primary(channel)) {
    return;
  }
  resolve_uart(uart_)->sendChannelPressure(channel, pressure);
}

void ExtSeqTrack::after_touch(uint8_t note, uint8_t pressure,
                              MidiUartClass *uart_) {
  if (ptc_route_channel_is_primary(channel)) {
    return;
  }
  resolve_uart(uart_)->sendPolyKeyPressure(channel, note, pressure);
}

void ExtSeqTrack::send_cc(uint8_t cc, uint8_t value, MidiUartClass *uart_) {
  if (ptc_route_channel_is_primary(channel)) {
    return;
  }
  resolve_uart(uart_)->sendCC(channel, cc, value);
}

void ExtSeqTrack::update_param(uint8_t param_id, uint8_t value) {
  param_id += 1;

  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] == param_id) {
        locks_params_orig[c] = value;
        return;
      }
    }
  }
}

uint8_t ExtSeqTrack::find_lock_idx(uint8_t param_id) {
  param_id += 1;
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] == param_id) {
      return c;
    }
  }
  return 255;
}
#define PARAM_VEL 128

void ExtSeqTrack::record_track_locks(uint8_t track_param, uint8_t value,
                                     bool slide) {
  if (!mcl_cfg.rec_automation || step_count >= length) {
    return;
  }

  uint8_t utiming = 0;
  uint8_t step = get_quantized_step(utiming);
  // clear all locks on step
  clear_track_locks(step, track_param, 255);
  set_track_locks(step, utiming, track_param, value, slide);
}

bool ExtSeqTrack::del_track_locks(int16_t cur_x, uint8_t lock_idx,
                                  uint8_t value) {
  uint8_t ticks_per_step = get_ticks_per_step();
  uint8_t step = (cur_x / ticks_per_step);

  if (step != 0) {
    --step;
  }
  uint16_t start_idx = 0;
  uint16_t end = 0;
  locate(step, start_idx, end);
  end = start_idx;
  bool ret = false;

  uint8_t r = 4;

  for (uint8_t n = step; n < min(length, step + 3); n++) {
    end += event_buckets.get(n);
    for (; start_idx < end;) {
      DEBUG_DUMP(start_idx);
      uint16_t i = start_idx;
      if (!events[i].is_lock || events[i].lock_idx != lock_idx) {
        //|| (events[i].event_value > value + r ||
        // events[i].event_value < min(0, value - r)))
        ++start_idx;
        continue;
      }
      int16_t event_x = ext_event_tick(n, events[i].micro_timing, ticks_per_step);
      if (event_x == cur_x || (event_x <= cur_x + r && event_x >= cur_x - r)) {
        uint8_t param = locks_params[lock_idx] - 1;
        if (param == PARAM_PRG) {
          pgm_oneshot = 0;
        }
        remove_event(i);
        --end;
        ret = true;
      } else {
        ++start_idx;
      }
    }
  }
  return ret;
}

void ExtSeqTrack::clear_track_locks() {
  for (uint8_t n = 0; n < NUM_LOCKS; n++) {
    clear_track_locks(n);
  }
  pgm_oneshot = 0;
}

void ExtSeqTrack::clear_track_locks(uint8_t idx) {
  if (idx >= NUM_LOCKS) {
    return;
  }
  for (uint8_t n = 0; n < NUM_EXT_STEPS; n++) {
    clear_track_locks_idx(n, idx, 255);
  }
  locks_slide_data[idx].init();
}

bool ExtSeqTrack::clear_track_locks(uint8_t step, uint8_t track_param,
                                    uint8_t value) {
  uint8_t lock_idx = find_lock_idx(track_param);
  return clear_track_locks_idx(step, lock_idx, value);
}

bool ExtSeqTrack::clear_track_locks_idx(uint8_t step, uint8_t lock_idx,
                                        uint8_t value) {
  if (lock_idx >= NUM_LOCKS) {
    return false;
  }
  uint16_t start_idx, end;
  locate(step, start_idx, end);
  bool ret = false;

  for (uint16_t i = start_idx; i != end;) {
    if (!events[i].is_lock || events[i].lock_idx != lock_idx) {
      ++i;
      continue;
    }
    if (value == 255 || (events[i].event_value == value)) {
      remove_event(i);
      --end;
      ret = true;
      if (value != 255) {
        break;
      }
    } else {
      ++i;
    }
  }
  return ret;
}

uint8_t ExtSeqTrack::count_lock_event(uint8_t step, uint8_t lock_idx) {
  uint16_t start_idx, end;
  locate(step, start_idx, end);
  uint8_t count = 0;
  for (uint16_t i = start_idx; i != end; ++i) {
    if (!events[i].is_lock || events[i].lock_idx != lock_idx) {
      continue;
    }
    count++;
  }
  return count;
}

bool ExtSeqTrack::set_track_locks(uint8_t step, uint8_t utiming,
                                  uint8_t track_param, uint8_t value,
                                  bool event_on, uint8_t lock_idx) {

  if (lock_idx == 255) {
    // Let's try and find an existing param
    lock_idx = find_lock_idx(track_param);

    for (uint8_t c = 0; c < NUM_LOCKS && lock_idx == 255; c++) {
      if (locks_params[c] == 0) {
        locks_params[c] = track_param + 1;
        // locks_params_orig[c] = MD.kit.params[track_number][track_param];
        lock_idx = c;
      }
    }
  }
  ext_event_t *e;

  if (lock_idx != 255) {

    //  uint16_t lock_ev_idx = find_lock(step, match, ev_idx);

    // if (lock_ev_idx != 0xFFFF) {
    //   e = &events[lock_ev_idx];
    // } else {

    // Only allow maximum 2 lock events of same idx per step
    /*
    uint8_t count = count_lock_event(step, lock_idx);
    if (count >= 2) {
      return false;
    }*/

    // Don't allow slides for non CC paramaters
    if (track_param == PARAM_PRG || track_param > PARAM_CHP) {
      event_on = false;
    }

    ext_event_t new_event;
    e = &new_event;
    DEBUG_DUMP(F("adding lock"));
    DEBUG_DUMP(lock_idx);

    e->is_lock = true;
    e->cond_id = 0;
    e->lock_idx = lock_idx;
    e->event_value = value;
    e->event_on = event_on;
    e->micro_timing = ext_page_timing_to_microtiming(utiming,
                                                     get_ticks_per_step());

    if (add_event(step, e) == 0xFFFF) {
      return false;
    }

    return true;
  }
  return false;
}

bool ExtSeqTrack::set_track_step(uint8_t step, uint8_t utiming,
                                 uint8_t note_num, uint8_t event_on,
                                 uint8_t velocity, uint8_t cond) {
  ext_event_t e;

  if (event_on) {
    CLEAR_BIT128_P(oneshot_mask, step);
  }
  e.is_lock = false;
  e.lock_idx = 0;
  ext_event_set_condition(e, cond);
  e.event_value = note_num;
  e.event_on = event_on;
  e.micro_timing = ext_page_timing_to_microtiming(utiming,
                                                  get_ticks_per_step());

  DEBUG_PRINTLN("adding step");
  DEBUG_DUMP(event_on);
  DEBUG_DUMP(step);
  DEBUG_DUMP(utiming);
  if (add_event(step, &e) == 0xFFFF) {
    return false;
  }
  if (event_on || velocities[step] == 0) {
    velocities[step] = velocity;
  }
  return true;
}

void ExtSeqTrack::store_mute_state() {
  uint8_t ticks_per_step = get_ticks_per_step();
  for (uint8_t n = 0; n < NUM_EXT_STEPS; n++) {
    if (IS_BIT_SET128_P(mute_mask, n)) {
      uint16_t ev_idx, ev_end;
    loc:
      locate(n, ev_idx, ev_end);
      for (uint16_t m = ev_idx; m < ev_end; m++) {
        if (!events[m].is_lock && events[m].event_on) {
          if (del_note(ext_event_tick(n, events[m].micro_timing, ticks_per_step),
                       ticks_per_step, events[m].event_value)) {
            goto loc;
          }
        }
      }
    }
  }
  clear_mutes();
}

void ExtSeqTrack::record_track_noteoff(uint8_t note_num) {

  uint8_t ticks_per_step = get_ticks_per_step();

  uint8_t n = find_notes_on(note_num);
  if (n == 255)
    return;

  if (MidiClock.state == 2 && SeqPage::recording) {

    int8_t utiming = mod12_counter - 1;
    uint8_t step = step_count;

    ignore_step = step;
    SET_BIT128_P(ignore_notes, note_num);

    uint16_t w = 0;

    int16_t start_x = notes_on[n].step * ticks_per_step + notes_on[n].utiming;
    int16_t end_x = step * ticks_per_step + utiming;
    int16_t roll_length = length * ticks_per_step;

    if (start_x < 0) {
      start_x += roll_length;
    }
    if (end_x < 0) {
      end_x += roll_length;
    }

    if (mcl_cfg.rec_quant) {
      if (end_x < start_x) {
        end_x += roll_length;
      }
      w = max(1, end_x - start_x);

      int8_t u = notes_on[n].utiming;
      uint8_t s = notes_on[n].step;
      if (u > ticks_per_step / 2) {
        s++;
        if (s == length) {
          s = 0;
        }
      }
      u = 0;
      start_x = s * ticks_per_step + u;
      end_x = start_x + w;
      if (end_x > roll_length) {
        del_note(0, end_x - roll_length, note_num);
      }
    } else {
      if (end_x < start_x) {
        del_note(0, end_x, note_num);
        end_x += roll_length;
      }
      w = end_x - start_x;
    }
    del_note(start_x, w, note_num);
    add_note(start_x, w, note_num, notes_on[n].velocity, 0);
  }

  notes_on[n].value = 255;
  notes_on_count--;
}

void ExtSeqTrack::record_track_noteon(uint8_t note_num, uint8_t velocity) {

  int8_t utiming = mod12_counter - 1;
  uint8_t step = step_count;

  ignore_step = step;
  SET_BIT128_P(ignore_notes, note_num);

  add_notes_on(step, utiming, note_num, velocity);
}

void ExtSeqTrack::clear_mute() { memset(mute_mask, 0, sizeof(mute_mask)); }

void ExtSeqTrack::clear_mutes() {
  memset(oneshot_mask, 0, sizeof(oneshot_mask));
  memset(mute_mask, 0, sizeof(mute_mask));
}

void ExtSeqTrack::clear_ext_conditional() {
  for (uint16_t x = 0; x < NUM_EXT_EVENTS; x++) {
    ext_event_set_condition(events[x], 0);
    events[x].micro_timing = 0;
  }
  clear_mutes();
  memset(ignore_notes, 0, sizeof(ignore_notes));
}

void ExtSeqTrack::clear_ext_notes() {
  ExtSeqTrackData::clear();
}

void ExtSeqTrack::clear_track(bool) {
  clear_ext_notes();
  // Events are inactive after clear_ext_notes(); only runtime masks need reset.
  clear_mutes();
  memset(ignore_notes, 0, sizeof(ignore_notes));
  notesoff_pending = true;
}

void ExtSeqTrack::clear_step(uint8_t step) {
  uint16_t start_idx, end_idx;
  locate(step, start_idx, end_idx);

  uint8_t bucket_size = event_buckets.get(step);

  if (bucket_size > 0) {
    // Remove events by shifting everything after this step's events
    memmove(events + start_idx, events + end_idx,
            sizeof(ext_event_t) * (event_count - end_idx));

    // Update counts
    event_count -= bucket_size;
    event_buckets.set(step, 0);

    // Update cur_event_idx if we're clearing before current position
    if (step < step_count) {
      cur_event_idx -= bucket_size;
    } else if (step == step_count) {
      // If clearing current step, reset to start of step
      cur_event_idx = start_idx;
    }

    epoch++;
  }

  // Clear velocity for this step
  velocities[step] = 0;

  // Clear masks for this step
  CLEAR_BIT128_P(oneshot_mask, step);
  CLEAR_BIT128_P(mute_mask, step);
}

void ExtSeqTrack::modify_track(uint8_t dir) {
  uint8_t old_mute_state = mute_state;
  uint8_t n_cur;
  mute_state = SEQ_MUTE_ON;
  notesoff_pending = true;

  uint8_t ticks_per_step = get_ticks_per_step();
  uint16_t step_idx = 0, ev_end = 0;

  // Collect orphaned notes first (don't modify events yet)
  uint8_t orphaned_notes[16]; // Max possible orphaned notes
  uint8_t orphaned_count = 0;

  // Check for orphaned notes and clear events beyond length
  for (uint8_t step = 0; step < NUM_EXT_STEPS; step++) {
    if (step < length) {
      step_idx = ev_end;
      ev_end += event_buckets.get(step);
      // Check each note-on in this step
      for (uint16_t i = step_idx; i < ev_end; i++) {
        auto &ev = events[i];
        if (!ev.is_lock && ev.event_on) {
          // Found a note-on, search for its note-off
          uint16_t search_idx = i;
          uint16_t step_end = ev_end;
          uint8_t note_off_step = search_note_off(ev.event_value, step,
                                                  search_idx, step_end, NUM_EXT_STEPS);
          if (note_off_step >= length && orphaned_count < 16) {
            orphaned_notes[orphaned_count++] = ev.event_value;
          }
        }
      }
    } else {
      clear_step(step);
    }
  }
  // Now add all the missing note-offs

  //event_count = ev_end;
  for (uint8_t i = 0; i < orphaned_count; i++) {
    set_track_step(length - 1, (ticks_per_step * 2) - 1, orphaned_notes[i], false, 0, 0);
  }
  ev_end = event_count;
  //locate(length - 1, step_idx, ev_end);
  switch (dir) {
  case DIR_LEFT:
    n_cur = event_buckets.get(0);
    ext_rotate_events(events, ev_end, n_cur, DIR_LEFT);
    ext_rotate_bytes(velocities, length, DIR_LEFT);
    event_buckets.shift_left(length);
    break;
  case DIR_RIGHT:
    n_cur = event_buckets.get(length - 1);
    ext_rotate_events(events, ev_end, n_cur, DIR_RIGHT);
    ext_rotate_bytes(velocities, length, DIR_RIGHT);
    event_buckets.shift_right(length);
    break;
  case DIR_REVERSE:
    uint16_t end = ev_end / 2;
    for (uint16_t i = 0; i < end; ++i) {
      auto tmp = events[i];
      auto j = ev_end - i - 1;
      events[i] = events[j];
      events[j] = tmp;

      // need to flip note on/off
      if (!events[i].is_lock) {
        events[i].event_on = !events[i].event_on;
      }
      if (!events[j].is_lock) {
        events[j].event_on = !events[j].event_on;
      }

      events[i].micro_timing = ext_reverse_microtiming(events[i].micro_timing);
      events[j].micro_timing = ext_reverse_microtiming(events[j].micro_timing);
    }
    for (uint8_t n = 0; n < length / 2; n++) {
      uint8_t vel_tmp = velocities[n];
      uint8_t z = length - 1 - n;
      velocities[n] = velocities[z];
      velocities[z] = vel_tmp;
    }
    // reverse timing buckets
    event_buckets.reverse(length);
    break;
  }

  locate(step_count, cur_event_idx, ev_end);
  memset(oneshot_mask, 0, sizeof(oneshot_mask));

  mute_state = old_mute_state;
}

void ExtSeqTrack::apply_pending_mute() {
  if (!mute_state_pending) {
    return;
  }

  mute_state = SEQ_MUTE_ON;
  mute_state_pending = false;
  buffer_notesoff();
}

void ExtSeqTrack::mute_on() {
  if (MidiClock.state == 2) {
    mute_state_pending = true;
  } else {
    mute_state = SEQ_MUTE_ON;
    buffer_notesoff();
  }
}

void ExtSeqTrack::toggle_mute() {
  if (mute_state == SEQ_MUTE_ON) {
    mute_state = SEQ_MUTE_OFF;
  } else {
    mute_on();
  }
}

void ExtSeqTrack::transpose(int8_t offset) {
  uint8_t old_mute_state = mute_state;
  mute_on();
  apply_pending_mute();
  for (uint16_t ev_idx = 0; ev_idx < event_count; ++ev_idx) {
    auto &ev = events[ev_idx];
    if (!ev.is_lock) {
      int16_t note = ev.event_value + offset;
      if (note < 0) {
        ev.event_value = 0;
      } else if (note > 127) {
        ev.event_value = 127;
      } else {
        ev.event_value = note;
      }
    }
  }
  mute_state = old_mute_state;
}
