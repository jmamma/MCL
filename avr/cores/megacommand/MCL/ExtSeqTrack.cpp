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
  while (step_count >= length) {
    step_count = length % step_count;
  }
  DEBUG_DUMP(step_count);
}

void ExtSeqTrack::re_sync() {
  uint16_t q = length;
  start_step = (MidiClock.div16th_counter / q) * q + q;
  start_step_offset = 0;
  mute_until_start = true;
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

  ++event_count;
  return idx;
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

void ExtSeqTrack::add_notes_on(uint16_t x, uint8_t value) {
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
  DEBUG_DUMP("adding notes_on");
  DEBUG_DUMP(slot);
  DEBUG_DUMP(x);
  DEBUG_DUMP(value);
  notes_on[slot].value = value;
  notes_on[slot].x = x;

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

void ExtSeqTrack::add_note(uint16_t cur_x, uint16_t cur_w, uint8_t cur_y) {

  uint8_t timing_mid = get_timing_mid();

  uint8_t start_step = (cur_x / timing_mid);
  uint8_t start_utiming = timing_mid + cur_x - (start_step * timing_mid);

  uint8_t end_step = ((cur_x + cur_w) / timing_mid);
  uint8_t end_utiming = timing_mid + (cur_x + cur_w) - (end_step * timing_mid);

  if (end_step == start_step) {
    DEBUG_DUMP("ALERT start == end");
    end_step = end_step + 1;
    //    end_utiming = 2 * timing_mid - end_utiming;
    end_utiming -= timing_mid;
  }

  if (end_step >= length) {
    end_step = end_step - length;
  }

  uint16_t ev_idx;
  uint16_t note_idx =
      find_midi_note(start_step, cur_y, ev_idx, /*event_on*/ true);
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

  set_ext_track_step(start_step, start_utiming, cur_y, true);
  set_ext_track_step(end_step, end_utiming, cur_y, false);
}

bool ExtSeqTrack::del_note(uint16_t cur_x, uint16_t cur_w, uint8_t cur_y) {
  DEBUG_DUMP("del_note");
  DEBUG_DUMP(cur_x);
  DEBUG_DUMP(cur_w);
  uint8_t timing_mid = get_timing_mid();
  uint8_t start_step = (cur_x / timing_mid);

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

void ExtSeqTrack::handle_event(uint16_t index) {
  auto &ev = events[index];
  if (ev.is_lock) {
    // plock
    if (ev.event_on) {
      // lock engage
    } else {
      // lock disengage
    }
  } else {
    // midi note
    if (ev.event_on) {
      noteon_conditional(ev.cond_id, ev.event_value);
    } else {
      note_off(ev.event_value);
    }
  }
}

void ExtSeqTrack::seq() {
  if (mute_until_start) {

    if (clock_diff(MidiClock.div16th_counter, start_step) == 0) {
      if (MidiClock.mod12_counter == start_step_offset) {
        reset();
      }
    }
  }
  uint8_t timing_mid = get_timing_mid_inline();
  if ((MidiUart2.uart_block == 0) && (mute_until_start == false) &&
      (mute_state == SEQ_MUTE_OFF)) {

    // the range we're interested in:
    // [current timing bucket, micro >= timing_mid ... next timing bucket, micro
    // < timing_mid]

    uint16_t ev_idx, ev_end;

    // Locate CURRENT
    locate(step_count, ev_idx, ev_end);

    // Go over CURRENT
    for (; ev_idx != ev_end; ++ev_idx) {
      auto u = events[ev_idx].micro_timing;
      if (u >= timing_mid && u - timing_mid == mod12_counter) {
        handle_event(ev_idx);
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
        handle_event(ev_idx);
      }
    }
  }
  mod12_counter++;

  if (mod12_counter == timing_mid) {
    mod12_counter = 0;
    step_count_inc();
  }
}

void ExtSeqTrack::note_on(uint8_t note) {
  uart->sendNoteOn(channel, note, 100);
  // Greater than 64
  if (IS_BIT_SET(note, 6)) {
    SET_BIT64(note_buffer[1], note - 64);
  } else {
    SET_BIT64(note_buffer[0], note);
  }
}

void ExtSeqTrack::note_off(uint8_t note) {
  uart->sendNoteOff(channel, note, 0);

  // Greater than 64
  if (IS_BIT_SET(note, 6)) {
    CLEAR_BIT64(note_buffer[1], note - 64);
  } else {
    CLEAR_BIT64(note_buffer[0], note);
  }
}

void ExtSeqTrack::noteon_conditional(uint8_t condition, uint8_t note) {
  if (condition > 64) {
    condition -= 64;
  }

  switch (condition) {
  case 0:
  case 1:
    if (!IS_BIT_SET128(oneshot_mask, step_count)) {
      note_on(note);
    }
    break;
  case 2:
    if (!IS_BIT_SET(iterations_8, 0)) {
      note_on(note);
    }
    break;
  case 3:
    if ((iterations_6 == 3) || (iterations_6 == 6)) {
      note_on(note);
    }
    break;
  case 6:
    if (iterations_6 == 6) {
      note_on(note);
    }
    break;
  case 4:
    if ((iterations_8 == 4) || (iterations_8 == 8)) {
      note_on(note);
    }
    break;
  case 8:
    if (iterations_8 == 8) {
      note_on(note);
    }
  case 5:
    if (iterations_5 == 5) {
      note_on(note);
    }
    break;
  case 7:
    if (iterations_7 == 7) {
      note_on(note);
    }
    break;
  case 9:
    if (get_random_byte() <= 13) {
      note_on(note);
    }
    break;
  case 10:
    if (get_random_byte() <= 32) {
      note_on(note);
    }
    break;
  case 11:
    if (get_random_byte() <= 64) {
      note_on(note);
    }
    break;
  case 12:
    if (get_random_byte() <= 96) {
      note_on(note);
    }
    break;
  case 13:
    if (get_random_byte() <= 115) {
      note_on(note);
    }
    break;
  case 14:
    if (!IS_BIT_SET128(oneshot_mask, step_count)) {
      SET_BIT128(oneshot_mask, step_count);
      note_on(note);
    }
  }
}

bool ExtSeqTrack::set_ext_track_step(uint8_t &step, uint8_t utiming,
                                     uint8_t note_num, uint8_t event_on) {
  ext_event_t e;

  e.is_lock = false;
  e.cond_id = 0;
  e.event_value = note_num;
  e.event_on = event_on;
  e.micro_timing = utiming;

  add_event(step, &e);
  return true;
}

void ExtSeqTrack::record_ext_track_noteoff(uint8_t note_num, uint8_t velocity) {

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
    end_x += length * mod12_counter;
  }

  uint16_t w = end_x - start_x;

  del_note(start_x, w, note_num);
  add_note(start_x, w, note_num);

  notes_on[n].value = 255;
  notes_on_count--;

  return;
}

void ExtSeqTrack::record_ext_track_noteon(uint8_t note_num, uint8_t velocity) {

  uint8_t timing_mid = get_timing_mid();

  uint8_t utiming = (mod12_counter + get_timing_mid());
  uint8_t condition = 0;

  add_notes_on(step_count * timing_mid + utiming - timing_mid, note_num);

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

  uint8_t n_cur;
  ext_event_t ev_cur[16];

  switch (dir) {
  case DIR_LEFT:
    n_cur = timing_buckets.get(0);
    memcpy(ev_cur, events + n_cur, sizeof(ext_event_t) * n_cur);
    memmove(events, events + n_cur,
            sizeof(ext_event_t) * (event_count - n_cur));
    memcpy(events + event_count - n_cur, ev_cur, sizeof(ext_event_t) * n_cur);
    timing_buckets.shift_left(length);
    break;
  case DIR_RIGHT:
    n_cur = timing_buckets.get(length - 1);
    memcpy(ev_cur, events + event_count - n_cur, sizeof(ext_event_t) * n_cur);
    memmove(events + n_cur, events,
            sizeof(ext_event_t) * (event_count - n_cur));
    memcpy(events, ev_cur, sizeof(ext_event_t) * n_cur);
    timing_buckets.shift_right(length);
    break;
  case DIR_REVERSE:
    uint16_t end = event_count / 2;
    for (uint16_t i = 0; i < end; ++i) {
      auto tmp = events[i];
      auto j = event_count - 1 - i;
      events[i] = events[j];
      events[j] = tmp;

      // need to flip note on/off
      if (!events[i].is_lock) {
        events[i].event_on = !events[i].event_on;
      }

      if (!events[j].is_lock) {
        events[j].event_on = !events[j].event_on;
      }
    }
    timing_buckets.reverse(length);
    break;
  }

  oneshot_mask[0] = 0;
  oneshot_mask[1] = 0;
}
