#include "MCL_impl.h"

void SeqExtStepPage::setup() { SeqPage::setup(); }
void SeqExtStepPage::config() {
#ifdef EXT_TRACKS
//  seq_param3.cur = mcl_seq.ext_tracks[last_ext_track].length;
#endif
  // config info labels
  constexpr uint8_t len1 = sizeof(info1);

#ifdef EXT_TRACKS
/*
  if (mcl_seq.ext_tracks[last_ext_track].speed == EXT_SPEED_2X) {
    strcpy(info1, "HI-RES");
  } else {
    strcpy(info1, "LOW-RES");
  }
*/
#endif

  strcpy(info2, "EXT");
  // config menu
  config_as_trackedit();

  // use continuous page index display
  display_page_index = false;
}

void SeqExtStepPage::config_encoders() {
#ifdef EXT_TRACKS
  uint8_t timing_mid = mcl_seq.ext_tracks[last_ext_track].get_timing_mid();
  seq_param1.max = 127;
  seq_param1.cur = 64;
  seq_param1.old = 64;

  seq_param2.cur = 64;
  seq_param2.old = 64;

  seq_param3.handler = NULL;
  seq_param3.cur = 64;
  seq_param3.old = 64;
  seq_param3.max = 128;
  seq_param3.min = 1;

  fov_offset = 0;
  cur_x = 0;
  fov_y = 64;
  cur_y = fov_y + 1;
  cur_w = timing_mid;

  seq_param2.max = 127;
  seq_param4.max = 128;
  seq_param4.cur = 16;
  seq_param4.min = 4;
  config();
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
#endif
}

void SeqExtStepPage::init() {
  page_count = 8;
  DEBUG_PRINTLN(F("seq extstep init"));
  SeqPage::init();
  param_select = 129;
  trig_interface.on();
  note_interface.state = true;
  x_notes_down = 0;
  last_cur_x = -1;
  config_encoders();
  midi_events.setup_callbacks();
  seq_menu_page.menu.enable_entry(SEQ_MENU_TRACK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_CHANNEL, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_PIANOROLL, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, true);
}

void SeqExtStepPage::cleanup() {
  SeqPage::cleanup();
  midi_events.remove_callbacks();
}

#define MAX_FOV_W 96

void SeqExtStepPage::draw_seq_pos() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint16_t cur_tick_x = active_track.step_count * active_track.get_timing_mid();
  +active_track.mod12_counter;

  // Draw sequencer position..
  if (is_within_fov(cur_tick_x)) {

    uint8_t cur_tick_fov_x =
        min(127, draw_x + fov_pixels_per_tick * (cur_tick_x - fov_offset));
    oled_display.drawLine(cur_tick_fov_x, 0, cur_tick_fov_x, fov_h - 1, WHITE);
  }
}

void SeqExtStepPage::draw_grid() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t h = fov_h / fov_notes;
  for (uint8_t i = 0; i < active_track.length; i++) {
    uint16_t grid_tick_x = i * active_track.get_timing_mid();
    if (is_within_fov(grid_tick_x)) {
      uint8_t grid_fov_x =
          draw_x + fov_pixels_per_tick * (grid_tick_x - fov_offset);

      for (uint8_t k = 0; k < fov_notes; k += 1) {
        // draw crisscross
        // if ((fov_y + k + i) % 2 == 0) { oled_display.drawPixel(
        // grid_fov_x, (k * (fov_h / fov_notes)), WHITE); }
        bool draw = false;
        if ((pianoroll_mode > 0 && k == 3) || i % 16 == 0) {
          draw = true;
        }
        if (pianoroll_mode == 0) {
          draw = true;
        }
        if (draw) {
          oled_display.drawPixel(grid_fov_x, draw_y + (k * (h)), WHITE);
        }
        if (i % 16 == 0) {
          oled_display.drawPixel(grid_fov_x, draw_y + (k * h) + (h / 2), WHITE);
        }
      }
    }
  }
}
void SeqExtStepPage::draw_thick_line(uint8_t x1, uint8_t y1, uint8_t x2,
                                     uint8_t y2, uint8_t color) {
  oled_display.drawLine(x1, y1, x2, y2, color);
  oled_display.drawLine(x1, y1 + 1, x2, y2 + 1, color);
}

void SeqExtStepPage::draw_lockeditor() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();

  // Absolute piano roll dimensions
  uint8_t pattern_end_fov_x = fov_w;

  if (is_within_fov(roll_length)) {
    pattern_end_fov_x =
        min(fov_w, fov_pixels_per_tick * (roll_length - fov_offset));
  }

  uint16_t ev_idx = 0, ev_end = 0, ev_j_end;
  uint8_t j = 0;

  for (uint8_t i = 0; i < active_track.length; i++) {
    // Update bucket index range
    /*    if (j > i) {
          i = j;
          ev_end = ev_j_end;
          // active_track.locate(i, ev_idx, ev_end);
        } else {
          ev_end += active_track.timing_buckets.get(i);
        }*/
    ev_end += active_track.timing_buckets.get(i);
    for (; ev_idx != ev_end; ++ev_idx) {
      auto &ev = active_track.events[ev_idx];

      if (!ev.is_lock || (ev.lock_idx != pianoroll_mode - 1)) {
        continue;
      }

      uint16_t next_lock_ev = ev_idx;
      ev_j_end = ev_end;
      if (ev.event_on) {
        j = active_track.search_lock_idx(pianoroll_mode - 1, i, next_lock_ev,
                                         ev_j_end);
        if (next_lock_ev == 0xFFFF) {
          next_lock_ev = ev_idx;
        }

      } else {
        j = i;
      }
      auto &ev_j = active_track.events[next_lock_ev];

      uint16_t start_x = i * timing_mid + ev.micro_timing - timing_mid;
      uint16_t end_x = j * timing_mid + ev_j.micro_timing - timing_mid;

      if (is_within_fov(start_x, end_x)) {
        uint8_t start_fov_x, end_fov_x;

        uint8_t start_y = ev.event_value;
        uint8_t end_y = ev_j.event_value;

        uint16_t start_x_tmp = start_x;
        uint16_t end_x_tmp = end_x;
        uint8_t start_y_tmp = start_y;
        uint8_t end_y_tmp = end_y;

        if (end_x < start_x) {
          end_x_tmp += roll_length;
        }
        float gradient;
        if (start_x == end_x) {
          gradient = 0;
        } else {
          gradient =
              (float)(end_y - start_y) / (float)(end_x_tmp - start_x_tmp);
        }
        // y = mx + y2 - mx2 = m( x - x1) + y1

        if (start_x < fov_offset) {
          start_fov_x = 0;
          start_y_tmp = ((float)(fov_offset - start_x) * gradient) + start_y;
        } else {
          start_fov_x = (float)(start_x - fov_offset) * fov_pixels_per_tick;
        }

        if (end_x >= fov_offset + fov_length) {
          end_fov_x = fov_w;
          if (start_x > end_x) {
            end_y_tmp =
                ((float)(fov_offset + fov_length + roll_length - start_x)) *
                    gradient +
                start_y;
          } else {
            end_y_tmp =
                ((float)(fov_offset + fov_length - start_x)) * gradient +
                start_y;
          }
        } else {
          end_fov_x = (float)(end_x - fov_offset) * fov_pixels_per_tick;
        }
        uint8_t start_fov_y =
            fov_h - (((float)start_y_tmp / 128.0) * (float)fov_h);
        uint8_t end_fov_y = fov_h - (((float)end_y_tmp / 128.0) * (float)fov_h);

        if (!ev.event_on) {
          // Draw single lock.
          oled_display.fillRect(start_fov_x + draw_x, start_fov_y, 2, 2, WHITE);
        } else {
          // Draw Slide
          if (end_x < start_x) {
            // Wrap around note
            if (start_x < fov_offset + fov_length) {
              //     uint8_t d = pattern_end_fov_x - start_fov_x
              //
              //     uint8_t end_fov_x_y = d * gradient;
              float end_y_tmp =
                  ((float)(fov_offset + fov_length - start_x)) * gradient +
                  start_y;
              uint8_t tmp_end_fov_y =
                  fov_h - (((float)end_y_tmp / 128.0) * (float)fov_h);
              draw_thick_line(start_fov_x + draw_x, start_fov_y, fov_w + draw_x,
                              tmp_end_fov_y);
            }

            if (end_x > fov_offset) {

              uint8_t end_y_tmp =
                  ((float)(roll_length - start_x + fov_offset)) * gradient +
                  start_y;
              uint8_t tmp_end_fov_y =
                  fov_h - (((float)end_y_tmp / 128.0) * (float)fov_h);

              draw_thick_line(draw_x, tmp_end_fov_y, end_fov_x + draw_x,
                              end_fov_y);
            }

          } else {
            // Standard note.
            draw_thick_line(start_fov_x + draw_x, start_fov_y,
                            draw_x + end_fov_x, end_fov_y);
          }
        }
      }
    }
    /*if (j < i) {
      break;
    }*/
  }
  // Draw interactive cursor
  int16_t fov_cur_x = (float)(cur_x - fov_offset) * fov_pixels_per_tick;
  uint8_t fov_cur_w = (float)(cur_w)*fov_pixels_per_tick;
  uint8_t fov_cur_y = fov_h - ((float)lock_cur_y / 128.0 * (float)fov_h);

  oled_display.drawPixel(draw_x + fov_cur_x - 1, draw_y + fov_cur_y - 2, WHITE);
  oled_display.drawPixel(draw_x + fov_cur_x + 1, draw_y + fov_cur_y - 2, WHITE);
  oled_display.drawPixel(draw_x + fov_cur_x, draw_y + fov_cur_y - 1 - 2, WHITE);
  oled_display.drawPixel(draw_x + fov_cur_x, draw_y + fov_cur_y + 1 - 2, WHITE);
}

void SeqExtStepPage::draw_pianoroll() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();

  uint8_t roll_height = 127; // 127, Notes.
  // Absolute piano roll dimensions

  uint8_t pattern_end_fov_x = fov_w;

  if (is_within_fov(roll_length)) {
    pattern_end_fov_x =
        min(fov_w, fov_pixels_per_tick * (roll_length - fov_offset));
  }

  uint16_t ev_idx = 0, ev_end = 0;
  uint8_t h = fov_h / fov_notes;
  for (int i = 0; i < active_track.length; i++) {
    // Update bucket index range
    ev_end += active_track.timing_buckets.get(i);

    for (; ev_idx != ev_end; ++ev_idx) {
      auto &ev = active_track.events[ev_idx];
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
      auto &ev_j = active_track.events[note_off_idx];

      uint16_t note_start = i * timing_mid + ev.micro_timing - timing_mid;
      uint16_t note_end = j * timing_mid + ev_j.micro_timing - timing_mid;

      if (is_within_fov(note_start, note_end)) {
        uint8_t note_fov_start, note_fov_end;

        if (note_start < fov_offset) {
          note_fov_start = 0;
        } else {
          note_fov_start =
              (float)(note_start - fov_offset) * fov_pixels_per_tick;
        }

        if (note_end >= fov_offset + fov_length) {
          note_fov_end = fov_w;

        } else {
          note_fov_end = (float)(note_end - fov_offset) * fov_pixels_per_tick;
        }

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

        if (proj_y != 255) {
          if (note_end < note_start) {
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
          if (note_end < note_start) {
            // Wrap around note
            if (note_start < fov_offset + fov_length) {
              oled_display.drawRect(note_fov_start + draw_x,
                                    draw_y + note_fov_y,
                                    pattern_end_fov_x - note_fov_start,
                                    (fov_h / fov_notes), WHITE);
            }

            if (note_end > fov_offset) {
              oled_display.drawRect(draw_x, draw_y + note_fov_y, note_fov_end,
                                    (fov_h / fov_notes), WHITE);
            }

          } else {
            // Standard note.
            oled_display.drawRect(note_fov_start + draw_x, draw_y + note_fov_y,
                                  note_fov_end - note_fov_start,
                                  (fov_h / fov_notes), WHITE);
          }
        }
      }
    }
  }
  // Draw interactive cursor
  uint8_t fov_cur_y = fov_h - ((cur_y - fov_y) * ((fov_h) / fov_notes));
  int16_t fov_cur_x = (float)(cur_x - fov_offset) * fov_pixels_per_tick;
  uint8_t fov_cur_w = (float)(cur_w)*fov_pixels_per_tick;
  if (fov_cur_x < 0) {
    fov_cur_x = 0;
  }
  if (fov_cur_x + fov_cur_w > fov_w) {
    fov_cur_w = fov_w - fov_cur_x;
  }
  oled_display.fillRect(draw_x + fov_cur_x, draw_y + fov_cur_y, fov_cur_w,
                        (fov_h / fov_notes), WHITE);
}

void SeqExtStepPage::draw_viewport_minimap() {
#ifdef OLED_DISPLAY
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();
  constexpr uint16_t width = pidx_w * 4 + 3;
  uint16_t pattern_end = active_track.length * timing_mid;
  uint16_t cur_tick_x =
      active_track.step_count * timing_mid + active_track.mod12_counter;

  oled_display.drawRect(pidx_x0, pidx_y, width, pidx_h, WHITE);

  // viewport is [fov_offset, fov_offset+fov_length] out of [0,
  // pattern_end]

  uint16_t s = fov_offset * (width - 1) / pattern_end;
  uint16_t w = fov_length * (width - 2) / pattern_end;
  uint16_t p = cur_tick_x * (width - 1) / pattern_end;
  oled_display.drawFastHLine(pidx_x0 + 1 + s, pidx_y + 1, w, WHITE);
  oled_display.drawPixel(pidx_x0 + 1 + p, pidx_y + 1, INVERT);
#endif
}

void SeqExtStepPage::draw_note(uint8_t note_val, uint16_t note_start,
                               uint16_t note_end) {}

#ifndef OLED_DISPLAY
void SeqExtStepPage::display() { SeqPage::display(); }
#else

void SeqExtStepPage::loop() {

  if (pianoroll_mode == 0) {
    seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, false);

  } else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, true);
  }
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();
  SeqPage::loop();

  // If pianoroll_edit mode changed.
  if (show_seq_menu) {
    if (last_pianoroll_mode != pianoroll_mode) {

      if (pianoroll_mode > 0) {
        if (mcl_seq.ext_tracks[last_ext_track]
                .locks_params[pianoroll_mode - 1] == 0) {
          param_select = 129;
        } else {
          param_select = mcl_seq.ext_tracks[last_ext_track]
                             .locks_params[pianoroll_mode - 1] -
                         1;
        }
      }
      last_pianoroll_mode = pianoroll_mode;
    }
    if ((pianoroll_mode > 0)) {
      if (param_select == 129) {
        mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] = 0;
      } else {
        mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] =
            param_select + 1;
      }
    }
  }
  if (seq_param1.hasChanged()) {
    // Horizontal translation

    int16_t diff = seq_param1.cur - seq_param1.old;
    if (seq_param4.cur == zoom_max && BUTTON_DOWN(Buttons.ENCODER1)) {
      diff *= (timing_mid / 3);
    }
    uint8_t w = cur_w;
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
      if (cur_x >= fov_offset + fov_length - w) {
        if (fov_offset + fov_length + diff < roll_length) {
          fov_offset += diff;
          cur_x = fov_offset + fov_length - w;
        }
      } else {
        cur_x += diff;
        if (cur_x > fov_offset + fov_length - w) {
          cur_x = fov_offset + fov_length - w;
        }
      }
    }

    seq_param1.cur = 64;
    seq_param1.old = 64;
  }

  if (seq_param2.hasChanged()) {
    // Vertical translation
    int16_t diff = seq_param2.old - seq_param2.cur; // reverse dir for sanity.
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
    seq_param2.cur = 64;
    seq_param2.old = 64;
  }

  if (seq_param3.hasChanged()) {

    int16_t diff = seq_param3.cur - seq_param3.old;

    if (diff < 0) {
      cur_w += diff;
      if (cur_w < cur_w_min) {
        cur_w = cur_w_min;
      }
    } else {
      if (cur_x >= fov_offset + fov_length - cur_w - diff) {
        if (fov_offset + fov_length + diff < roll_length) {
          cur_w += diff;
          fov_offset += diff;
        }
      } else {
        cur_w += diff;
      }
    }
    seq_param3.cur = 64;
    seq_param3.old = 64;
  }
}

void SeqExtStepPage::display() {
#ifdef EXT_TRACKS
  oled_display.clearDisplay();

  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();
  draw_viewport_minimap();

  roll_length = active_track.length * timing_mid; // in ticks

  // FOV offsets

  if (seq_param4.cur > zoom_max) {
    seq_param4.cur = zoom_max;
  }
  if (seq_param4.cur > active_track.length) {
    seq_param4.cur = active_track.length;
  }

  uint8_t fov_zoom = seq_param4.cur;

  fov_length = fov_zoom * timing_mid; // how many ticks to display on screen.
  if (fov_length + fov_offset > roll_length) {
    fov_offset = roll_length - fov_length;
  }

  fov_pixels_per_tick = (float)fov_w / (float)fov_length;

  draw_grid();
  draw_seq_pos();

  if (pianoroll_mode == 0) {
    MusicalNotes number_to_note;
    uint8_t oct = cur_y / 12;
    uint8_t note = cur_y - 12 * (cur_y / 12);
    strcpy(info1, number_to_note.notes_upper[note]);
    info1[2] = oct + '0';
    info1[3] = 0;
    strcpy(info2, "NOTE");
    draw_pianoroll();
  } else {
    strcpy(info2, "LOCK  ");
    mcl_gui.put_value_at(pianoroll_mode, info2 + 5);
    mcl_gui.put_value_at(lock_cur_y, info1);
    draw_lockeditor();
  }

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

  oled_display.display();
#endif
}
#endif

void SeqExtStepPage::enter_notes() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  for (uint8_t n = 0; n < NUM_NOTES_ON; n++) {
    if (active_track.notes_on[n].value == 255)
      continue;
    active_track.del_note(cur_x, cur_w, active_track.notes_on[n].value);
    active_track.add_note(cur_x, cur_w, active_track.notes_on[n].value,
                          velocity, cond);
  }
}

bool SeqExtStepPage::handleEvent(gui_event_t *event) {

  auto &active_track = mcl_seq.ext_tracks[last_ext_track];

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;
    uint8_t track = event->source - 128;

    if (device != DEVICE_MD) {
      return true;
    }

    trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);

    if (mask == EVENT_BUTTON_PRESSED) {
      ++x_notes_down;
      if (x_notes_down == 1) {
        cur_x = fov_offset + (float)(fov_length / 16) * (float)track;
        note_interface.last_note = track;
      } else {
        cur_w =
            fov_offset + (float)(fov_length / 16) * (float)track - cur_x - 1;
        if (cur_w < 0) {
          cur_x += cur_w;
          cur_w = -cur_w;
        }
      }
      cur_x = max(0, cur_x);
      cur_w = max(cur_w_min, cur_w);
    } else if (mask == EVENT_BUTTON_RELEASED) {

      --x_notes_down;
      if (x_notes_down == 0) {
        enter_notes();
      }
      return true;
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    if (pianoroll_mode >= 1) {
      uint8_t timing_mid = active_track.get_timing_mid();

      uint8_t step = (cur_x / timing_mid);
      uint8_t utiming = timing_mid + cur_x - (step * timing_mid);
      /*
      if (BUTTON_DOWN(Buttons.BUTTON3) || !active_track.clear_track_locks(step,
      active_track.locks_params[pianoroll_mode - 1] - 1, 255)) {
      active_track.set_track_locks(
          step, utiming, active_track.locks_params[pianoroll_mode - 1] - 1,
          lock_cur_y);
      }*/
      uint8_t lock_idx = pianoroll_mode - 1;

      bool clear = false;
      clear = active_track.del_track_locks(cur_x, lock_idx, lock_cur_y);
      if (!clear) {
        active_track.set_track_locks(step, utiming,
                                     active_track.locks_params[lock_idx] - 1,
                                     lock_cur_y, slide, lock_idx);
      }
      DEBUG_DUMP(active_track.count_lock_event(step, lock_idx));
      return true;
    }
    if (!recording) {
      if (active_track.notes_on_count > 0) {
        enter_notes();
      } else {

        if (!active_track.del_note(cur_x, cur_w, cur_y)) {
          active_track.add_note(cur_x, cur_w, cur_y, velocity, cond);
        }
      }
      return true;
    } else {
      switch (last_rec_event) {
      case REC_EVENT_TRIG:
        if (BUTTON_DOWN(Buttons.BUTTON3)) {
          oled_display.textbox("CLEAR ", "TRACKS");
          for (uint8_t n = 0; n < NUM_EXT_TRACKS; ++n) {
            mcl_seq.ext_tracks[n].clear_track();
          }
        } else {
          oled_display.textbox("CLEAR ", "TRACK");
          active_track.clear_track();
        }
        break;
      case REC_EVENT_CC:
        // TODO
        // oled_display.textbox("CLEAR ", "LOCK");
        break;
      }
      return true;
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    recording = !recording;
    if (recording) {
      oled_display.textbox("REC", "");
      setLed2();
    } else {
      clearLed2();
    }
    return true;
  }

  if (!recording || MidiClock.state != 2 ||
      EVENT_PRESSED(event, Buttons.BUTTON2)) {
    if (pianoroll_mode > 0) {
      if (mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] ==
          0) {
        param_select = 129;
      } else {
        param_select = mcl_seq.ext_tracks[last_ext_track]
                           .locks_params[pianoroll_mode - 1] -
                       1;
      }
    }
    if (SeqPage::handleEvent(event)) {
    }
    return true;
  }
}

void SeqExtStepMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {
      if (SeqPage::pianoroll_mode > 0) {
        if (mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] -
                1 ==
            130) {
          mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] =
              param + 1;
          SeqPage::param_select = param;
        }
      }
      if (SeqPage::recording) {
        DEBUG_DUMP("Record... cc");
        mcl_seq.ext_tracks[n].record_track_locks(param, value, SeqPage::slide);
        mcl_seq.ext_tracks[n].update_param(param, value);
      }
      return;
    }
  }
}

void SeqExtStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  // Step edit for ExtSeq
  // For each incoming note, check to see if note interface has any steps
  // selected For selected steps record notes.
#ifdef EXT_TRACKS
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t note_num = msg[1];
  DEBUG_PRINT(F("note on midi2 ext, "));
  DEBUG_DUMP(channel);
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {

      auto fov_y = seq_extstep_page.fov_y;
      auto cur_y = seq_ptc_page.seq_ext_pitch(note_num) + ptc_param_oct.cur * 12;
      if (cur_y > 127) { continue; }

      if (fov_y >= cur_y && cur_y != 0) {
        fov_y = cur_y - 1;
      } else if (fov_y + SeqExtStepPage::fov_notes <= cur_y) {
        fov_y = cur_y - SeqExtStepPage::fov_notes;
      }
      if (MidiClock.state != 2) {
        seq_extstep_page.fov_y = fov_y;
        seq_extstep_page.cur_y = cur_y;
      }

      if (last_ext_track != n) {
        last_ext_track = n;
        return;
      }

      mcl_seq.ext_tracks[n].record_track_noteon(cur_y, msg[2]);
      if (seq_extstep_page.x_notes_down > 0) {
        seq_extstep_page.enter_notes();
      }
      // if ((MidiClock.state != 2) || (seq_extstep_page.recording)) {
      seq_extstep_page.last_rec_event = REC_EVENT_TRIG;
      uint8_t vel = SeqPage::velocity;
      if (seq_extstep_page.recording) {
        vel = msg[2];
      }
      mcl_seq.ext_tracks[n].note_on(cur_y, vel);
      //}
      return;
    }
  }
#endif
}

void SeqExtStepMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
#ifdef EXT_TRACKS
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t note_num = msg[1];
  uint8_t pitch = seq_ptc_page.seq_ext_pitch(note_num) + ptc_param_oct.cur * 12;

  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {

      // if (MidiClock.state != 2) {
      mcl_seq.ext_tracks[n].note_off(pitch);
      // }
      if ((seq_extstep_page.recording) && (MidiClock.state == 2)) {
        mcl_seq.ext_tracks[n].note_off(pitch);
        mcl_seq.ext_tracks[n].record_track_noteoff(pitch);
      } else {
        mcl_seq.ext_tracks[n].remove_notes_on(pitch);
      }
      return;
    }
  }
#endif
}

void SeqExtStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  Midi2.addOnControlChangeCallback(this,
                                   (midi_callback_ptr_t)&SeqExtStepMidiEvents::
                                       onControlChangeCallback_Midi2);
  state = true;
}

void SeqExtStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  Midi2.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::
                onControlChangeCallback_Midi2);
  state = false;
}
