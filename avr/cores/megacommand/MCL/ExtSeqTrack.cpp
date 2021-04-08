#include "MCL_impl.h"

void ExtSeqTrack::set_speed(uint8_t _speed) {
  uint8_t old_speed = speed;
  float mult = get_speed_multiplier(_speed) / get_speed_multiplier(old_speed);
  for (uint16_t i = 0; i < NUM_EXT_EVENTS; i++) {
    events[i].micro_timing = round(mult * (float)events[i].micro_timing);
  }
  speed = _speed;
  uint8_t timing_mid = get_timing_mid();
  if (mod12_counter > timing_mid) {
    mod12_counter = mod12_counter - (mod12_counter / timing_mid) * timing_mid;
    // step_count_inc();
  }
}

void ExtSeqTrack::set_length(uint8_t len) {
  length = len;
  while (step_count >= length && length > 0) {
    step_count = length - step_count;
  }
  DEBUG_DUMP(step_count);
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
  for (step = 0; step < length && ev_idx <= index; ++step) {
    bucket = timing_buckets.get(step);
    ev_idx += bucket;
  }
  --step;
  timing_buckets.set(step, bucket - 1);
  // move [index+1...event_count-1] to [index...event_count-2]
  memmove(events + index, events + index + 1,
          sizeof(ext_event_t) * (event_count - index - 1));
  if (step < step_count) {
    cur_event_idx--;
  }
  --event_count;
}

uint16_t ExtSeqTrack::add_event(uint8_t step, ext_event_t *e) {
  uint8_t u = timing_buckets.get(step);
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
  timing_buckets.set(step, u + 1);
  // move [idx...event_count-1] to [idx+1...event_count]
  memmove(events + idx + 1, events + idx,
          sizeof(ext_event_t) * (event_count - idx));
  events[idx] = *e;
  if (step < step_count) {
    cur_event_idx++;
  }
  ++event_count;
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
  uint8_t timing_mid = get_timing_mid_inline();

  uint8_t find_array[NUM_LOCKS] = {0};

  uint16_t curidx, ev_end;

  curidx = locks_slides_idx;

  // Because, we support two lock values of same lock_idx per step.
  // Slide -> from last lock event on start step to first lock event on
  // destination step
  for (int8_t n = timing_buckets.get(step) - 1; n >= 0; n--) {
    auto &e = events[curidx + n];
    if (e.is_lock && e.event_on) {
      find_array[e.lock_idx] = 1;
    }
  }

  find_next_locks(curidx, step, find_array);

  ext_event_t *e;
  for (int8_t n = timing_buckets.get(step) - 1; n >= 0; n--) {
    e = &events[curidx + n];
    uint8_t c = e->lock_idx;
    if (!e->is_lock || !e->event_on || !locks_params[c] || locks_params[c] - 1 > 127)
      continue;

    uint8_t next_lockstep = locks_slide_next_lock_step[c];

    if (step == next_lockstep) {
      locks_slide_data[c].init();
      continue;
    }
    x0 = step * timing_mid + e->micro_timing - timing_mid + 1;
    if (next_lockstep < step) {
      x1 = (length + next_lockstep) * timing_mid +
           locks_slide_next_lock_utiming[c] - timing_mid - 1;
    } else {
      x1 = next_lockstep * timing_mid + locks_slide_next_lock_utiming[c] -
           timing_mid - 1;
    }
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
    end += timing_buckets.get(next_step);
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

  curidx += timing_buckets.get(step);
  uint8_t next_step = step + 1;

  uint16_t end = curidx;
  uint8_t max_len = length;

again:
  for (; next_step < max_len; next_step++) {
    end += timing_buckets.get(next_step);
    for (; curidx < end; curidx++) {
      if (count == 0) {
        return;
      }
      auto &e = events[curidx];
      if (!e.is_lock)
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
    ev_end += timing_buckets.get(j);
  } while (j != step);
end:
  ev_idx = 0xFFFF;
  return step;
}

uint8_t ExtSeqTrack::search_note_off(int8_t note_val, uint8_t step,
                                     uint16_t &ev_idx, uint16_t ev_end) {
  // Scan for matching note off;
  uint8_t j = step;
  ++ev_idx;

  do {
    for (; ev_idx != ev_end; ++ev_idx) {
      auto &ev = events[ev_idx];
      if (ev.is_lock || ev.event_value != note_val || ev.event_on) {
        continue;
      }
      return j;
    }
    ++j;
    if (j >= length) {
      // wrap around
      j = 0;
      ev_end = 0;
    }
    ev_idx = ev_end;
    ev_end += timing_buckets.get(j);
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

void ExtSeqTrack::add_notes_on(uint16_t x, uint8_t value, uint8_t velocity) {
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
  notes_on[slot].x = x;
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

  uint8_t timing_mid = get_timing_mid();

  uint8_t step = (cur_x / timing_mid);
  uint8_t start_utiming = timing_mid + cur_x - (step * timing_mid);

  uint8_t end_step = ((cur_x + cur_w) / timing_mid);
  uint8_t end_utiming = timing_mid + (cur_x + cur_w) - (end_step * timing_mid);

  if (end_step == step) {
    DEBUG_DUMP("ALERT start == end");
    end_step = end_step + 1;
    //    end_utiming = 2 * timing_mid - end_utiming;
    end_utiming -= timing_mid;
  }

  if (end_step >= length) {
    end_step = end_step - length;
  }

  uint16_t ev_idx;
  uint16_t note_idx = find_midi_note(step, cur_y, ev_idx, /*event_on*/ true);
  if (note_idx != 0xFFFF) {
    DEBUG_DUMP("abort note on");
    return;
  }

  ev_idx = 0;
  note_idx = find_midi_note(end_step, cur_y, ev_idx, /*event_on*/ false);
  if (note_idx != 0xFFFF) {
    DEBUG_DUMP("abort note off");
    return;
  }

  set_track_step(step, start_utiming, cur_y, true, velocity, cond);
  set_track_step(end_step, end_utiming, cur_y, false, velocity, 0);
}

bool ExtSeqTrack::del_note(uint16_t cur_x, uint16_t cur_w, uint8_t cur_y) {
  DEBUG_DUMP("del_note");
  DEBUG_DUMP(cur_x);
  DEBUG_DUMP(cur_w);
  uint8_t timing_mid = get_timing_mid();
  uint8_t step = (cur_x / timing_mid);

  // uint8_t end_step = ((cur_x + cur_w) / timing_mid);
  // uint8_t end_utiming = timing_mid + (cur_x + cur_w) - (end_step *
  // timing_mid);

  uint16_t note_idx_off, note_idx_on;
  bool note_on_found = false;
  uint16_t ev_idx, ev_end;
  uint16_t note_start, note_end;
  bool ret = false;

  for (int i = 0; i < length; i++) {

  again:
    note_idx_on = find_midi_note(i, cur_y, ev_idx, /*event_on*/ true);
    ev_end = ev_idx + timing_buckets.get(i);

    if (note_idx_on != 0xFFFF) {
      note_on_found = true;
      note_idx_off = note_idx_on;
      uint8_t j = search_note_off(cur_y, i, note_idx_off, ev_end);
      if (note_idx_off != 0xFFFF) {
        auto &ev = events[note_idx_on];
        auto &ev_j = events[note_idx_off];
        note_start = i * timing_mid + ev.micro_timing - timing_mid;
        note_end = j * timing_mid + ev_j.micro_timing - timing_mid;
        if (note_end < note_start) {
          note_end += length * timing_mid;
        }
        if ((note_start <= cur_x + cur_w) && (note_end > cur_x)) {
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
      DEBUG_DUMP("Wrap");
      // Remove wrap around notes
      auto &ev = events[note_idx_off];
      uint16_t note_end = i * timing_mid + ev.micro_timing - timing_mid;
      if (note_end > cur_x) {
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

void ExtSeqTrack::reset_params() {
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      uint8_t param = locks_params[c] - 1;
      if (param < 128) {
        uart->sendCC(channel, param, locks_params_orig[c]);
      }
    }
  }
}

void ExtSeqTrack::handle_event(uint16_t index, uint8_t step) {
  auto &ev = events[index];
  if (ev.is_lock) {
    // plock
    uint8_t param = locks_params[ev.lock_idx] - 1;
    if (param == 128) {
      uart->sendProgramChange(channel, ev.event_value);
    } else {
      uart->sendCC(channel, param, ev.event_value);
      // event_on == lock slide
      if (ev.event_on) {
        locks_slides_recalc = step;
      }
    }
  } else {
    // midi note
    if (ev.event_on) {
      noteon_conditional(ev.cond_id, ev.event_value, velocities[step]);
    } else {
      note_off(ev.event_value, 0);
    }
  }
}

void ExtSeqTrack::seq() {

  if (count_down) {
    count_down--;
    if (count_down == 0) {
      reset();
    }
  }

  uint8_t timing_mid = get_timing_mid_inline();
  uint16_t ev_idx, ev_end;

  if ((count_down == 0) && (mute_state == SEQ_MUTE_OFF)) {

    // the range we're interested in:
    // [current timing bucket, micro >= timing_mid ... next timing bucket, micro
    // < timing_mid]

    ev_idx = cur_event_idx;
    ev_end = cur_event_idx + timing_buckets.get(step_count);

    send_slides(locks_params, channel);

    // Go over CURRENT
    for (; ev_idx != ev_end; ++ev_idx) {
      auto u = events[ev_idx].micro_timing;
      if (u >= timing_mid && u - timing_mid == mod12_counter) {
        handle_event(ev_idx, step_count);
      }
    }

    // Locate NEXT
    uint8_t next_step = 0;
    if (step_count == length) {
      next_step = 0;
      ev_idx = 0;
    } else {
      next_step = step_count + 1;
    }
    ev_end = ev_idx + timing_buckets.get(next_step);

    // Go over NEXT
    for (; ev_idx != ev_end; ++ev_idx) {
      auto u = events[ev_idx].micro_timing;
      if (u < timing_mid && u == mod12_counter) {
        handle_event(ev_idx, next_step);
      }
    }
  }
  mod12_counter++;

  locks_slides_idx = cur_event_idx;

  if (mod12_counter == timing_mid) {
    cur_event_idx += timing_buckets.get(step_count);
    mod12_counter = 0;
    step_count_inc();
  }
}

void ExtSeqTrack::note_on(uint8_t note, uint8_t velocity) {
  uart->sendNoteOn(channel, note, velocity);
  // Greater than 64
  if (IS_BIT_SET(note, 6)) {
    SET_BIT64(note_buffer[1], note - 64);
  } else {
    SET_BIT64(note_buffer[0], note);
  }
}

void ExtSeqTrack::note_off(uint8_t note, uint8_t velocity) {
  uart->sendNoteOff(channel, note, velocity);

  // Greater than 64
  if (IS_BIT_SET(note, 6)) {
    CLEAR_BIT64(note_buffer[1], note - 64);
  } else {
    CLEAR_BIT64(note_buffer[0], note);
  }
}

void ExtSeqTrack::noteon_conditional(uint8_t condition, uint8_t note,
                                     uint8_t velocity) {
  if (IS_BIT_SET128(oneshot_mask, step_count)) {
    return;
  }
  if (condition > 64) {
    condition -= 64;
  }

  switch (condition) {
  case 0:
  case 1:
    note_on(note, velocity);
    break;
  case 2:
    if (!IS_BIT_SET(iterations_8, 0)) {
      note_on(note, velocity);
    }
    break;
  case 3:
    if ((iterations_6 == 3) || (iterations_6 == 6)) {
      note_on(note, velocity);
    }
    break;
  case 6:
    if (iterations_6 == 6) {
      note_on(note, velocity);
    }
    break;
  case 4:
    if ((iterations_8 == 4) || (iterations_8 == 8)) {
      note_on(note, velocity);
    }
    break;
  case 8:
    if (iterations_8 == 8) {
      note_on(note, velocity);
    }
  case 5:
    if (iterations_5 == 5) {
      note_on(note, velocity);
    }
    break;
  case 7:
    if (iterations_7 == 7) {
      note_on(note, velocity);
    }
    break;
  case 9:
    if (get_random_byte() <= 13) {
      note_on(note, velocity);
    }
    break;
  case 10:
    if (get_random_byte() <= 32) {
      note_on(note, velocity);
    }
    break;
  case 11:
    if (get_random_byte() <= 64) {
      note_on(note, velocity);
    }
    break;
  case 12:
    if (get_random_byte() <= 96) {
      note_on(note, velocity);
    }
    break;
  case 13:
    if (get_random_byte() <= 115) {
      note_on(note, velocity);
    }
    break;
  case 14:
    if (!IS_BIT_SET128(oneshot_mask, step_count)) {
      SET_BIT128(oneshot_mask, step_count);
      note_on(note, velocity);
    }
  }
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
  uint8_t utiming = (mod12_counter + get_timing_mid());
  if (step_count >= length) {
    return;
  }
  // clear all locks on step
  clear_track_locks(step_count, track_param, 255);
  set_track_locks(step_count, utiming, track_param, value, slide);
}

bool ExtSeqTrack::del_track_locks(int16_t cur_x, uint8_t lock_idx,
                                  uint8_t value) {
  uint8_t timing_mid = get_timing_mid();
  uint8_t step = (cur_x / timing_mid);

  if (step != 0) {
    --step;
  }
  uint16_t start_idx = 0;
  uint16_t end = 0;
  locate(step, start_idx, end);
  end = start_idx;
  bool ret = false;

  uint8_t r = 2;

  for (uint8_t n = step; n < min(length, step + 3); n++) {
    end += timing_buckets.get(n);
    for (; start_idx < end; start_idx++) {
      DEBUG_DUMP(start_idx);
      uint8_t i = start_idx;
      if (!events[i].is_lock || events[i].lock_idx != lock_idx ||
          (events[i].event_value > value + r ||
           events[i].event_value < min(0, value - r)))
        continue;
      int16_t event_x = n * timing_mid + events[i].micro_timing - timing_mid;
      if (event_x == cur_x || (event_x <= cur_x + r && event_x >= cur_x - r)) {
        remove_event(i);
        ret = true;
      }
    }
  }
  return ret;
}

void ExtSeqTrack::clear_track_locks(uint8_t idx) {
  for (uint8_t n = 0; n < length; n++) {
    clear_track_locks_idx(n, idx, 255);
  }
}

bool ExtSeqTrack::clear_track_locks(uint8_t step, uint8_t track_param,
                                    uint8_t value) {
  uint8_t lock_idx = find_lock_idx(track_param);
  return clear_track_locks_idx(step, lock_idx, value);
}

bool ExtSeqTrack::clear_track_locks_idx(uint8_t step, uint8_t lock_idx,
                                        uint8_t value) {
  uint16_t start_idx, end;
  locate(step, start_idx, end);
  bool ret = false;

  for (uint16_t i = start_idx; i != end; ++i) {
    if (!events[i].is_lock || events[i].lock_idx != lock_idx) {
      continue;
    }
    if (value == 255 || (events[i].event_value == value)) {
      remove_event(i);
      ret = true;
      if (value != 255) {
        break;
      }
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

    uint16_t ev_idx;
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
    if (track_param > 127) {
      event_on = false;
    }

    ext_event_t new_event;
    e = &new_event;
    DEBUG_DUMP("adding lock");
    DEBUG_DUMP(lock_idx);

    e->is_lock = true;
    e->cond_id = 0;
    e->lock_idx = lock_idx;
    e->event_value = value;
    e->event_on = event_on;
    e->micro_timing = utiming;

    if (add_event(step, e) == 0xFFFF) {
      return false;
    }

    return true;
  }
  return false;
}

bool ExtSeqTrack::set_track_step(uint8_t &step, uint8_t utiming,
                                 uint8_t note_num, uint8_t event_on,
                                 uint8_t velocity, uint8_t cond) {
  ext_event_t e;

  e.is_lock = false;
  e.cond_id = cond;
  e.event_value = note_num;
  e.event_on = event_on;
  e.micro_timing = utiming;

  if (add_event(step, &e) == 0xFFFF) {
    return false;
  }
  if (event_on || velocities[step] == 0) {
    velocities[step] = velocity;
  }
  return true;
}

void ExtSeqTrack::record_track_noteoff(uint8_t note_num) {

  uint8_t timing_mid = get_timing_mid();
  uint8_t utiming = (mod12_counter + timing_mid);

  uint8_t condition = 0;

  uint8_t step = step_count;
  uint16_t ev_idx;

  uint8_t n = find_notes_on(note_num);
  if (n == 255)
    return;

  uint16_t start_x = notes_on[n].x;
  uint16_t end_x = step * timing_mid + utiming - timing_mid;

  if (end_x < start_x) {
    del_note(0, end_x, note_num);
    end_x += length * timing_mid;
  }
  uint16_t w = end_x - start_x;

  del_note(start_x, w, note_num);
  add_note(start_x, w, note_num, notes_on[n].velocity);

  notes_on[n].value = 255;
  notes_on_count--;

  return;
}

void ExtSeqTrack::record_track_noteon(uint8_t note_num, uint8_t velocity) {

  uint8_t timing_mid = get_timing_mid();

  uint8_t utiming = (mod12_counter + get_timing_mid());
  uint8_t condition = 0;

  add_notes_on(step_count * timing_mid + utiming - timing_mid, note_num,
               velocity);

  return;
}

void ExtSeqTrack::clear_ext_conditional() {
  for (uint16_t x = 0; x < NUM_EXT_EVENTS; x++) {
    events[x].cond_id = 0;
    events[x].micro_timing = 0; // XXX zero or mid?
  }
}

void ExtSeqTrack::clear_ext_notes() {
  event_count = 0;
  for (uint8_t c = 0; c < NUM_EXT_STEPS; c++) {
    timing_buckets.set(c, 0);
  }
}

void ExtSeqTrack::clear_track() {
  clear_ext_notes();
  clear_ext_conditional();
  buffer_notesoff();
}

void ExtSeqTrack::modify_track(uint8_t dir) {
  uint8_t old_mute_state = mute_state;
  uint8_t n_cur;
  ext_event_t ev_cur[16];
  uint8_t vel_tmp;

  mute_state = SEQ_MUTE_ON;
  buffer_notesoff();

  uint8_t timing_mid = get_timing_mid();

  uint16_t ev_idx, ev_end;
  locate(length, ev_idx, ev_end);

  switch (dir) {
  case DIR_LEFT:
    n_cur = timing_buckets.get(0);
    memcpy(ev_cur, events, sizeof(ext_event_t) * n_cur);

    memmove(events, events + n_cur, sizeof(ext_event_t) * (ev_end - n_cur));

    memcpy(events + ev_end - n_cur, ev_cur, sizeof(ext_event_t) * n_cur);

    vel_tmp = velocities[0];
    memmove(velocities, velocities + 1, length - 1);
    velocities[length - 1] = vel_tmp;

    timing_buckets.shift_left(length);
    break;
  case DIR_RIGHT:
    n_cur = timing_buckets.get(length - 1);
    memcpy(ev_cur, events + ev_end - n_cur, sizeof(ext_event_t) * n_cur);
    memmove(events + n_cur, events, sizeof(ext_event_t) * (ev_end - n_cur));
    memcpy(events, ev_cur, sizeof(ext_event_t) * n_cur);
    vel_tmp = velocities[length - 1];
    memmove(velocities + 1, velocities, length - 1);
    velocities[0] = vel_tmp;

    timing_buckets.shift_right(length);
    break;
  case DIR_REVERSE:
    uint16_t end = ev_end / 2;
    uint8_t timing_mid = get_timing_mid();
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

      events[i].micro_timing = timing_mid * 2 - events[i].micro_timing;
      events[j].micro_timing = timing_mid * 2 - events[j].micro_timing;
    }
    for (uint8_t n = 0; n < length / 2; n++) {
      uint8_t vel_tmp = velocities[n];
      uint8_t z = length - 1 - n;
      velocities[n] = velocities[z];
      velocities[z] = vel_tmp;
    }
    // reverse timing buckets
    timing_buckets.reverse(length);
    break;
  }

  locate(step_count, cur_event_idx, ev_end);

  oneshot_mask[0] = 0;
  oneshot_mask[1] = 0;
  mute_state = old_mute_state;
}
