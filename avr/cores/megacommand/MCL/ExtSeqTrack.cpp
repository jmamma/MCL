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
  if (step_count >= length) {
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

  //Insertion sort

  if (e->is_lock) {
  //Insert in slot 0
  } else {
    if (!e->event_on) {
      //If note off, insert in first position after locks
      for (; idx != end; idx++) {
        if (!events[idx].is_lock && !events[idx].event_on) {
          break;
        }
      }
    }
    else {
    //If note on, insert at end
    idx = end;
    }
  }

  timing_buckets.set(step, u + 1);
  // move [end...event_count-1] to [end+1...event_count]
  memmove(events + idx + 1, events + idx,
          sizeof(ext_event_t) * (event_count - idx));

  memcpy(events + idx, e, sizeof(ext_event_t));

  ++event_count;

  return idx;
}

uint16_t ExtSeqTrack::find_midi_note(uint8_t step, uint8_t note_num,
                                     uint16_t &idx, bool event_on) {
  uint16_t end;
  locate(step, idx, end);
  for (uint16_t i = idx; i != end; ++i) {
    if (events[i].is_lock || events[i].event_value != note_num ||
        events[i].event_on != event_on) {
      continue;
    }
    return i;
  }
  return 0xFFFF;
}

uint16_t ExtSeqTrack::find_midi_note(uint8_t step, uint8_t note_num,
                                     uint16_t &idx) {
  uint16_t end;
  locate(step, idx, end);
  for (uint16_t i = idx; i != end; ++i) {
    if (events[i].is_lock || events[i].event_value != note_num) {
      continue;
    }
    return i;
  }
  return 0xFFFF;
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
  DEBUG_PRINTLN(F("note on"));
  DEBUG_DUMP(note);
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

void ExtSeqTrack::set_ext_track_step(uint8_t step, uint8_t utiming,
                                     uint8_t note_num, uint8_t event_on) {
  // Look for matching note already on this step
  // If it's a note off, then disable the note
  // If it's a note on, set the note note-off.
  uint16_t ev_idx;
  uint16_t note_idx = find_midi_note(step, note_num, ev_idx, event_on);

  ext_event_t e;

  e.is_lock = false;
  e.cond_id = 0;
  e.event_value = note_num;
  e.event_on = event_on;
  e.micro_timing = utiming;


  if (note_idx == 0xFFFF) {
    ev_idx = add_event(step, &e);
  } else {
    memcpy(events + note_idx, &e, sizeof(ext_event_t));
  }

}

void ExtSeqTrack::record_ext_track_noteoff(uint8_t note_num, uint8_t velocity) {

  uint8_t timing_mid = get_timing_mid() - 1;
  uint8_t utiming = (mod12_counter + timing_mid);

  uint8_t condition = 0;

  uint8_t step = step_count;
  uint16_t ev_idx;
  uint16_t note_idx = find_midi_note(step, note_num, ev_idx);
  if (note_idx != 0xFFFF) {
    if (events[note_idx].event_on) {
      // if current step already has this note, then we'll use the next step
      // over
      step = step + 1;
      if (step > length) {
        step = 0;
      }
      utiming = (timing_mid - mod12_counter);
    } else {
      // note off event already on this step
      return;
    }
  }


  ext_event_t e;

  e.is_lock = false;
  e.cond_id = condition;
  e.event_value = note_num;
  e.event_on = false;
  e.micro_timing = utiming;


  add_event(step, &e);
  
  return;
}

void ExtSeqTrack::record_ext_track_noteon(uint8_t note_num, uint8_t velocity) {

  uint8_t utiming = (mod12_counter + get_timing_mid() - 1);
  uint8_t condition = 0;

  // Let's try and find an existing note
  uint16_t ev_idx;
  uint16_t note_idx = find_midi_note(step_count, note_num, ev_idx);

  if (note_idx != 0xFFFF && !events[note_idx].event_on) {
    note_idx = 0xFFFF;
  }


  ext_event_t e;

  e.is_lock = false;
  e.cond_id = 0;
  e.event_value = note_num;
  e.event_on = true;
  e.micro_timing = utiming;


  if (note_idx == 0xFFFF) {
   add_event(step_count, &e);
  } else {
    memcpy(events + note_idx, &e, sizeof(ext_event_t));
  }

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
