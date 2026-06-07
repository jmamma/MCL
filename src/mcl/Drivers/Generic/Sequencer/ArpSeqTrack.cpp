#include "ArpSeqTrack.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

namespace {

#if defined(PLATFORM_TBD)
bool md_arp_targets_tbd() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}
#endif

} // namespace

//length will determine retrig speed.
//arp position (idx) is independent of length

void ArpSeqTrack::set_speed(uint8_t speed_) {
  speed = speed_;
  uint8_t ticks_per_step = get_ticks_per_step();
  if (ticks_per_step && mod12_counter >= ticks_per_step) {
    mod12_counter = mod12_counter % ticks_per_step;
  }
}

void ArpSeqTrack::set_length(uint8_t length_) {
  length = length_;
  while (step_count >= length && length > 0) {
    // re_sync();
    step_count = (step_count - length);
  }
}

uint8_t ArpSeqTrack::speed_for_parent_speed(uint8_t parent_speed) {
  return parent_speed == SEQ_SPEED_3_4X || parent_speed == SEQ_SPEED_3_2X
             ? SEQ_SPEED_3_2X
             : SEQ_SPEED_2X;
}

void ArpSeqTrack::load_data(const ArpSeqData &data,
                            const ArpSeqPhaseData &phase,
                            uint8_t parent_speed) {
  reset();
  static_cast<ArpSeqData &>(*this) = data;
  speed = speed_for_parent_speed(parent_speed);
  length = rate ? rate : 2;
  len = 0;
  idx = 0;
  last_note_on = 255;
#if defined(__AVR__)
  if (enabled && (note_mask[0] || note_mask[1])) {
    render(mode, oct, fine_tune, range, note_mask);
  } else {
    clear_notes();
  }
#else
  uint64_t loaded_note_mask[2] = {note_mask[0], note_mask[1]};
  if (enabled && (loaded_note_mask[0] || loaded_note_mask[1])) {
    render(mode, oct, fine_tune, range, loaded_note_mask);
  } else {
    clear_notes();
  }
#endif

  if (phase.valid() && len > 0) {
    idx = phase.idx < len ? phase.idx : 0;
    step_count = phase.step_count < length ? phase.step_count : 0;
    uint8_t ticks_per_step = speed == SEQ_SPEED_3_2X ? 8 : 6;
    uint8_t saved_mod12 = phase.mod12_counter;
    if (saved_mod12 != 255 && saved_mod12 >= ticks_per_step) {
      saved_mod12 = 0;
    }
    mod12_counter = saved_mod12;
  }
}

void ArpSeqTrack::store_data(ArpSeqData *data) const {
  if (data == nullptr) {
    return;
  }
  *data = static_cast<const ArpSeqData &>(*this);
  data->rate = length ? length : 2;
}

void ArpSeqTrack::store_phase_data(ArpSeqPhaseData &phase) const {
  phase.init();
  if (!enabled || len == 0 || (note_mask[0] == 0 && note_mask[1] == 0)) {
    return;
  }
  phase.idx = idx;
  phase.step_count = step_count;
  phase.mod12_counter = mod12_counter;
  phase.set_valid();
}

void ArpSeqTrack::seq(MidiUartClass *uart_, MidiUartClass *uart2_) {
  MidiUartClass *uart_old = uart;
  MidiUartClass *uart2_old = uart2;
  uart = uart_;
  uart2 = uart2_;

  if (count_down) {
    count_down--;
    if (count_down) {
      uart = uart_old;
      uart2 = uart2_old;
      return;
    }
  }

  uint8_t ticks_per_step = get_ticks_per_step_inline();

  mod12_counter++;
  if (mod12_counter == ticks_per_step) {
    step_count_inc();
    on_cycle_midpoint(uart_, uart2_);
    mod12_counter = 0;
  }
  if (mod12_counter == 0 && enabled && mute_state == SEQ_MUTE_OFF) {
   if (step_count == 0) {
      if (len > 0) {
        uint8_t note = mode == ARP_RND2 ? notes[get_random(len)] : notes[idx];
        note += oct*12;
        dispatch_note(note, uart_, uart2_);
        idx++;
        if (idx == len) {
          idx = 0;
        }
      }
    }
  }
  uart = uart_old;
  uart2 = uart2_old;
}

#define NOTE_RANGE 128

uint8_t ArpSeqTrack::get_next_note_up(int8_t cur) {

  for (uint8_t i = (uint8_t)(cur + 1); i != NOTE_RANGE; i++) {
    if (IS_BIT_SET128_P(note_mask, i)) {
      return i;
    }
  }
  return 255;
}

void ArpSeqTrack::render(uint8_t mode_, uint8_t oct_, uint8_t fine_tune_, uint8_t range_, const uint64_t *note_mask_) {
  DEBUG_PRINT_FN();
  uint8_t mute_state_old = mute_state;
  mute_state = SEQ_MUTE_ON;

  fine_tune = fine_tune_;
  range = range_;
  mode = mode_;
  len = 0;
  oct = oct_;

  if (!enabled) {
    mute_state = mute_state_old;
    return;
  }
  on_render_begin();

  memcpy(note_mask, note_mask_, sizeof(note_mask));
  uint8_t num_of_notes = 0;
  uint8_t note = 0;
  uint8_t b = 0;

  uint8_t sort_up[ARP_MAX_NOTES];

  // Collect notes, sort in ascending order
  int8_t last_note = -1;
  while (num_of_notes < ARP_MAX_NOTES) {
    note = get_next_note_up(last_note);
    if (note == 255) { break; }
    last_note = note;
    sort_up[num_of_notes++] = note;
  }
  if (num_of_notes == 0) {
    mute_state = mute_state_old;
    return;
  }
  uint8_t top_note = sort_up[num_of_notes - 1];

  for (uint8_t i = 0; i < num_of_notes; i++) {
    switch (mode) {
    case ARP_RND:
      note = sort_up[get_random(num_of_notes)] + 12 * get_random(range);
      break;
    case ARP_UP2:
    case ARP_UPP:
    case ARP_UP:
    case ARP_UPDOWN:
    case ARP_UPNDOWN:
    case ARP_RND2:
      note = sort_up[i];
      break;
    case ARP_DOWN2:
    case ARP_DOWNP:
    case ARP_DOWN:
    case ARP_DOWNUP:
    case ARP_DOWNNUP:
      note = sort_up[num_of_notes - i - 1];
      break;
    case ARP_CONV:
    case ARP_CONVDIV:
      if (i & 1) {
        note = sort_up[num_of_notes - b - 1];
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
        note = sort_up[num_of_notes - b - 1];
      }
      break;
    default:
      goto next;
    }
    notes[len] = note;
    len++;
  }
  next:

  for (uint8_t i = 0; i < num_of_notes; i++) {
     switch (mode) {
      case ARP_UPNDOWN:
        note = sort_up[num_of_notes - i - 1];
        break;
      case ARP_DOWNNUP:
        note = sort_up[i];
        break;
      case ARP_CONVDIV:
        if (i & 1) {
          note = sort_up[b];
          b++;
        } else {
          note = sort_up[num_of_notes - b - 1];
        }
        break;
      default:
        goto next1;
    }
    if (len >= ARP_MAX_NOTES) { break; }
    notes[len] = note;
    len++;
  }
  next1:
  for (uint8_t i = 1; i < num_of_notes - 1; i++) {
    switch (mode) {
      case ARP_UPDOWN:
        note = sort_up[num_of_notes - i - 1];
        break;
      case ARP_DOWNUP:
        note = sort_up[i];
        break;
      default:
        goto next2;
    }
    if (len >= ARP_MAX_NOTES) { break; }
    notes[len++] = note;
  }
  next2:
  switch (mode) {
  case ARP_PINKUP:
  case ARP_THUMBUP:
    if (num_of_notes == 1) {
      if (len >= ARP_MAX_NOTES) { break; }
      notes[len++] = sort_up[0];
    }
    for (uint8_t i = 0; i < num_of_notes - 1; i++) {
      note = sort_up[i];
      if (len >= ARP_MAX_NOTES) { break; }
      notes[len++] = note;
      note = top_note;
      if (len >= ARP_MAX_NOTES) { break; }
      notes[len++] = note;
    }

    break;

  case ARP_PINKDOWN:
  case ARP_THUMBDOWN:
    for (uint8_t i = 1; i < num_of_notes; i++) {
      note = sort_up[num_of_notes - i - 1];
      if (len >= ARP_MAX_NOTES) { break; }
      notes[len++] = note;
      note = mode == ARP_PINKDOWN ? sort_up[0] : top_note;
      if (len >= ARP_MAX_NOTES) { break; }
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
  mute_state = mute_state_old;
}

void ArpSeqTrack::on_cycle_midpoint(MidiUartClass *, MidiUartClass *) {}

void ArpSeqTrack::on_render_begin() {}

void MDArpSeqTrack::dispatch_note(uint8_t note, MidiUartClass *uart_,
                                  MidiUartClass *uart2_) {
#if defined(PLATFORM_TBD)
  if (md_arp_targets_tbd()) {
    if (track_number < mcl_seq.num_tbd_tracks) {
      auto &track = mcl_seq.tbd_tracks[track_number];
      track.note_on(note, 127, uart_);
      last_note_on = note;
      if (SeqPage::recording && MidiClock.state == 2) {
        reset_undo();
        track.record_track(127);
        track.record_track_pitch(note);
      }
    }
    return;
  }
#endif
  bool is_midi_model =
      ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
  if (is_midi_model) {
    SeqTrackUtil::with_md_track(track_number,
                                [&](auto &t) { t.send_notes(note, uart2_); });
    seq_ptc_page.record(note, track_number);
  } else {
    seq_ptc_page.trig_primary(note, track_number, CTRL_EVENT, fine_tune, uart_);
  }
}

void MDArpSeqTrack::on_cycle_midpoint(MidiUartClass *uart_,
                                      MidiUartClass *) {
#if defined(PLATFORM_TBD)
  if (md_arp_targets_tbd() && last_note_on != 255 &&
      step_count == length / 2 && track_number < mcl_seq.num_tbd_tracks) {
    mcl_seq.tbd_tracks[track_number].note_off(uart_);
    last_note_on = 255;
  }
#else
  (void)uart_;
#endif
}

void MDArpSeqTrack::on_render_begin() {
#if defined(PLATFORM_TBD)
  if (md_arp_targets_tbd() && track_number < mcl_seq.num_tbd_tracks) {
    mcl_seq.tbd_tracks[track_number].send_notes_off();
    last_note_on = 255;
  }
#endif
}

void ExtArpSeqTrack::dispatch_note(uint8_t note, MidiUartClass *,
                                   MidiUartClass *uart2_) {
  seq_ptc_page.note_on_ext(note, 127, track_number, uart2_);
  last_note_on = note;
}

void ExtArpSeqTrack::on_cycle_midpoint(MidiUartClass *, MidiUartClass *uart2_) {
  if (last_note_on != 255 && step_count == length / 2) {
    seq_ptc_page.note_off_ext(last_note_on, 0, track_number, uart2_);
    last_note_on = 255;
  }
}

void ExtArpSeqTrack::on_render_begin() {
  seq_ptc_page.buffer_notesoff_ext(track_number);
}
