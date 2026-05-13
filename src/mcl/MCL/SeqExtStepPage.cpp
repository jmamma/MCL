#include "SeqExtStepPage.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "SeqExtStepTrackRef.h"
#include "SeqPages.h"

namespace {

constexpr uint8_t kExtStepVisibleSteps = 16;
constexpr uint8_t kExtStepDefaultZoomSteps = 16;
constexpr uint8_t kExtStepLockParamFallback = 0;

SeqExtStepTrackApi active_ext_step_track() {
  return SeqExtStepTrackRef::active_track();
}

#ifdef PLATFORM_TBD
bool seq_ext_step_param_menu_label(uint8_t entry_index, uint8_t option_n,
                                   char *dst, uint8_t dst_len) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE ||
      entry_index != SEQ_MENU_PARAMSELECT) {
    return false;
  }
  return active_ext_step_track().locks().copy_lock_menu_value_label(option_n,
                                                                    dst,
                                                                    dst_len);
}
#endif

#ifdef PLATFORM_TBD
uint8_t seq_ext_step_pitch_from_midi_note(uint8_t note_num) {
  uint16_t pitch = seq_ptc_page.seq_ext_pitch(note_num);
  if (pitch == 255) return 255;
  pitch += ptc_param_oct.cur * 12;
  return pitch < 128 ? pitch : 255;
}
#endif

bool seq_ext_step_menu_entry_is(uint8_t entry_index) {
  if (!SeqPage::show_seq_menu) return false;
  return seq_menu_page.menu.get_item_index(seq_menu_page.encoders[1]->cur) ==
         entry_index;
}

seq_extstep_tick_t seq_ext_step_page_width(uint16_t ticks_per_step) {
  return (seq_extstep_tick_t)kExtStepVisibleSteps * ticks_per_step;
}

#ifdef PLATFORM_TBD
uint8_t seq_ext_step_at_key(seq_extstep_tick_t x, uint16_t ticks_per_step,
                            uint8_t key) {
  seq_extstep_tick_t page_width = seq_ext_step_page_width(ticks_per_step);
  return ((x / page_width) * kExtStepVisibleSteps) + key;
}

void seq_ext_step_update_lock_cursor(SeqExtStepLockApi &locks,
                                     uint8_t lock_idx, uint8_t ctrl_type,
                                     uint16_t ctrl, uint16_t value,
                                     bool update_param_select) {
  SeqExtStepLockParamInfo info;
  if (locks.selected_lock_param_info(lock_idx, info) && info.learn) {
    if (update_param_select && ctrl_type == SEQ_EXT_LOCK_CTRL_CC &&
        ctrl <= 127) {
      SeqPage::param_select = (uint8_t)ctrl;
    }
    locks.set_selected_lock_control(lock_idx, ctrl_type, ctrl, value);
  }
  if (locks.selected_lock_matches_control(lock_idx, ctrl_type, ctrl)) {
    seq_extstep_page.lock_cur_y =
        locks.lock_ui_value_from_control(lock_idx, ctrl_type, ctrl, value);
  }
}

bool seq_ext_step_lock_held_steps(SeqExtStepTrackApi &track,
                                  uint8_t ctrl_type, uint16_t ctrl,
                                  uint16_t value,
                                  bool update_param_select) {
  auto locks = track.locks();
  uint16_t timing_mid = track.ticks_per_step();
  uint8_t lock_idx = SeqPage::pianoroll_mode - 1;
  SeqExtStepLockParamInfo info;
  if (!locks.selected_lock_param_info(lock_idx, info)) {
    return false;
  }

  if (info.learn) {
    if (update_param_select && ctrl_type == SEQ_EXT_LOCK_CTRL_CC &&
        ctrl <= 127) {
      SeqPage::param_select = (uint8_t)ctrl;
    }
    locks.set_selected_lock_control(lock_idx, ctrl_type, ctrl, value);
    if (!locks.selected_lock_param_info(lock_idx, info)) {
      return false;
    }
  }
  if (!locks.selected_lock_matches_control(lock_idx, ctrl_type, ctrl)) {
    return false;
  }

  uint8_t lock_param =
      info.param_id <= 127 ? (uint8_t)info.param_id : kExtStepLockParamFallback;
  uint8_t lock_value =
      locks.lock_ui_value_from_control(lock_idx, ctrl_type, ctrl, value);

  for (uint8_t i = 0; i < kExtStepVisibleSteps; i++) {
    if (!note_interface.is_note_on(i)) continue;
    uint8_t step = seq_ext_step_at_key(seq_extstep_page.cur_x, timing_mid, i);
    if (step >= track.length()) continue;
    locks.add_lock(step, timing_mid, lock_param, lock_value, SeqPage::slide,
                   lock_idx);
  }
  return true;
}

bool seq_ext_step_handle_control(SeqExtStepTrackApi &track, uint8_t ctrl_type,
                                 uint16_t ctrl, uint16_t value,
                                 uint8_t track_index,
                                 bool update_param_select) {
  auto locks = track.locks();
  if (SeqPage::pianoroll_mode > 0) {
    uint8_t lock_idx = SeqPage::pianoroll_mode - 1;
    seq_ext_step_update_lock_cursor(locks, lock_idx, ctrl_type, ctrl, value,
                                    update_param_select);
  }

  if (SeqExtStepTrackRef::active_track_index() == track_index &&
      note_interface.notes_on && SeqPage::pianoroll_mode > 0) {
    auto page_track = active_ext_step_track();
    return seq_ext_step_lock_held_steps(page_track, ctrl_type, ctrl, value,
                                        update_param_select);
  }
  return true;
}
#endif

} // namespace

void SeqExtStepPage::setup() {
  SeqPage::setup();
  encoder_init = true;
}
void SeqExtStepPage::config() {
  // config info labels
  strcpy_P(info2, mclstr_ext);
  // config menu
  config_as_trackedit();

  // use continuous page index display
  display_page_index = false;
}
void SeqExtStepPage::config_encoders() {
  if (show_seq_menu) { return; }

  if (encoder_init) {
    encoder_init = false;
    auto active_track = active_ext_step_track();
    uint16_t timing_mid = active_track.ticks_per_step();
    seq_extparam4.cur = kExtStepDefaultZoomSteps;
    fov_offset = 0;
    cur_x = 0;
    fov_y = MIDI_NOTE_C3 - 1;
    cur_y = fov_y + 1;
    cur_w = timing_mid;
    uint8_t track_length = active_track.length();
    if (track_length == 0) {
      track_length = 1;
    }
    roll_length =
        (seq_extstep_tick_t)track_length * (seq_extstep_tick_t)timing_mid;
    seq_extstep_tick_t requested_fov =
        (seq_extstep_tick_t)seq_extparam4.cur * (seq_extstep_tick_t)timing_mid;
    fov_length = requested_fov < roll_length ? requested_fov : roll_length;
    if (fov_length <= 0) {
      fov_length = timing_mid ? timing_mid : 1;
    }
    fov_pixels_per_tick = ((seq_extstep_tick_t)fov_w << 8) / fov_length;

  }

  config();
  SeqPage::select_device_idx(DeviceIdx::Secondary);

}

void SeqExtStepPage::init() {
  page_count = 8;
  DEBUG_PRINTLN(F("seq extstep init"));

  select_device_idx(DeviceIdx::Secondary);

  SeqPage::init();
  SeqExtStepTrackRef::set_panel_rec_mode(3);
  param_select = PARAM_OFF;
  key_interface.on();
  key_interface.send_md_leds(TRIGLED_EXCLUSIVE);

  last_cur_x = -1;
  config_menu_entries();
  config_encoders();
#ifdef PLATFORM_TBD
  seq_menu_page.menu.option_name_override = seq_ext_step_param_menu_label;
#endif
#ifdef PLATFORM_TBD
  midi_events.setup_callbacks();
#endif

}

void SeqExtStepPage::cleanup() {
  SeqPage::cleanup();
  SeqExtStepTrackRef::set_panel_rec_mode(0);
#ifdef PLATFORM_TBD
  if (seq_menu_page.menu.option_name_override == seq_ext_step_param_menu_label) {
    seq_menu_page.menu.option_name_override = nullptr;
  }
#endif
#ifdef PLATFORM_TBD
  midi_events.remove_callbacks();
#endif
}

#define MAX_FOV_W 96

uint8_t SeqExtStepPage::fov_x_for_tick(seq_extstep_tick_t tick) const {
  return (uint8_t)(((int32_t)(tick - fov_offset) * fov_pixels_per_tick) >> 8);
}

uint8_t SeqExtStepPage::lock_y_for_value(uint8_t value) const {
  return fov_h - ((uint16_t)value * fov_h) / 128;
}

void SeqExtStepPage::draw_seq_pos() {
  auto active_track = active_ext_step_track();
  seq_extstep_tick_t cur_tick_x =
      (seq_extstep_tick_t)active_track.step_count() *
      active_track.ticks_per_step();


  // Draw sequencer position..
  if (is_within_fov(cur_tick_x)) {

    seq_extstep_tick_t cur_tick_fov_x =
        draw_x + fov_x_for_tick(cur_tick_x);
    cur_tick_fov_x = max((seq_extstep_tick_t)draw_x,
                         min((seq_extstep_tick_t)(draw_x + fov_w),
                             cur_tick_fov_x));
    oled_display.drawFastVLine((uint8_t)cur_tick_fov_x, 0, fov_h, WHITE);
  }
}

void SeqExtStepPage::draw_grid() {
  auto active_track = active_ext_step_track();
  uint8_t h = fov_h / fov_notes;

  uint8_t m = 4, n = kExtStepVisibleSteps;

  switch (active_track.speed()) {
     default:
     break;
     //case SEQ_SPEED_2X:
     //m = 2; n = 8;
     //break;
     case SEQ_SPEED_3_2X:
     case SEQ_SPEED_3_4X:
     m = 3; n = 12;
     break;
     /*
     case SEQ_SPEED_3_2X:
     m = 3; n = 6;
     break;
     case SEQ_SPEED_1_2X:
     m = 8; n = 32;
     break;
     case SEQ_SPEED_1_4X:
     m = 16; n = 64;
     break;
     case SEQ_SPEED_1_8X:
     m = 32; n = 128;
     break;
     */
  }

  for (uint8_t i = 0; i < active_track.length(); i++) {
    seq_extstep_tick_t grid_tick_x = active_track.step_tick(i);
    if (is_within_fov(grid_tick_x)) {
      seq_extstep_tick_t grid_fov_x =
          draw_x + fov_x_for_tick(grid_tick_x);

      if (grid_fov_x < draw_x) continue;

      for (uint8_t k = 0; k < fov_notes; k += 1) {
        // draw crisscross
        // if ((fov_y + k + i) % 2 == 0) { oled_display.drawPixel(
        // grid_fov_x, (k * (fov_h / fov_notes)), WHITE); }
        bool draw = false;
        uint8_t v = draw_y + (k * h);
        if ((pianoroll_mode > 0 && k == 3) ||
            i % kExtStepVisibleSteps == 0) {
          draw = true;
        }
        if (pianoroll_mode == 0) {
          draw = true;
        }
        if (i % n == 0) {
          //if ((fov_y + k + i) % 2 == 0) {
          if (k % 2 == 0) {
            oled_display.drawFastVLine((uint8_t)grid_fov_x, v, 4, WHITE);
          }
          continue;
        }
        if (draw) {
          oled_display.drawPixel((uint8_t)grid_fov_x, v, WHITE);
        }

        if (i % m == 0) {
          oled_display.drawPixel((uint8_t)grid_fov_x, v + (h / 2), WHITE);
        }
      }
    }
  }
}
void SeqExtStepPage::draw_thick_line(uint8_t x1, uint8_t y1, uint8_t x2,
                                     uint8_t y2) {
  oled_display.drawLine(x1, y1, x2, y2, WHITE);
  oled_display.drawLine(x1, y1 + 1, x2, y2 + 1, WHITE);
}

void SeqExtStepPage::draw_lockeditor() {
  auto active_track = active_ext_step_track();

  // Absolute piano roll dimensions

  uint16_t ev_idx = 0, ev_end = 0, ev_j_end;
  uint8_t j = 0;

  for (uint8_t i = 0; i < active_track.length(); i++) {
    // Update bucket index range
    /*    if (j > i) {
          i = j;
          ev_end = ev_j_end;
          // active_track.locate(i, ev_idx, ev_end);
        } else {
          ev_end += active_track.event_buckets.get(i);
        }*/

    ev_end += active_track.event_bucket_size(i);
    for (; ev_idx != ev_end; ++ev_idx) {
      auto ev = active_track.event(ev_idx);

      if (!ev.is_lock || (ev.lock_idx != pianoroll_mode - 1)) {
        continue;
      }

      uint16_t next_lock_ev = ev_idx;
      ev_j_end = ev_end;
      j = active_track.search_lock(pianoroll_mode - 1, i, next_lock_ev,
                                   ev_j_end);
      if (next_lock_ev == 0xFFFF) {
        next_lock_ev = ev_idx;
      }

      auto ev_j = active_track.event(next_lock_ev);

      seq_extstep_tick_t start_x = active_track.event_tick(i, ev);
      seq_extstep_tick_t end_x = active_track.event_tick(j, ev_j);
      if (start_x == end_x) {
        end_x = start_x - 1;
      }

      if (is_within_fov(start_x, end_x)) {
        uint8_t start_fov_x, end_fov_x;

        uint8_t start_y = ev.event_value;
        uint8_t end_y = ev_j.event_value;

        seq_extstep_tick_t start_x_tmp = start_x;
        seq_extstep_tick_t end_x_tmp = end_x;
        uint8_t start_y_tmp = start_y;
        uint8_t end_y_tmp = end_y;
        if (end_x < start_x) {
          end_x_tmp += roll_length;
        }

        // Fixed-point gradient (scaled by 256 for precision)
        seq_extstep_tick_t gradient_fixed = 0;
        if (start_x != end_x && ev.event_on) {
          gradient_fixed =
              ((seq_extstep_tick_t)(end_y - start_y) * 256) /
              (end_x_tmp - start_x_tmp);
        }

        // y = mx + y2 - mx2 = m( x - x1) + y1
        if (start_x < fov_offset) {
          start_fov_x = 0;
          // start_y_tmp = ((fov_offset - start_x) * gradient) + start_y
          seq_extstep_tick_t dx = fov_offset - start_x;
          start_y_tmp = (((int32_t)dx * gradient_fixed) / 256) + start_y;
        } else {
          // Convert fov_pixels_per_tick to fixed point once:
          // fov_pixels_per_tick_fixed = fov_pixels_per_tick * 256
          start_fov_x = fov_x_for_tick(start_x);
        }

        if (end_x >= fov_offset + fov_length) {
          end_fov_x = fov_w;
          seq_extstep_tick_t dx;
          if (start_x > end_x) {
            dx = fov_offset + fov_length + roll_length - start_x;
          } else {
            dx = fov_offset + fov_length - start_x;
          }
          end_y_tmp = (((int32_t)dx * gradient_fixed) / 256) + start_y;
        } else {
          end_fov_x = fov_x_for_tick(end_x);
        }

        uint8_t start_fov_y = lock_y_for_value(start_y_tmp);
        uint8_t end_fov_y = lock_y_for_value(end_y_tmp);

        // Draw logic
        if (end_x < start_x) {
          // Wrap around note
          if (start_x < fov_offset + fov_length) {
            seq_extstep_tick_t dx = fov_offset + fov_length - start_x;
            uint8_t calc_end_y_tmp =
                (((int32_t)dx * gradient_fixed) / 256) + start_y;
            uint8_t tmp_end_fov_y =
                lock_y_for_value(calc_end_y_tmp);

            draw_thick_line(start_fov_x + draw_x, start_fov_y, fov_w + draw_x,
                            tmp_end_fov_y);
          }

          if (end_x > fov_offset) {
            seq_extstep_tick_t dx = roll_length - start_x + fov_offset;
            uint8_t calc_end_y_tmp =
                (((int32_t)dx * gradient_fixed) >> 8) + start_y;
            uint8_t tmp_end_fov_y =
                lock_y_for_value(calc_end_y_tmp);

            draw_thick_line(draw_x, !ev.event_on ? start_fov_y : tmp_end_fov_y,
                            end_fov_x + draw_x,
                            !ev.event_on ? start_fov_y : end_fov_y);
          }
        } else {
          // Standard note
          draw_thick_line(start_fov_x + draw_x, start_fov_y, draw_x + end_fov_x,
                          !ev.event_on ? start_fov_y : end_fov_y);
        }
      }
      //  }
      /*if (j < i) {
        break;
      }*/
      }
    }
    // Draw interactive cursor
    seq_extstep_tick_t fov_cur_x = fov_x_for_tick(cur_x);
    uint8_t fov_cur_y = lock_y_for_value(lock_cur_y);
    if (fov_cur_x < 0) fov_cur_x = 0;
    if (fov_cur_x > fov_w) fov_cur_x = fov_w;
    mcl_gui.draw_cross(draw_x + (uint8_t)fov_cur_x, draw_y + fov_cur_y);
  }

void SeqExtStepPage::draw_note(uint8_t x, uint8_t y, uint8_t w, bool note_beyond_fov) {
  if (w == 0) {
    w = 1;
  }
  oled_display.drawRect(x, y, w, fov_h / fov_notes, WHITE);
  if (note_beyond_fov) { w += 1; }
  uint8_t h = fov_h / fov_notes;
  if (w > 2 && h > 2) {
    oled_display.fillRect(x + 1, y + 1, w - 2, h - 2, BLACK);
  }
}

void SeqExtStepPage::draw_pianoroll() {
  auto active_track = active_ext_step_track();
  uint16_t timing_mid = active_track.ticks_per_step();

  // Absolute piano roll dimensions

  uint8_t pattern_end_fov_x = fov_w;

  if (is_within_fov(roll_length)) {
    pattern_end_fov_x = min(fov_w, fov_x_for_tick(roll_length));
  }

  uint16_t ev_idx = 0, ev_end = 0;
  for (uint8_t i = 0; i < active_track.length(); i++) {
    // Update bucket index range
    ev_end += active_track.event_bucket_size(i);

    for (; ev_idx != ev_end; ++ev_idx) {
      auto ev = active_track.event(ev_idx);
      int note_val = ev.event_value;
      // Check if note is note_on and is visible within fov vertical
      // range.
      if (ev.is_lock || !ev.event_on) {
        continue;
      }
      uint16_t note_off_idx = ev_idx;
      uint8_t j =
          active_track.search_note_off(note_val, i, note_off_idx, ev_end);
      if (note_off_idx == 0xFFFF) {
        note_off_idx = ev_idx;
      }
      auto ev_j = active_track.event(note_off_idx);

      seq_extstep_tick_t note_start = active_track.event_tick(i, ev);
      seq_extstep_tick_t note_end = active_track.event_tick(j, ev_j);

      if (i > j && j == 0) { note_end += timing_mid * active_track.length(); }

      if (is_within_fov(note_start, note_end)) {
        uint8_t note_fov_start, note_fov_end;

        bool note_beyond_fov = false;

        if (note_start < fov_offset) {
          note_fov_start = 0;
        } else {
          note_fov_start = fov_x_for_tick(note_start);
        }
        if (note_end >= fov_offset + fov_length) {
          note_fov_end = fov_w;
          note_beyond_fov = true;
        } else {
          note_fov_end = fov_x_for_tick(note_end);
        }
        //On screen notes to be no less than 2 pixels, regardless of zoom
        if (i < j && note_fov_end - note_fov_start < 2) { note_fov_end = note_fov_start + 2; }
        //if (note_fov_end <= note_fov_start) { note_fov_end = note_fov_start + 1; }
        uint8_t note_fov_y = fov_h - ((note_val - fov_y) * (fov_h / fov_notes));
        // Draw vertical projection
        uint8_t proj_y = 255;
        if (note_val > fov_y + fov_notes) {
          //&&     (cur_y == fov_y + fov_notes - 1)) {
          proj_y = 0;
        } else if (note_val <= fov_y) {
          // && (cur_y == fov_y)) {
          proj_y = draw_y + fov_h + 1;
        }
        bool wrap = note_end < note_start;
        note_beyond_fov |= wrap;
        if (proj_y != 255) {
          if (wrap) {
            // Wrap around note

            if (note_start < fov_offset + fov_length) {
              oled_display.drawRect(note_fov_start + draw_x, proj_y,
                                     pattern_end_fov_x - note_fov_start, 1,
                                    WHITE);
            }

            if (note_end > fov_offset) {
             oled_display.drawRect(draw_x, proj_y, note_fov_end, 1, WHITE);
            }

          } else {
            // Standard note.
            oled_display.drawRect(note_fov_start + draw_x, proj_y,
                                  note_fov_end - note_fov_start, 1, WHITE);
          }
        }
        // Draw notes
        if ((note_val > fov_y) && (note_val <= fov_y + fov_notes)) {
          if (wrap) {
            // Wrap around note
            if (note_start < fov_offset + fov_length) {
              draw_note(note_fov_start + draw_x,
                                    draw_y + note_fov_y,
                                    pattern_end_fov_x - note_fov_start, note_beyond_fov);
            }

            if (note_end > fov_offset) {
              draw_note(draw_x, draw_y + note_fov_y, note_fov_end, false);
            }

          } else {
            // Standard note.
            draw_note(note_fov_start + draw_x, draw_y + note_fov_y,
                                  note_fov_end - note_fov_start, note_beyond_fov);
          }
        }
      }
    }
  }
  // Draw interactive cursor
  uint8_t fov_cur_y = fov_h - ((cur_y - fov_y) * ((fov_h) / fov_notes));
  seq_extstep_tick_t fov_cur_x =
      fov_x_for_tick(cur_x);
  uint16_t fov_cur_w =
      ((seq_extstep_tick_t)cur_w *
           (seq_extstep_tick_t)fov_pixels_per_tick +
       255) >>
      8;
  if (fov_cur_x < 0) {
    fov_cur_x = 0;
  }
  if (fov_cur_x < fov_w) {
    if (fov_cur_x + fov_cur_w > fov_w) {
      fov_cur_w = fov_w - fov_cur_x;
    }
    oled_display.fillRect(draw_x + (uint8_t)fov_cur_x, draw_y + fov_cur_y,
                          (uint8_t)fov_cur_w, (fov_h / fov_notes), WHITE);
  }
}

void SeqExtStepPage::draw_viewport_minimap() {
  auto active_track = active_ext_step_track();
  uint16_t timing_mid = active_track.ticks_per_step();

  constexpr uint16_t width = pidx_w * 4 + 3;

  seq_extstep_tick_t pattern_end =
      (seq_extstep_tick_t)max(kExtStepVisibleSteps, active_track.length()) *
      timing_mid;

  seq_extstep_tick_t cur_tick_x =
      (seq_extstep_tick_t)active_track.step_count() * timing_mid +
      active_track.mod_ticks();

  oled_display.drawRect(pidx_x0, pidx_y, width, pidx_h, WHITE);

  // viewport is [fov_offset, fov_offset+fov_length] out of [0,
  // pattern_end]

  uint16_t s = (seq_extstep_tick_t)fov_offset * (width - 1) / pattern_end;
  uint16_t w = fov_length * (width - 2) / pattern_end;
  uint16_t p = min((seq_extstep_tick_t)width,
                   (seq_extstep_tick_t)(cur_tick_x *
                                        (seq_extstep_tick_t)(width - 1) /
                                        pattern_end));

  oled_display.drawFastHLine(pidx_x0 + 1 + s, pidx_y + 1, w, WHITE);

  oled_display.drawPixel(pidx_x0 + 1 + p, pidx_y + 1, INVERT);
}

void SeqExtStepPage::pos_cur_x(seq_extstep_tick_t diff) {
  seq_extstep_tick_t w = cur_w;
  if (pianoroll_mode >= 1) {
    w = 3;
  }
  if (diff < 0) {
    if (cur_x <= fov_offset) {
      fov_offset += diff;
      // / fov_pixels_per_tick;
      if (fov_offset < 0) {
        fov_offset = 0;
      }
      cur_x = fov_offset;
    } else {
      cur_x += diff;
      if (cur_x < fov_offset) {
        cur_x = fov_offset;
      }
    }

  } else {
    if (cur_x + w >= fov_offset + fov_length) {
      if (fov_offset + fov_length < roll_length) {
        fov_offset += diff;
        cur_x = fov_offset + fov_length - w;
      }
      else {
        if (cur_x + diff < roll_length) {
        cur_x += diff;
       // cur_w = roll_length - cur_x;
        }
      }
   } else {
      cur_x += diff;
      if (cur_x > fov_offset + fov_length - w) {
        cur_x = fov_offset + fov_length - w;
      }
    }
  }
}

void SeqExtStepPage::set_cur_y(uint8_t cur_y_) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE || show_seq_menu) { return; }
  uint8_t fov_y_ = fov_y;
  if (fov_y >= cur_y_ && cur_y_ != 0) {
    fov_y_ = cur_y_ - 1;
  } else if (fov_y + fov_notes <= cur_y_) {
    fov_y_ = cur_y_ - fov_notes;
  }
  //  if (MidiClock.state != 2) {
  fov_y = fov_y_;
  cur_y = cur_y_;
  for (uint8_t n = 0; n < kExtStepVisibleSteps; n++) {
    if (note_interface.is_note_on(n)) {
      auto active_track = active_ext_step_track();
      uint16_t timing_mid = active_track.ticks_per_step();

      seq_extstep_tick_t pos =
          fov_offset + (seq_extstep_tick_t)timing_mid * n;
      seq_extstep_tick_t w = cur_w;
      if (pos < 0 || pos >= roll_length) continue;
      if (pos + w >= roll_length) { w = roll_length - pos - 1; }
      if (w <= 0) continue;

      active_track.delete_note(pos, w - 1, cur_y);
      active_track.add_note(pos, w, cur_y, velocity, cond);
    }
  }

  //  }
}

void SeqExtStepPage::pos_cur_y(int16_t diff) {
  if (pianoroll_mode >= 1) {
    lock_cur_y = limit_value(lock_cur_y, diff, 0, 127);
  }

  else {
    if (diff < 0) {
      scroll_dir = false;
      if (cur_y <= fov_y + 1) {
        fov_y += diff;
        if (fov_y < 1) {
          fov_y = 1;
        }
        cur_y = fov_y + 1;
      } else {
        cur_y += diff;
        if (cur_y < fov_y + 1) {
          cur_y = fov_y + 1;
        }
      }
    } else {
      scroll_dir = true;
      if (cur_y >= fov_y + fov_notes) {
        fov_y += diff;
        if (fov_y + fov_notes > 127) {
          fov_y = 127 - fov_notes;
        }
        cur_y = fov_y + fov_notes;
      } else {
        cur_y += diff;
        if (cur_y >= fov_y + fov_notes) {
          cur_y = fov_y + fov_notes;
        }
      }
    }
  }
}

void SeqExtStepPage::pos_cur_w(seq_extstep_tick_t diff) {
  if (diff < 0) {
    cur_w += diff;
    cur_w = max(cur_w, cur_w_min);
  } else {
    if (cur_x + cur_w >= fov_offset + fov_length) {
      if (fov_offset + fov_length + diff < roll_length) {
        cur_w += diff;
        fov_offset += diff;
      }
    } else {
      cur_w += diff;
    }
  }
}
void SeqExtStepPage::config_menu_entries() {
  seq_menu_page.menu.enable_entry(SEQ_MENU_PIANOROLL, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CHANNEL, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_EXT, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_AUTOMATION, true);

  if (pianoroll_mode == 0) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, false);
    encoders[1]->rot_res = ENCODER_RES_SEQ;
  } else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, true);
    encoders[1]->rot_res = 1;
  }

}

void SeqExtStepPage::loop() {
  config_menu_entries();
  auto active_track = active_ext_step_track();
  uint16_t timing_mid = active_track.ticks_per_step();
  auto active_locks = active_track.locks();
  SeqPage::loop();

  bool is_lockeditor = (pianoroll_mode > 0);
  // If pianoroll_edit mode changed.
  if (show_seq_menu) {
    display_mute_mask(SeqExtStepTrackRef::mute_mask_device(), 8);
    if (last_pianoroll_mode != pianoroll_mode) {

      if (is_lockeditor) {
        param_select =
            active_locks.selected_lock_menu_value(pianoroll_mode - 1);
      }
      last_pianoroll_mode = pianoroll_mode;
    }
    if (is_lockeditor &&
        seq_ext_step_menu_entry_is(SEQ_MENU_PARAMSELECT)) {
      auto *value_encoder = (MCLEncoder *)seq_menu_page.encoders[0];
      value_encoder->min = 0;
      value_encoder->max = active_locks.lock_param_menu_max();
      if (!active_locks.selected_lock_menu_editable(pianoroll_mode - 1)) {
        value_encoder->setValue(PARAM_OFF);
        param_select = PARAM_OFF;
        return;
      }
      bool menu_value_changed = value_encoder->cur != value_encoder->old;
      uint8_t normalized = active_locks.normalize_lock_menu_value(
          value_encoder->cur, value_encoder->old);
      if (normalized != value_encoder->cur) {
        value_encoder->setValue(normalized);
        menu_value_changed = true;
      }
      param_select = normalized;
      active_locks.set_selected_lock_menu_value(pianoroll_mode - 1,
                                                param_select);
      if (menu_value_changed) {
        lock_cur_y = active_locks.selected_lock_current_ui_value(
            pianoroll_mode - 1);
      }
    }
  }
  seq_extstep_tick_t diff = seq_extparam1.getValue();
  if (diff) {
    // Horizontal translation

    if (seq_extparam4.cur == zoom_max && BUTTON_DOWN(Buttons.ENCODER1)) {
      diff *= (timing_mid / 3);
    }
    else if (!BUTTON_DOWN(Buttons.ENCODER1)) { diff = diff * timing_mid; }
    pos_cur_x(diff);
  }
  diff = seq_extparam2.getValue();
  if (diff) {
        // Vertical translation
    bool button_pressed = BUTTON_DOWN(Buttons.ENCODER2);

    // Multiply by 4 when mode and button state match, divide by 4 otherwise
    if (is_lockeditor) { diff = button_pressed ? diff / 4 : diff * 4; }
    //else { diff = button_pressed ? diff * 4 : diff / 4; }

    pos_cur_y(-1 * diff);
  }

  diff = seq_extparam3.getValue();
  if (diff) {
    pos_cur_w(diff);
  }

  roll_length = active_track.length() * timing_mid; // in ticks

  if (seq_extparam4.cur > zoom_max) {
      seq_extparam4.cur = zoom_max;
  }

  if (cur_w > roll_length) { cur_w = roll_length / 2; }

  int32_t fov_length_new =
      ((int32_t)active_track.length() * timing_mid * fov_pixels_per_tick) >> 8;
  if (fov_length_new < fov_w) {
      seq_extparam4.cur = (uint16_t)active_track.length() * active_track.speed_multiplier_int() / 12;
      if (seq_extparam4.cur > zoom_max) {
        seq_extparam4.cur = zoom_max;
      }
      fov_length = fov_w;

      goto change;
//      fov_offset = 0;
//      cur_x = 0;
  }
  if (seq_extparam4.hasChanged()) {
    change:
    uint8_t fov_zoom = seq_extparam4.cur;

    fov_length = fov_zoom * timing_mid; // how many ticks to display on screen.
    if (fov_length > roll_length) { fov_length = roll_length; }

    seq_extstep_tick_t x = cur_x - fov_offset;

    int32_t fov_old_x = ((int32_t)x * fov_pixels_per_tick) >> 8;

    fov_pixels_per_tick = ((seq_extstep_tick_t)fov_w << 8) / fov_length;

    int32_t fov_cur_x = ((int32_t)x * fov_pixels_per_tick) >> 8;

    int32_t offset =
        ((int32_t)(fov_cur_x - fov_old_x) << 8) / fov_pixels_per_tick;

    fov_offset += offset;

    if (fov_length + fov_offset > roll_length) {
      fov_offset = roll_length - fov_length;
    }
    fov_offset = max(0,fov_offset);

  }

}

void SeqExtStepPage::display() {
  #ifdef EXT_TRACKS
  auto active_track = active_ext_step_track();
  auto active_locks = active_track.locks();
  uint16_t timing_mid = active_track.ticks_per_step();
  uint8_t epoch = 0;
  do {
  oled_display.clearDisplay();
  draw_viewport_minimap();
  draw_grid();
  mcl_gui.put_value_at(cur_x/timing_mid + 1,info1);
  epoch = active_track.change_counter();
  if (pianoroll_mode == 0) {
    uint8_t oct = cur_y / 12;
    uint8_t note = cur_y - 12 * (cur_y / 12);
    char str[5] = " ";
    const char *note_name = number_to_note.notes_upper[note];
    str[1] = note_name[0];
    str[2] = note_name[1];
    str[3] = oct + '0';
    str[4] = 0;
    strcat(info1, str);
    strcpy_P(info2, mclstr_note);
    draw_pianoroll();
  } else {
    if (!active_locks.copy_selected_lock_label(pianoroll_mode - 1, info2,
                                               sizeof(info2))) {
      strcpy_P(info2, mclstr_lock_space);
      mcl_gui.put_value_at(pianoroll_mode, info2 + 5);
    }
    if (!active_locks.copy_lock_value_label(pianoroll_mode - 1, lock_cur_y,
                                            info1, sizeof(info1))) {
      mcl_gui.put_value_at(lock_cur_y, info1);
    }
    draw_lockeditor();
  }
  } while (epoch != active_track.change_counter());
  draw_seq_pos();

  SeqPage::display();
  // Draw vertical keyboard
  oled_display.fillRect(draw_x - keyboard_w - 1, 0, keyboard_w + 1, fov_h,
                        BLACK);
  if (pianoroll_mode == 0) {
    const uint16_t chromatic = 0b0000010101001010;
    for (uint8_t k = 0; k < fov_notes; k++) {
      uint8_t scale_pos =
          (fov_y + fov_notes - k) - (((fov_y + fov_notes - k) / 12) * 12);
      if (!IS_BIT_SET16(chromatic, scale_pos)) {
        oled_display.fillRect(draw_x - keyboard_w,
                              draw_y + k * (fov_h / fov_notes) + 1,
                              keyboard_w + 1, (fov_h / fov_notes) - 1, WHITE);
      } else {
        //    oled_display.fillRect(draw_x - keyboard_w,
        //                            draw_y + k * (fov_h / fov_notes) + 1,
        //                            keyboard_w + 1, (fov_h / fov_notes) -
        //                            1, BLACK);
        oled_display.fillRect(draw_x, draw_y + k * (fov_h / fov_notes), 1,
                              (fov_h / fov_notes) + 1, WHITE);
      }
    }
  }
  // oled_display.fillRect(draw_x, 0, 1 , fov_h, WHITE);

#endif
}

void SeqExtStepPage::enter_notes() {
  auto active_track = active_ext_step_track();
  seq_extstep_tick_t w = cur_w;
  if (cur_x < 0 || cur_x >= roll_length) return;
  if (cur_x + w >= roll_length) { w = roll_length - cur_x - 1; }
  if (w <= 0) return;

  for (uint8_t n = 0; n < NUM_NOTES_ON; n++) {
    NoteVector note;
    if (!active_track.note_on_at(n, note))
      continue;
    active_track.delete_note(cur_x, w - 1, note.value);
    active_track.add_note(cur_x, w, note.value, velocity, cond);
  }
}

void SeqExtStepPage::param_select_update() {
  if (pianoroll_mode > 0) {
    auto active_track = active_ext_step_track();
    auto active_locks = active_track.locks();
    param_select = active_locks.selected_lock_menu_value(pianoroll_mode - 1);
    param_select = active_locks.normalize_lock_menu_value(param_select,
                                                          param_select);
  }
}

bool SeqExtStepPage::handleEvent(gui_event_t *event) {

  auto active_track = active_ext_step_track();
  uint16_t timing_mid = active_track.ticks_per_step();

  if (EVENT_NOTE(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t track = event->source;

    if (!SeqExtStepTrackRef::supports_trig_port(port)) {
      return true;
    }
    if (show_seq_menu) {
      if (mask == EVENT_BUTTON_PRESSED) {
        toggle_ext_mask(track);
        param_select_update();
      }
      return true;
    }

    key_interface.send_md_leds(TRIGLED_EXCLUSIVE);

    if (mask == EVENT_BUTTON_PRESSED) {
      seq_extstep_tick_t a = seq_ext_step_page_width(timing_mid);
      seq_extstep_tick_t new_x =
          ((cur_x / a) * a + timing_mid * track) - cur_x;
      pos_cur_x(new_x);
      new_x = ((cur_x / a) * a + timing_mid * track) - cur_x;
      pos_cur_x(new_x);
      note_interface.last_note = track;
    }
    if (mask == EVENT_BUTTON_RELEASED) {
      if (note_interface.notes_all_off()) {
        enter_notes();
      }
    }
    return true;
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
#ifdef PLATFORM_TBD
    if (show_seq_menu) {
      return seq_menu_page.handleEvent(event);
    }
#endif
    if (key_interface.is_key_down(MDX_KEY_PATSONG)) {
      return seq_menu_page.handleEvent(event);
    }
    uint8_t inc = 1;
    seq_extstep_tick_t w = timing_mid;
    if (key_interface.is_key_down(MDX_KEY_FUNC)) {
      inc = 4;
      w = w * 2;
    }

    if (pianoroll_mode > 0) {
      inc = 1;
      w = seq_extparam4.cur / 2;
      if (!key_interface.is_key_down(MDX_KEY_FUNC)) {
        w *= 2;
        inc = 12;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_SCALE: {
        seq_extstep_tick_t a = seq_ext_step_page_width(timing_mid);
        cur_x += a;
        if (cur_x > fov_offset + fov_length) {
          fov_offset += a;
        }
        if (cur_x >= roll_length) {
          cur_x = cur_x - (cur_x / a) * a;
          fov_offset = 0;
        }
        pos_cur_x(0);
        /*  if (fov_offset + fov_length > roll_length) {
            cur_x = cur_x - fov_offset;
            fov_offset = 0;
          } */
        return true;
      }
      }
    }
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (key_interface.is_key_down(MDX_KEY_YES)) {
        w = 1;
      }

      if (key_interface.is_key_down(MDX_KEY_NO) ||
          note_interface.notes_count_on()) {
        switch (key) {
        case MDX_KEY_UP: {
          seq_extparam4.cur -= 2;
          return true;
        }
        case MDX_KEY_DOWN: {
          seq_extparam4.cur += 2;
          return true;
        }
        case MDX_KEY_LEFT: {
          pos_cur_w(-1 * w);
          if (cur_w == cur_w_min && w > 1) {
            cur_w = (seq_extstep_tick_t)(timing_mid / 2);
          }
          return true;
        }
        case MDX_KEY_RIGHT: {
          if (cur_w == (seq_extstep_tick_t)(timing_mid / 2) && w > 1) {
            w = (seq_extstep_tick_t)(timing_mid / 2);
          }
          pos_cur_w(w);
          return true;
        }
        case MDX_KEY_CLEAR: {
          if (pianoroll_mode == 0) {
            for (uint8_t n = 0; n < 127; n++) {
              active_track.delete_note(cur_x, w - 1, n);
            }
          }
          return true;
        }
        }
      } else {
        switch (key) {

        case MDX_KEY_LEFT: {
          pos_cur_x(-1 * w);
          if (key_interface.is_key_down(MDX_KEY_YES)) {
            key_interface.ignoreNextEvent(MDX_KEY_YES);
          }
          return true;
        }
        case MDX_KEY_RIGHT: {
          pos_cur_x(w);
          if (key_interface.is_key_down(MDX_KEY_YES)) {
            key_interface.ignoreNextEvent(MDX_KEY_YES);
          }
          return true;
        }
        case MDX_KEY_UP: {
          pos_cur_y(inc);
          return true;
        }
        case MDX_KEY_DOWN: {
          pos_cur_y(-1 * inc);
          return true;
        }
          // case MDX_KEY_YES: {
          //   ignore_clear = true;
          //   goto YES;
          // }
        }
      }
    } else {
      switch (key) {
      case MDX_KEY_YES: {
        goto YES;
      }
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    YES:
      if (pianoroll_mode >= 1) {

        uint8_t step = active_track.step_from_tick(cur_x);
        uint16_t utiming = active_track.timing_from_tick(cur_x);
        /*
        if (BUTTON_DOWN(Buttons.BUTTON3) ||
        !active_track.clear_track_locks(step,
        active_track.locks_params[pianoroll_mode - 1] - 1, 255)) {
        active_track.set_track_locks(
            step, utiming, active_track.locks_params[pianoroll_mode - 1] - 1,
            lock_cur_y);
        }*/
        uint8_t lock_idx = pianoroll_mode - 1;
        auto locks = active_track.locks();

        bool clear = false;
        clear = locks.delete_lock(cur_x, lock_idx, lock_cur_y);
        uint8_t param_id = 0;
        if (!clear && locks.selected_lock_param_id(lock_idx, param_id)) {
          locks.add_lock(step, utiming, param_id, lock_cur_y, slide, lock_idx);
        }
        DEBUG_DUMP(locks.count_lock_event(step, lock_idx));
        return true;
      } else {
        DEBUG_PRINTLN("here");
        DEBUG_PRINTLN(active_track.notes_on_count());
        if (active_track.notes_on_count() > 0) {
          enter_notes();
        } else {
          seq_extstep_tick_t w = cur_w;
          if (cur_x < 0 || cur_x >= roll_length) return true;
          if (cur_x + w >= roll_length) {
            w = roll_length - cur_x - 1;
          }
          if (w <= 0) return true;

          if (!active_track.delete_note(cur_x, w - 1, cur_y)) {
            active_track.add_note(cur_x, w, cur_y, velocity, cond);
          }
        }
        return true;
      }
    }

#ifndef PLATFORM_TBD
    if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
      toggle_record();
      return true;
    }
#endif

    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      mute_mask = 128;
      param_select_update();
    }
  }
  if (SeqPage::handleEvent(event)) {
    return true;
  }
  return false;
}

#ifdef PLATFORM_TBD
void SeqExtStepMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) return;

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track = 0;
  if (!SeqExtStepTrackRef::track_for_channel(channel, &track)) return;

  auto active_track = SeqExtStepTrackRef::track(track);
  auto active_locks = active_track.locks();
  if (mcl_cfg.uart_cc_fwd) {
    active_track.send_cc(param, value);
  }

  if (SeqExtStepTrackRef::active_track_index() != track) {
    SeqExtStepTrackRef::select_track(track);
    seq_extstep_page.config_encoders();
  }

  SeqExtParsedControl parsed_control;
  if (active_track.parse_control_change(control_state, channel, param, value,
                                        parsed_control)) {
    if (!parsed_control.has_value) return;
    if (!seq_ext_step_handle_control(active_track, parsed_control.ctrl_type,
                                     parsed_control.parameter,
                                     parsed_control.value, track, false)) {
      return;
    }
    if (SeqPage::recording && MidiClock.state == 2 &&
        !note_interface.notes_on) {
      active_locks.record_control_lock(
          parsed_control.ctrl_type, parsed_control.parameter,
          parsed_control.value, SeqPage::slide);
    }
    return;
  }

  if (!seq_ext_step_handle_control(active_track, SEQ_EXT_LOCK_CTRL_CC, param,
                                   value, track, true)) {
    return;
  }

  if (SeqPage::recording && MidiClock.state == 2 &&
      !note_interface.notes_on &&
      !SeqExtStepTrackRef::is_mute_cc(param)) {
    active_locks.record_control_lock(SEQ_EXT_LOCK_CTRL_CC, param, value,
                                     SeqPage::slide);
  }
  active_track.update_param(param, value);
}

void SeqExtStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) return;

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t note_num = msg[1];
  uint8_t track = 0;
  if (!SeqExtStepTrackRef::track_for_channel(channel, &track)) return;

  uint8_t pitch = seq_ext_step_pitch_from_midi_note(note_num);
  if (pitch == 255) return;

  if (SeqExtStepTrackRef::active_track_index() != track) {
    SeqExtStepTrackRef::select_track(track);
    seq_extstep_page.config_encoders();
  }

  auto active_track = SeqExtStepTrackRef::track(track);
  seq_extstep_page.set_cur_y(pitch);
  active_track.record_note_on(pitch, msg[2]);
  seq_extstep_page.last_rec_event = REC_EVENT_TRIG;

  uint8_t velocity = SeqPage::recording ? msg[2] : SeqPage::velocity;
  if (mcl_cfg.uart_note_fwd) {
    active_track.note_on(pitch, velocity);
  }
}

void SeqExtStepMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) return;

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t note_num = msg[1];
  uint8_t track = 0;
  if (!SeqExtStepTrackRef::track_for_channel(channel, &track)) return;

  uint8_t pitch = seq_ext_step_pitch_from_midi_note(note_num);
  if (pitch == 255) return;

  auto active_track = SeqExtStepTrackRef::track(track);
  if (mcl_cfg.uart_note_fwd) {
    active_track.note_off(pitch, msg[2]);
  }
  active_track.record_note_off(pitch);
}

void SeqExtStepMidiEvents::onPitchWheelCallback_Midi2(uint8_t *msg) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) return;

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t track = 0;
  if (!SeqExtStepTrackRef::track_for_channel(channel, &track)) return;

  uint16_t value = msg[1] | (msg[2] << 7);
  auto active_track = SeqExtStepTrackRef::track(track);
  active_track.pitch_bend(value);
  if (SeqPage::recording && MidiClock.state == 2) {
    active_track.locks().record_control_lock(SEQ_EXT_LOCK_CTRL_PITCH_BEND, 0,
                                             value, SeqPage::slide);
  }
}

void SeqExtStepMidiEvents::onChannelPressureCallback_Midi2(uint8_t *msg) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) return;

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t track = 0;
  if (!SeqExtStepTrackRef::track_for_channel(channel, &track)) return;

  auto active_track = SeqExtStepTrackRef::track(track);
  active_track.channel_pressure(msg[1]);
  if (SeqPage::recording && MidiClock.state == 2) {
    active_track.locks().record_control_lock(
        SEQ_EXT_LOCK_CTRL_CHANNEL_PRESSURE, 0, msg[1], false);
  }
}

void SeqExtStepMidiEvents::onAfterTouchCallback_Midi2(uint8_t *msg) {
  if (mcl.currentPage() != SEQ_EXTSTEP_PAGE) return;

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t track = 0;
  if (!SeqExtStepTrackRef::track_for_channel(channel, &track)) return;

  auto active_track = SeqExtStepTrackRef::track(track);
  active_track.after_touch(msg[1], msg[2]);
  if (SeqPage::recording && MidiClock.state == 2) {
    active_track.locks().record_control_lock(SEQ_EXT_LOCK_CTRL_POLY_PRESSURE,
                                             msg[1], msg[2], false);
  }
}

void SeqExtStepMidiEvents::setup_callbacks() {
#ifndef PLATFORM_TBD
  state = true;
  return;
#else
  if (state) {
    return;
  }
  bound_midi = SeqExtStepTrackRef::input_midi();
  if (bound_midi == nullptr) {
    return;
  }
  bound_midi->addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  bound_midi->addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  bound_midi->addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onControlChangeCallback_Midi2);
  bound_midi->addOnPitchWheelCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onPitchWheelCallback_Midi2);
  bound_midi->addOnAfterTouchCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onAfterTouchCallback_Midi2);
  bound_midi->addOnChannelPressureCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::
          onChannelPressureCallback_Midi2);
  state = true;
#endif
}

void SeqExtStepMidiEvents::remove_callbacks() {

#ifndef PLATFORM_TBD
  state = false;
  bound_midi = nullptr;
  return;
#else
  if (!state) {
    return;
  }
  if (bound_midi == nullptr) {
    state = false;
    bound_midi = nullptr;
    return;
  }
  bound_midi->removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  bound_midi->removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  bound_midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onControlChangeCallback_Midi2);
  bound_midi->removeOnPitchWheelCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onPitchWheelCallback_Midi2);
  bound_midi->removeOnAfterTouchCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onAfterTouchCallback_Midi2);
  bound_midi->removeOnChannelPressureCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::
          onChannelPressureCallback_Midi2);
  state = false;
  bound_midi = nullptr;
#endif
}
#endif
