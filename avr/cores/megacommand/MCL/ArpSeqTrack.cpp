#include "MCL_impl.h"

//length will determine retrig speed.
//arp position (arp_idx) is independent of length

void ArpSeqTrack::set_speed(uint8_t speed_) {
  speed = speed_;
  uint8_t timing_mid = get_timing_mid();
  if (mod12_counter > timing_mid) {
    mod12_counter = mod12_counter - (mod12_counter / timing_mid) * timing_mid;
  }
}

void ArpSeqTrack::seq(MidiUartParent *uart_) {
  uart = uart_;
  uint8_t timing_mid = get_timing_mid_inline();
  
  if (mod12_counter == 0 && arp_enabled) {
    if ((arp_len > 0)) {
      switch (active) {
        case MD_ARP_TRACK_TYPE:
          seq_ptc_page.trig_md(arp_notes[arp_idx],uart);
          break;
        case EXT_ARP_TRACK_TYPE:
          mcl_seq.ext_tracks[last_ext_track].note_on(arp_notes[arp_idx], 127, uart);
          break;
      }
      arp_idx++;
      if (arp_idx == arp_len) {
        arp_idx = 0;
      }   
    }   
  }
  
  step_count_inc();
  if (mod12_counter == timing_mid) {
    mod12_counter = 0;
  }

}

#define NOTE_RANGE 24

uint8_t ArpSeqTrack::get_next_note_up(int8_t cur) {

  for (int8_t i = cur + 1; i < NOTE_RANGE; i++) {
    if (IS_BIT_SET32(note_mask, i)) {
      return i;
    }    
  }
  return 255; 
}

void ArpSeqTrack::render(uint8_t mode_, uint8_t oct_, uint32_t note_mask_) {
 DEBUG_PRINT_FN();
  if (!arp_enabled) {
    return;
  }
  note_mask = note_mask_;
  oct = oct_;
  mode = mode_;

  arp_len = 0;

  uint8_t num_of_notes = 0;
  uint8_t note = 0;
  uint8_t b = 0;

  uint8_t sort_up[NOTE_RANGE];
  uint8_t sort_down[NOTE_RANGE];

  // Collect notes, sort in ascending order
  for (int8_t i = 0; i < NOTE_RANGE && note != 255; i++) {
    note = get_next_note_up(i - 1);
    if (note == 255) { break; }
    num_of_notes++;
    sort_up[i] = note;
  }
  if (num_of_notes == 0) {
    return;
  }
  // Sort notes in descending order

  for (uint8_t i = 0; i < num_of_notes; i++) {
    sort_down[num_of_notes - i - 1] = sort_up[i];
  }
  note = 255;

  switch (mode) {
  case ARP_RND:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[random(0, num_of_notes)];
      arp_notes[arp_len++] = note + 12 * random(0, oct);
    }
    break;

  case ARP_UP2:
  case ARP_UPP:
  case ARP_UP:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[i] = note;
      arp_len++;
    }
    break;
  case ARP_DOWN2:
  case ARP_DOWNP:
  case ARP_DOWN:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[i] = note;
      arp_len++;
    }
    break;

  case ARP_UPDOWN:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[i] = note;
      arp_len++;
    }
    for (uint8_t i = 1; i < num_of_notes - 1; i++) {
      note = sort_down[i];
      arp_notes[arp_len] = note;
      arp_len++;
    }
    break;
  case ARP_DOWNUP:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[i] = note;
      arp_len++;
    }
    for (uint8_t i = 1; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      arp_notes[arp_len] = note;
      arp_len++;
    }

    break;
  case ARP_UPNDOWN:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[i] = note;
      arp_len++;
    }
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[arp_len] = note;
      arp_len++;
    }
    break;
  case ARP_DOWNNUP:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[i] = note;
      arp_len++;
    }
    for (uint8_t i = 0; i < num_of_notes; i++) {
      note = sort_up[i];
      arp_notes[arp_len] = note;
      arp_len++;
    }
    break;
  case ARP_CONV:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      arp_notes[i] = note;
      arp_len++;
    }
    break;
  case ARP_DIV:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_up[b];
        b++;
      } else {
        note = sort_down[b];
      }
      arp_notes[i] = note;
      arp_len++;
    }
    break;
  case ARP_CONVDIV:
    for (uint8_t i = 0; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      arp_notes[i] = note;
      arp_len++;
    }
    b = 0;
    for (uint8_t i = 1; i < num_of_notes; i++) {
      if (i & 1) {
        note = sort_down[b];
        b++;
      } else {
        note = sort_up[b];
      }
      arp_notes[i] = note;
      arp_len++;
    }
    break;
  case ARP_PINKUP:
    if (num_of_notes == 1) {
      note = sort_up[0];
      arp_notes[arp_len++] = note;
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      arp_notes[arp_len++] = note;
      note = sort_down[0];
      arp_notes[arp_len++] = note;
    }

    break;

  case ARP_PINKDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[arp_len++] = note;
      note = sort_down[0];
      arp_notes[arp_len++] = note;
    }
    break;

  case ARP_THUMBUP:
    if (num_of_notes == 1) {
      note = sort_down[0];
      arp_notes[arp_len++] = note;
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      arp_notes[arp_len++] = note;
      note = sort_down[0];
      arp_notes[arp_len++] = note;
    }

    break;

  case ARP_THUMBDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_down[i];
      arp_notes[arp_len++] = note;
      note = sort_down[0];
      arp_notes[arp_len++] = note;
    }
    break;
  }

  // Generate subsequent octave itterations
  for (uint8_t n = 0; n < oct; n++) {
    for (uint8_t i = 0; i < num_of_notes && arp_len < ARP_MAX_NOTES; i++) {
      switch (mode) {
      case ARP_UP2:
      case ARP_DOWN2:
        arp_notes[arp_len] = arp_notes[i];
        if (!(i & 1)) {
          arp_notes[arp_len] += (n + 1) * 12;
        }
        break;
      case ARP_UPP:
      case ARP_DOWNP:
        arp_notes[arp_len] = arp_notes[i];
        if (i == num_of_notes - 1) {
          arp_notes[arp_len] += (n + 1) * 12;
        }
        break;
      default:
        arp_notes[arp_len] = arp_notes[i] + (n + 1) * 12;
        break;
      }
      arp_len++;
    }
  }

  if (arp_idx >= arp_len) {
    arp_idx = arp_len - 1;
  }

}


