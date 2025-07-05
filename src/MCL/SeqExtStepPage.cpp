#include "MCL_impl.h"

void SeqExtStepPage::setup() {
  SeqPage::setup();
  encoder_init = true;
}
void SeqExtStepPage::config() {
#ifdef EXT_TRACKS
//  seq_extparam3.cur = mcl_seq.ext_tracks[last_ext_track].length;
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
  if (show_seq_menu) { return; }

  seq_extparam1.max = 127;
  seq_extparam2.max = 127;
  seq_extparam3.max = 128;
  seq_extparam3.min = 1;

  seq_extparam4.min = 4;
  seq_extparam4.max = 128;
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];

  if (encoder_init) {
    encoder_init = false;
    uint8_t timing_mid = active_track.get_timing_mid();
    seq_extparam1.cur = 64;

    seq_extparam2.cur = 64;

    seq_extparam4.cur = 16;
    seq_extparam3.handler = NULL;
    seq_extparam3.cur = 64;
    fov_offset = 0;
    cur_x = 0;
    fov_y = MIDI_NOTE_C3 - 1;
    cur_y = fov_y + 1;
    cur_w = timing_mid;

  }

  seq_extparam1.old = seq_extparam1.cur;
  seq_extparam2.old = seq_extparam2.cur;
  seq_extparam3.old = seq_extparam3.cur;

  config();
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);

}

void SeqExtStepPage::init() {
  page_count = 8;
  DEBUG_PRINTLN(F("seq extstep init"));

  midi_device = midi_active_peering.get_device(UART2_PORT);

  SeqPage::init();
  MD.set_rec_mode(3);
  param_select = PARAM_OFF;
  trig_interface.on();
  trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);

  last_cur_x = -1;
  config_menu_entries();
  config_encoders();
//  midi_events.setup_callbacks();

}

void SeqExtStepPage::cleanup() {
  SeqPage::cleanup();
  MD.set_rec_mode(0);
//  midi_events.remove_callbacks();
}

#define MAX_FOV_W 96

void SeqExtStepPage::draw_seq_pos() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint16_t cur_tick_x = active_track.step_count * active_track.get_timing_mid();// + active_track.mod12_counter;


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

  uint8_t m = 4, n = 16;

  switch (active_track.speed) {
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
        uint8_t v = draw_y + (k * h);
        if ((pianoroll_mode > 0 && k == 3) || i % 16 == 0) {
          draw = true;
        }
        if (pianoroll_mode == 0) {
          draw = true;
        }
        if (i % n == 0) {
          //if ((fov_y + k + i) % 2 == 0) {
          if (k % 2 == 0) {
            oled_display.drawLine(grid_fov_x, v, grid_fov_x, v + 4, WHITE);
          }
          continue;
        }
        if (draw) {
          oled_display.drawPixel(grid_fov_x, v, WHITE);
        }

        if (i % m == 0) {
          oled_display.drawPixel(grid_fov_x, v + (h / 2), WHITE);
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
        j = active_track.search_lock_idx(pianoroll_mode - 1, i, next_lock_ev,
                                         ev_j_end);
        if (next_lock_ev == 0xFFFF) {
          next_lock_ev = ev_idx;
        }

      auto &ev_j = active_track.events[next_lock_ev];

      int16_t start_x = i * timing_mid + ev.micro_timing - timing_mid;
      int16_t end_x = j * timing_mid + ev_j.micro_timing - timing_mid;
      if (start_x == end_x) { end_x = start_x - 1; }

      if (is_within_fov(start_x, end_x)) {
        uint8_t start_fov_x, end_fov_x;

        uint8_t start_y = ev.event_value;
        uint8_t end_y = ev_j.event_value;

        int16_t start_x_tmp = start_x;
        int16_t end_x_tmp = end_x;
        uint8_t start_y_tmp = start_y;
        uint8_t end_y_tmp = end_y;

        if (end_x < start_x) {
          end_x_tmp += roll_length;
        }
        float gradient;
        if (start_x == end_x || !ev.event_on) {
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

     //   if (!ev.event_on) {
          // Draw single lock.
     //     oled_display.fillRect(start_fov_x + draw_x, start_fov_y, 2, 2, WHITE);
     //   } else {
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
              draw_thick_line(start_fov_x + draw_x, start_fov_y, fov_w + draw_x, tmp_end_fov_y);
            }

            if (end_x > fov_offset) {

              uint8_t end_y_tmp =
                  ((float)(roll_length - start_x + fov_offset)) * gradient +
                  start_y;
              uint8_t tmp_end_fov_y =
                  fov_h - (((float)end_y_tmp / 128.0) * (float)fov_h);

              draw_thick_line(draw_x, !ev.event_on ? start_fov_y : tmp_end_fov_y, end_fov_x + draw_x,
                              !ev.event_on ? start_fov_y : end_fov_y);
            }

          } else {
            // Standard note.
            draw_thick_line(start_fov_x + draw_x, start_fov_y,
                            draw_x + end_fov_x, !ev.event_on ? start_fov_y : end_fov_y);
          }
        }
      }
  //  }
    /*if (j < i) {
      break;
    }*/
  }
  // Draw interactive cursor
  int16_t fov_cur_x = (float)(cur_x - fov_offset) * fov_pixels_per_tick;
  uint8_t fov_cur_y = fov_h - ((float)lock_cur_y / 128.0 * (float)fov_h);

  oled_display.drawPixel(draw_x + fov_cur_x - 1, draw_y + fov_cur_y - 2, WHITE);
  oled_display.drawPixel(draw_x + fov_cur_x + 1, draw_y + fov_cur_y - 2, WHITE);
  oled_display.drawPixel(draw_x + fov_cur_x, draw_y + fov_cur_y - 1 - 2, WHITE);
  oled_display.drawPixel(draw_x + fov_cur_x, draw_y + fov_cur_y + 1 - 2, WHITE);
}

void SeqExtStepPage::draw_note(uint8_t x, uint8_t y, uint8_t w) {
  oled_display.drawRect(x, y, w, fov_h / fov_notes, WHITE);
  oled_display.fillRect(x + 1, y + 1, w - 2, fov_h / fov_notes - 2, BLACK);
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
  for (uint8_t i = 0; i < active_track.length; i++) {
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

      int16_t note_start = i * timing_mid + ev.micro_timing - timing_mid;
      int16_t note_end = j * timing_mid + ev_j.micro_timing - timing_mid;

      if (i > j && j == 0) { note_end += timing_mid * active_track.length; }

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
              draw_note(note_fov_start + draw_x,
                                    draw_y + note_fov_y,
                                    pattern_end_fov_x - note_fov_start);
            }

            if (note_end > fov_offset) {
              draw_note(draw_x, draw_y + note_fov_y, note_fov_end);
            }

          } else {
            // Standard note.
            draw_note(note_fov_start + draw_x, draw_y + note_fov_y,
                                  note_fov_end - note_fov_start);
          }
        }
      }
    }
  }
  // Draw interactive cursor
  uint8_t fov_cur_y = fov_h - ((cur_y - fov_y) * ((fov_h) / fov_notes));
  int16_t fov_cur_x = (float)(cur_x - fov_offset) * fov_pixels_per_tick;
  uint8_t fov_cur_w = ceil((float)(cur_w)*fov_pixels_per_tick);
  if (fov_cur_x < 0) {
    fov_cur_x = 0;
  }
  if (fov_cur_x < fov_w) {
    if (fov_cur_x + fov_cur_w > fov_w) {
      fov_cur_w = fov_w - fov_cur_x;
    }
    oled_display.fillRect(draw_x + fov_cur_x, draw_y + fov_cur_y, fov_cur_w,
                        (fov_h / fov_notes), WHITE);
  }
}

void SeqExtStepPage::draw_viewport_minimap() {
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();

  constexpr uint16_t width = pidx_w * 4 + 3;

  uint16_t pattern_end = max(16,active_track.length) * timing_mid;

  uint16_t cur_tick_x =
      active_track.step_count * timing_mid + active_track.mod12_counter;

  oled_display.drawRect(pidx_x0, pidx_y, width, pidx_h, WHITE);

  // viewport is [fov_offset, fov_offset+fov_length] out of [0,
  // pattern_end]

  uint16_t s = fov_offset * (width - 1) / pattern_end;
  uint16_t w = fov_length * (width - 2) / pattern_end;
  uint16_t p = min(width, cur_tick_x * (width - 1) / pattern_end);

  oled_display.drawFastHLine(pidx_x0 + 1 + s, pidx_y + 1, w, WHITE);

  oled_display.drawPixel(pidx_x0 + 1 + p, pidx_y + 1, INVERT);
}

void SeqExtStepPage::pos_cur_x(int16_t diff) {
  uint16_t w = cur_w;
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
  for (uint8_t n = 0; n < 16; n++) {
    if (note_interface.is_note_on(n)) {
      auto &active_track = mcl_seq.ext_tracks[last_ext_track];
      uint8_t timing_mid = active_track.get_timing_mid();

      uint16_t pos = fov_offset + timing_mid * n;
      uint16_t w = cur_w;
      if (pos + w >= roll_length) { w = roll_length - pos - 1; }

      active_track.del_note(pos, w - 1, cur_y);
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

void SeqExtStepPage::pos_cur_w(int16_t diff) {
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

  } else {
    seq_menu_page.menu.enable_entry(SEQ_MENU_ARP, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_VEL, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PROB, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_PARAMSELECT, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_TRACK, false);
    seq_menu_page.menu.enable_entry(SEQ_MENU_CLEAR_LOCKS, true);
    seq_menu_page.menu.enable_entry(SEQ_MENU_SLIDE, true);
  }

}

void SeqExtStepPage::loop() {
  config_menu_entries();
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();
  SeqPage::loop();

  // If pianoroll_edit mode changed.
  if (show_seq_menu) {
    display_mute_mask(midi_device, 8);
    if (last_pianoroll_mode != pianoroll_mode) {

      if (pianoroll_mode > 0) {
        if (mcl_seq.ext_tracks[last_ext_track]
                .locks_params[pianoroll_mode - 1] == 0) {
          param_select = PARAM_OFF;
        } else {
          param_select = mcl_seq.ext_tracks[last_ext_track]
                             .locks_params[pianoroll_mode - 1] -
                         1;
        }
      }
      last_pianoroll_mode = pianoroll_mode;
    }
    if ((pianoroll_mode > 0)) {
      if (param_select == PARAM_OFF) {
        mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] = 0;
      } else {
        mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] =
            param_select + 1;
      }
    }
  }
  if (seq_extparam1.hasChanged()) {
    // Horizontal translation

    int16_t diff = seq_extparam1.cur - seq_extparam1.old;
    if (seq_extparam4.cur == zoom_max && BUTTON_DOWN(Buttons.ENCODER1)) {
      diff *= (timing_mid / 3);
    }
    pos_cur_x(diff);
    seq_extparam1.cur = 64;
    seq_extparam1.old = 64;
  }

  if (seq_extparam2.hasChanged()) {
    // Vertical translation
    int16_t diff =
        seq_extparam2.old - seq_extparam2.cur; // reverse dir for sanity.
    pos_cur_y(diff);
    seq_extparam2.cur = 64;
    seq_extparam2.old = 64;
  }

  if (seq_extparam3.hasChanged()) {
    int16_t diff = seq_extparam3.cur - seq_extparam3.old;
    pos_cur_w(diff);
  }
  seq_extparam3.cur = 64;
  seq_extparam3.old = 64;

  roll_length = active_track.length * timing_mid; // in ticks

  if (seq_extparam4.cur > zoom_max) {
      seq_extparam4.cur = zoom_max;
  }

  if (cur_w > roll_length) { cur_w = roll_length / 2; }

  int fov_length_new = active_track.length * timing_mid * fov_pixels_per_tick;
  if (fov_length_new < fov_w) {
      seq_extparam4.cur = active_track.length  * active_track.get_speed_multiplier();
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

    int x = cur_x - fov_offset;

    int fov_old_x = x * fov_pixels_per_tick;

    fov_pixels_per_tick = (float)fov_w / (float)fov_length;

    int fov_cur_x = x * fov_pixels_per_tick;

    int offset = (fov_cur_x - fov_old_x) / fov_pixels_per_tick;

    fov_offset += offset;

    if (fov_length + fov_offset > roll_length) {
      fov_offset = roll_length - fov_length;
    }
    fov_offset = max(0,fov_offset);

  }

}

void SeqExtStepPage::display() {
#ifdef EXT_TRACKS
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();
  uint8_t epoch = 0;
  do {
  oled_display.clearDisplay();
  draw_viewport_minimap();
  draw_grid();
  mcl_gui.put_value_at(cur_x/timing_mid + 1,info1);
  epoch = active_track.epoch;
  if (pianoroll_mode == 0) {
    uint8_t oct = cur_y / 12;
    uint8_t note = cur_y - 12 * (cur_y / 12);
    char str[4] = " ";
    strcpy(str + 1, number_to_note.notes_upper[note]);
    str[3] = oct + '0';
    str[4] = 0;
    strcat(info1, str);
    strcpy(info2, "NOTE");
    draw_pianoroll();
  } else {
    strcpy(info2, "LOCK  ");
    mcl_gui.put_value_at(pianoroll_mode, info2 + 5);
    mcl_gui.put_value_at(lock_cur_y, info1);
    draw_lockeditor();
  }
  } while (epoch != ExtSeqTrack::epoch);
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
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint16_t w = cur_w;
  if (cur_x + w >= roll_length) { w = roll_length - cur_x - 1; }

  for (uint8_t n = 0; n < NUM_NOTES_ON; n++) {
    if (active_track.notes_on[n].value == 255)
      continue;
    active_track.del_note(cur_x, w - 1, active_track.notes_on[n].value);
    active_track.add_note(cur_x, w, active_track.notes_on[n].value,
                          velocity, cond);
  }
}

void SeqExtStepPage::param_select_update() {
  if (pianoroll_mode > 0) {
    if (mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] ==
        0) {
      param_select = PARAM_OFF;
    } else {
      param_select =
          mcl_seq.ext_tracks[last_ext_track].locks_params[pianoroll_mode - 1] -
          1;
    }
  }
}

bool SeqExtStepPage::handleEvent(gui_event_t *event) {

  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  uint8_t timing_mid = active_track.get_timing_mid();

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;
    uint8_t track = event->source - 128;

    if (device != DEVICE_MD) {
      return true;
    }
    if (show_seq_menu) {
      if (mask == EVENT_BUTTON_PRESSED) {
        toggle_ext_mask(track);
        param_select_update();
      }
      return true;
    }

    trig_interface.send_md_leds(TRIGLED_EXCLUSIVE);

    if (mask == EVENT_BUTTON_PRESSED) {
      // cur_x = fov_offset + (float)(fov_length / 16) * (float)track;
      int a = 16 * timing_mid;
      pos_cur_x(((cur_x / a) * a + timing_mid * track) - cur_x);
      pos_cur_x(((cur_x / a) * a + timing_mid * track) - cur_x);
      note_interface.last_note = track;
    }
    if (mask == EVENT_BUTTON_RELEASED) {
      if (note_interface.notes_all_off()) {
        enter_notes();
      }
    }
    return true;
  }
  bool ignore_clear = false;

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (trig_interface.is_key_down(MDX_KEY_PATSONG)) {
      return seq_menu_page.handleEvent(event);
    }
    uint8_t inc = 1;
    int w = timing_mid;
    if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
      inc = 4;
      w = w * 2;
    }

    if (pianoroll_mode > 0) {
      inc = 1;
      w = seq_extparam4.cur / 2;
      if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
         w *= 2;
         inc = 12;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
        case MDX_KEY_SCALE: {
       //   seq_extparam4.cur = 16;
          //
          //int a = fov_length
          int a = timing_mid * 16;// / active_track.get_speed_multiplier();
          cur_x += a;
          if (cur_x > fov_offset + fov_length) { fov_offset += a; }
          if (cur_x >= roll_length) {
            cur_x = cur_x - (cur_x / a ) * a;
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
      if (trig_interface.is_key_down(MDX_KEY_YES)) {
        w = 1;
      }

      if (trig_interface.is_key_down(MDX_KEY_NO) || note_interface.notes_count_on()) {
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
            cur_w = timing_mid / 2;
          }
          return true;
        }
        case MDX_KEY_RIGHT: {
          if (cur_w == timing_mid / 2 && w > 1) {
            w = timing_mid / 2;
          }
          pos_cur_w(w);
          return true;
        }
        case MDX_KEY_CLEAR: {
          if (pianoroll_mode == 0) {
            for (uint8_t n = 0; n < 127; n++) {
              active_track.del_note(cur_x, w - 1, n);
            }
          }
          return true;
        }
        }
      } else {
        switch (key) {

        case MDX_KEY_LEFT: {
        //  if (trig_interface.is_key_down(MDX_KEY_FUNC) &&
        //      (pianoroll_mode == 0)) {
        //    mcl_seq.ext_tracks[last_ext_track].rotate_left();
        //  } else {
            pos_cur_x(-1 * w);
            if (trig_interface.is_key_down(MDX_KEY_YES)) {
              trig_interface.ignoreNextEvent(MDX_KEY_YES);
            }
        //  }
          return true;
        }
        case MDX_KEY_RIGHT: {
        //  if (trig_interface.is_key_down(MDX_KEY_FUNC) &&
       //       (pianoroll_mode == 0)) {
       //     mcl_seq.ext_tracks[last_ext_track].rotate_right();
       //   } else {
            pos_cur_x(w);
            if (trig_interface.is_key_down(MDX_KEY_YES)) {
              trig_interface.ignoreNextEvent(MDX_KEY_YES);
            }
       //   }
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
        ignore_clear = true;
        goto YES;
      }
      }
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
  YES:
    if (pianoroll_mode >= 1) {

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
      if (!clear && active_track.locks_params[lock_idx] - 1 > 0) {
        active_track.set_track_locks(step, utiming,
                                     active_track.locks_params[lock_idx] - 1,
                                     lock_cur_y, slide, lock_idx);
      }
      DEBUG_DUMP(active_track.count_lock_event(step, lock_idx));
      return true;
    } else {
      DEBUG_PRINTLN("here");
      DEBUG_PRINTLN(active_track.notes_on_count);
      if (active_track.notes_on_count > 0) {
        enter_notes();
      } else {
        uint16_t w = cur_w;
        if (cur_x + w >= roll_length) { w = roll_length - cur_x - 1; }

        if (!active_track.del_note(cur_x, w - 1, cur_y)) {
          active_track.add_note(cur_x, w, cur_y, velocity, cond);
        }
      }
      return true;
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    toggle_record();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    mute_mask = 128;
    param_select_update();
  }
  if (SeqPage::handleEvent(event)) {
    return true;
  }
  return false;
}

void SeqExtStepMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  /*
    uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
    uint8_t param = msg[1];
    uint8_t value = msg[2];
    for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
      if (mcl_seq.ext_tracks[n].channel == channel) {
        if (SeqPage::pianoroll_mode > 0) {
          if (mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] -
                  1 ==
              PARAM_LEARN) {
            mcl_seq.ext_tracks[n].locks_params[SeqPage::pianoroll_mode - 1] =
                param + 1;
            SeqPage::param_select = param;
          }
        }
        if (SeqPage::recording) {
          DEBUG_DUMP("Record... cc");
          mcl_seq.ext_tracks[n].record_track_locks(param, value,
    SeqPage::slide); mcl_seq.ext_tracks[n].update_param(param, value);
        }
        return;
      }
    }
  */
}

void SeqExtStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  // Step edit for ExtSeq
  // For each incoming note, check to see if note interface has any steps
  // selected For selected steps record notes.
#ifdef EXT_TRACKS
/*
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t note_num = msg[1];
  DEBUG_PRINT(F("note on midi2 ext, "));
  DEBUG_DUMP(channel);
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {

      auto fov_y = seq_extstep_page.fov_y;
      auto cur_y =
          seq_ptc_page.seq_ext_pitch(note_num) + ptc_param_oct.cur * 12;
      if (cur_y > 127) {
        continue;
      }

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
*/
#endif
}

void SeqExtStepMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
#ifdef EXT_TRACKS
/*
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
  */
#endif
}

void SeqExtStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  /*
    Midi2.addOnNoteOnCallback(
        this,
    (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
    Midi2.addOnNoteOffCallback(
        this,
        (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
    Midi2.addOnControlChangeCallback(this,
                                     (midi_callback_ptr_t)&SeqExtStepMidiEvents::
                                         onControlChangeCallback_Midi2);
  */
  state = true;
}

void SeqExtStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  /*
    Midi2.removeOnNoteOnCallback(
        this,
    (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
    Midi2.removeOnNoteOffCallback(
        this,
        (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
    Midi2.removeOnControlChangeCallback(
        this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::
                  onControlChangeCallback_Midi2);
  */
  state = false;
}
