#include "MCL_impl.h"

//length will determine retrig speed.
//arp position (idx) is independent of length

void ArpSeqTrack::set_speed(uint8_t speed_) {
  speed = speed_;
  uint8_t timing_mid = get_timing_mid();
  if (mod12_counter > timing_mid) {
    mod12_counter = mod12_counter - (mod12_counter / timing_mid) * timing_mid;
  }
}

void ArpSeqTrack::set_length(uint8_t length_) {
  length = length_;
  while (step_count >= length && length > 0) {
    // re_sync();
    step_count = (step_count - length);
  }
}


void ArpSeqTrack::seq(MidiUartParent *uart_) {
  MidiUartParent *uart_old = uart;
  uart = uart_;

  uint8_t timing_mid = get_timing_mid_inline();

  if (mod12_counter == 0 && enabled) {
   if (step_count == 0) {
      if (len > 0) {
        switch (active) {
          case MD_ARP_TRACK_TYPE:
            seq_ptc_page.trig_md(notes[idx] + oct * 12, track_number, fine_tune, uart);
            break;
          case EXT_ARP_TRACK_TYPE:
            seq_ptc_page.note_on_ext(notes[idx] + oct * 12, 127, track_number, uart);
            last_note_on = notes[idx];
            break;
        }
        idx++;
        if (idx == len) {
          idx = 0;
        }
      }
    }
  }
  mod12_counter++;
  if (mod12_counter == timing_mid) {
    step_count_inc();
    if (active == EXT_ARP_TRACK_TYPE && last_note_on != 255 && step_count == length / 2) {
        seq_ptc_page.note_off_ext(last_note_on, 0, uart);
        last_note_on = 255;
    }
    mod12_counter = 0;
  }
  uart = uart_old;
}

#define NOTE_RANGE 128

uint8_t ArpSeqTrack::get_next_note_up(int8_t cur) {

  for (int8_t i = cur + 1; i < 128 && i >= 0; i++) {
    if (IS_BIT_SET128_P(note_mask, i)) {
      return i;
    }
  }
  return 255;
}

void ArpSeqTrack::render(uint8_t mode_, uint8_t oct_, uint8_t fine_tune_, uint8_t range_, uint64_t *note_mask_) {
  DEBUG_PRINT_FN();

  fine_tune = fine_tune_;
  range = range_;
  mode = mode_;
  len = 0;
  oct = oct_;

  if (!enabled) {
    return;
  }

  memcpy(note_mask, note_mask_, sizeof(note_mask));
  
  uint8_t num_of_notes = 0;
  uint8_t note = 0;
  uint8_t b = 0;

  uint8_t sort_up[ARP_MAX_NOTES];
  uint8_t sort_down[ARP_MAX_NOTES];

  // Collect notes, sort in ascending order
  note = get_next_note_up(-1);
  uint8_t last_note = note;
  if (note != 255) {
    num_of_notes++;
    sort_up[0] = min(127, note);
  }

  for (int8_t i = 1; i < ARP_MAX_NOTES && note != 255; i++) {
    note = get_next_note_up(last_note);
    if (note == 255) { break; }
    last_note = note;
    num_of_notes++;
    sort_up[i] = min(127, note);
  }
  if (num_of_notes == 0) {
    return;
  }
  // Sort notes in descending order

  for (uint8_t i = 0; i < num_of_notes; i++) {
    sort_down[num_of_notes - i - 1] = sort_up[i];
  }
  note = 255;

  for (uint8_t i = 0; i < num_of_notes; i++) {
    switch (mode) {
    case ARP_RND:
      note = sort_up[random(0, num_of_notes)] + 12 * random(0,range);
      break;
    case ARP_UP2:
    case ARP_UPP:
    case ARP_UP:
    case ARP_UPDOWN:
    case ARP_UPNDOWN:
      note = sort_up[i];
      break;
    case ARP_DOWN2:
    case ARP_DOWNP:
    case ARP_DOWN:
    case ARP_DOWNUP:
    case ARP_DOWNNUP:
      note = sort_down[i];
      break;
    case ARP_CONV:
    case ARP_CONVDIV:
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      break;
    case ARP_DIV:
      if (i & 1) {
        note = sort_up[b];
        b++;
      } else {
        note = sort_down[b];
      }
      break;
    default:
      goto next;
    }
    notes[i] = note;
    len++;
  }
  next:

  for (uint8_t i = 0; i < num_of_notes; i++) {
     switch (mode) {
      case ARP_UPNDOWN:
        note = sort_down[i];
        break;
      case ARP_DOWNNUP:
        note = sort_up[i];
        break;
      case ARP_CONVDIV:
        if (i & 1) {
          note = sort_up[b];
          b++;
        } else {
          note = sort_down[b];
        }
        break;
      default:
        goto next1;
    }
    notes[len] = note;
    len++;
  }
  next1:
  for (uint8_t i = 1; i < num_of_notes - 1; i++) {
    switch (mode) {
      case ARP_UPDOWN:
        note = sort_down[i];
        break;
      case ARP_DOWNUP:
        note = sort_up[i];
        break;
      default:
        goto next2;
    }
    notes[len] = note;
    len++;
  }
  next2:
  switch (mode) {
  case ARP_PINKUP:
    if (num_of_notes == 1) {
      note = sort_up[0];
      notes[len++] = note;
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      notes[len++] = note;
      note = sort_down[0];
      notes[len++] = note;
    }

    break;

  case ARP_PINKDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_down[i];
      notes[len++] = note;
      note = sort_up[0];
      notes[len++] = note;
    }
    break;

  case ARP_THUMBUP:
    if (num_of_notes == 1) {
      note = sort_down[0];
      notes[len++] = note;
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      notes[len++] = note;
      note = sort_down[0];
      notes[len++] = note;
    }

    break;

  case ARP_THUMBDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_down[i];
      notes[len++] = note;
      note = sort_down[0];
      notes[len++] = note;
    }
    break;
  }

  // Generate subsequent octave ranges.
  uint8_t old_len = len;
  for (uint8_t n = 0; n < range; n++) {
    for (uint8_t i = 0; i < old_len && len < ARP_MAX_NOTES; i++) {
      switch (mode) {
      case ARP_UP2:
      case ARP_DOWN2:
        notes[len] = notes[i];
        if (!(i & 1)) {
          notes[len] += (n + 1) * 12;
        }
        break;
      case ARP_UPP:
      case ARP_DOWNP:
        notes[len] = notes[i];
        if (i == num_of_notes - 1) {
          notes[len] += (n + 1) * 12;
        }
        break;
      default:
        notes[len] = notes[i] + (n + 1) * 12;
        break;
      }
      len++;
    }
  }

  if (idx >= len) {
    idx = len - 1;
  }

}


