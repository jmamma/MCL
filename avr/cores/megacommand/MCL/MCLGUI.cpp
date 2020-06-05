#include "MCL.h"

void MCLGUI::draw_textbox(char *text, char *text2) {
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();
  oled_display.setFont();
  uint8_t font_width = 6;
  uint8_t w = ((strlen(text) + strlen(text2) + 2) * font_width);
  uint8_t x = 64 - w / 2;
  uint8_t y = 8;

  oled_display.fillRect(x - 1, y - 1, w + 2, 8 * 2 + 2, BLACK);
  oled_display.drawRect(x, y, w, 8 * 2, WHITE);
  oled_display.setCursor(x + font_width, y + 4);
  oled_display.print(text);
  oled_display.print(text2);
  oled_display.display();
  oled_display.setFont(oldfont);
#endif
}

bool MCLGUI::wait_for_input(char *dst, const char *title, uint8_t len) {
  text_input_page.init();
  text_input_page.init_text(dst, title, len);
  GUI.pushPage(&text_input_page);
  while (GUI.currentPage() == &text_input_page) {
    GUI.loop();
  }
  m_trim_space(dst);
  return text_input_page.return_state;
}

bool MCLGUI::wait_for_confirm(const char *title, const char *text) {
  questiondialog_page.init(title, text);
  GUI.pushPage(&questiondialog_page);
  while (GUI.currentPage() == &questiondialog_page) {
    GUI.loop();
  }
  return questiondialog_page.return_state;
}

void MCLGUI::draw_vertical_dashline(uint8_t x, uint8_t from, uint8_t to) {
#ifdef OLED_DISPLAY
  for (uint8_t y = from; y < to; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
#endif
}

void MCLGUI::draw_horizontal_dashline(uint8_t y, uint8_t from, uint8_t to) {
#ifdef OLED_DISPLAY
  for (uint8_t x = from; x < to; x += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
#endif
}

void MCLGUI::draw_horizontal_arrow(uint8_t x, uint8_t y, uint8_t w) {
#ifdef OLED_DISPLAY
  oled_display.drawFastHLine(x, y, w, WHITE);
  oled_display.drawFastVLine(x + w - 2, y - 1, 3, WHITE);
  oled_display.drawFastVLine(x + w - 3, y - 2, 5, WHITE);
#endif
}

void MCLGUI::draw_vertical_separator(uint8_t x) {
#ifdef OLED_DISPLAY
  auto x_ = x + 2;
  for (uint8_t y = 0; y < 32; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
    oled_display.drawPixel(x_, y, WHITE);
  }
  x_ = x + 1;
  for (uint8_t y = 1; y < 32; y += 2) {
    oled_display.drawPixel(x_, y, WHITE);
  }
#endif
}

void MCLGUI::draw_vertical_scrollbar(uint8_t x, uint8_t n_items,
                                     uint8_t n_window, uint8_t n_current) {
#ifdef OLED_DISPLAY
  uint8_t length = round(((float)(n_window - 1) / (float)(n_items - 1)) * 32);
  uint8_t y = round(((float)(n_current) / (float)(n_items - 1)) * 32);
  mcl_gui.draw_vertical_separator(x + 1);
  oled_display.fillRect(x + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x, y, 5, length, WHITE);
#endif
}

void MCLGUI::draw_knob_frame() {
#ifdef OLED_DISPLAY
  for (uint8_t x = knob_x0; x <= knob_xend; x += knob_w) {
    mcl_gui.draw_vertical_dashline(x, 0, knob_y);
    oled_display.drawPixel(x, knob_y, WHITE);
    oled_display.drawPixel(x, knob_y + 1, WHITE);
  }
  mcl_gui.draw_horizontal_dashline(knob_y, knob_x0 + 1, knob_xend + 1);
#endif
}

void MCLGUI::draw_knob(uint8_t i, const char *title, const char *text) {
  uint8_t x = knob_x0 + i * knob_w;
  draw_text_encoder(x, knob_y0, title, text);
}

void MCLGUI::draw_knob(uint8_t i, Encoder *enc, const char *title) {
  uint8_t x = knob_x0 + i * knob_w;
  draw_light_encoder(x + 7, 6, enc, title);
}

static char title_buf[16];

//  ref: Design/popup_menu.png
void MCLGUI::draw_popup(const char *title, bool deferred_display, uint8_t h) {
#ifdef OLED_DISPLAY
  strcpy(title_buf, title);
  m_toupper(title_buf);

  if (h == 0) {
    h = s_menu_h;
  }
  oled_display.setFont(&TomThumb);

  // draw menu body
  oled_display.fillRect(s_menu_x - 1, s_menu_y - 2, s_menu_w + 2, h + 2, BLACK);
  oled_display.drawRect(s_menu_x, s_menu_y, s_menu_w, h, WHITE);
  oled_display.fillRect(s_menu_x + 1, s_menu_y - 1, s_menu_w - 2, 5, WHITE);

  // draw the title '____/**********\____' part
  oled_display.drawRect(s_title_x, s_menu_y - 4, s_title_w, 4, BLACK);
  oled_display.fillRect(s_title_x, s_menu_y - 3, s_title_w, 3, WHITE);
  oled_display.drawPixel(s_title_x, s_menu_y - 3, BLACK);
  oled_display.drawPixel(s_title_x + s_title_w - 1, s_menu_y - 3, BLACK);

  oled_display.setTextColor(BLACK);
  auto len = strlen(title_buf) * 4;
  oled_display.setCursor(s_title_x + (s_title_w - len) / 2, s_menu_y + 3);
  // oled_display.setCursor(s_title_x + 2, s_menu_y + 3);
  oled_display.println(title_buf);
  oled_display.setTextColor(WHITE);
  if (!deferred_display) {
    oled_display.display();
  }
#endif
}

void MCLGUI::clear_popup(uint8_t h) {
#ifdef OLED_DISPLAY
  if (h == 0) {
    h = s_menu_h;
  }
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 4, s_menu_w - 2, h - 5, BLACK);
#endif
}

void MCLGUI::draw_progress(const char *msg, uint8_t cur, uint8_t _max,
                           bool deferred_display, uint8_t x_pos,
                           uint8_t y_pos) {

  draw_popup(msg, true);
  draw_progress_bar(cur, _max, deferred_display, x_pos, y_pos);
}
void MCLGUI::draw_progress_bar(uint8_t cur, uint8_t _max, bool deferred_display,
                               uint8_t x_pos, uint8_t y_pos) {
#ifdef OLED_DISPLAY
  oled_display.fillRect(x_pos + 1, y_pos + 1, s_progress_w - 2,
                        s_progress_h - 2, BLACK);

  float prog = (float)cur / (float)_max;
  auto progx = (uint8_t)(x_pos + 1 + prog * (s_progress_w - 2));
  // draw the progress
  oled_display.fillRect(x_pos + 1, y_pos + 1, progx - x_pos - 1,
                        s_progress_h - 2, WHITE);

  uint8_t shift = 1;

  // draw the '///////' pattern, using circular shifting
  uint8_t x = 0;

  uint8_t bitmask = s_progress_cookie;
  uint8_t temp_bitmask = s_progress_cookie;

  for (uint8_t i = x_pos + 1; i <= progx; i += 1) {

    for (uint8_t n = 0; n < s_progress_h - 2; n++) {
      uint8_t a = s_progress_h - 2 - n;

      if (IS_BIT_SET(temp_bitmask, a)) {
        oled_display.drawPixel(i, y_pos + 1 + n, BLACK);
      }
    }
    ROTATE_LEFT(temp_bitmask, 8);
  }
  if (s_progress_count == s_progress_speed) {
    s_progress_cookie = bitmask;
    ROTATE_LEFT(s_progress_cookie, 8);
    s_progress_count = 0;
  }
  s_progress_count++;
  oled_display.drawRect(x_pos, y_pos, s_progress_w, s_progress_h, WHITE);

  if (!deferred_display) {
    oled_display.display();
  }
#endif
}

//  ref: Design/infobox.png
void MCLGUI::draw_infobox(const char *line1, const char *line2,
                          const int line2_offset) {
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();

  oled_display.fillRect(dlg_info_x1 - 1, dlg_info_y1 - 1, dlg_info_w + 3,
                        dlg_info_h + 3, BLACK);
  oled_display.drawRect(dlg_info_x1, dlg_info_y1, dlg_info_w, dlg_info_h,
                        WHITE);
  oled_display.drawFastHLine(dlg_info_x1 + 1, dlg_info_y2 + 1, dlg_info_w,
                             WHITE);
  oled_display.drawFastVLine(dlg_info_x2 + 1, dlg_info_y1 + 1, dlg_info_h - 1,
                             WHITE);
  oled_display.fillRect(dlg_info_x1 + 1, dlg_info_y1 + 1, dlg_info_w - 2, 6,
                        WHITE);

  oled_display.fillCircle(dlg_circle_x, dlg_circle_y, 6, WHITE);
  oled_display.fillRect(dlg_circle_x - 1, dlg_circle_y - 3, 3, 4, BLACK);
  oled_display.fillRect(dlg_circle_x - 1, dlg_circle_y + 2, 3, 2, BLACK);

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(dlg_info_x1 + 4, dlg_info_y1 + 6);
  strcpy(title_buf, line1);
  m_toupper(title_buf);
  oled_display.println(title_buf);

  oled_display.setTextColor(WHITE);
  oled_display.setCursor(dlg_info_x1 + 23, dlg_info_y1 + 16 + line2_offset);
  oled_display.println(line2);

  oled_display.setFont(oldfont);
#endif
}

void MCLGUI::draw_encoder(uint8_t x, uint8_t y, uint8_t value) {
#ifdef OLED_DISPLAY
  bool vert_flip = false;
  bool horiz_flip = false;
  uint8_t image_w = 11;
  uint8_t image_h = 11;

  // Scale encoder values to 123. encoder animation does not start and stop on
  // 0.
  value = (uint8_t)((float)value * .95);

  value += 4;

  if (value < 32) {
    vert_flip = false;
    horiz_flip = false;
  } else if (value < 64) {
    vert_flip = false;
    horiz_flip = true;
    value = 32 - (value - 32);
  } else if (value < 96) {
    vert_flip = true;
    horiz_flip = true;
    value = value - 64;
  } else {
    vert_flip = true;
    horiz_flip = false;
    if (value > 122) {
      value = 122;
    }
    value = 32 - (value - 96);
  }

  if (value < 4) {
    oled_display.drawBitmap(x, y, encoder_small_0, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  } else if (value < 9) {
    oled_display.drawBitmap(x, y, encoder_small_1, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  } else if (value < 14) {
    oled_display.drawBitmap(x, y, encoder_small_2, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  } else if (value < 19) {
    oled_display.drawBitmap(x, y, encoder_small_3, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  } else if (value < 24) {
    oled_display.drawBitmap(x, y, encoder_small_4, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  } else if (value < 30) {
    oled_display.drawBitmap(x, y, encoder_small_5, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  } else {
    oled_display.drawBitmap(x, y, encoder_small_6, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);
  }
#endif
}

void MCLGUI::draw_encoder(uint8_t x, uint8_t y, Encoder *encoder) {
  draw_encoder(x, y, encoder->cur);
}

bool MCLGUI::show_encoder_value(Encoder *encoder) {
  uint8_t match = 255;

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS && match == 255; i++) {
    if (((LightPage *)GUI.currentPage())->encoders[i] == encoder) {
      match = i;
    }
  }

  if (match != 255) {
    if (clock_diff(((LightPage *)GUI.currentPage())->encoders_used_clock[match],
                   slowclock) < SHOW_VALUE_TIMEOUT) {
      return true;
    } else {
      ((LightPage *)GUI.currentPage())->encoders_used_clock[match] =
          slowclock - SHOW_VALUE_TIMEOUT - 1;
    }
  }

  return false;
}

void MCLGUI::draw_text_encoder(uint8_t x, uint8_t y, const char *name,
                               const char *value) {
#ifdef OLED_DISPLAY
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(x + 4, y + 6);
  oled_display.print(name);

  oled_display.setFont();
  oled_display.setCursor(x + 4, y + 8);
  oled_display.print(value);
#endif
}

void MCLGUI::draw_md_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                             const char *name) {
  bool show_value = show_encoder_value(encoder);
  draw_md_encoder(x, y, encoder->cur, name, show_value);
}

void MCLGUI::draw_md_encoder(uint8_t x, uint8_t y, uint8_t value,
                             const char *name, bool show_value) {
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();

  uint8_t image_w = 11;
  uint8_t image_h = 11;

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);
  uint8_t x_offset = x;
  // Find the encoder number matching the encoder.
  if (strlen(name) == 2) {
    x_offset += 2;
  }
  oled_display.setCursor(x_offset, y);
  oled_display.print(name);

  y += 5;

  draw_encoder(x, y, value);

  oled_display.drawPixel(x + image_w / 2, y - 3, WHITE);
  oled_display.drawPixel(x, y + image_h + 2, WHITE);
  oled_display.drawPixel(x + image_w - 1, y + image_h + 2, WHITE);

  x_offset = x;
  if (show_value) {
    if (value < 10) {
      x_offset += 2;
    }
    if (value < 100) {
      x_offset += 2;
    }

    oled_display.setCursor(x_offset, y + image_h + 1 + 8);

    oled_display.print(value);
  }

  oled_display.setFont(oldfont);
#endif
}

void MCLGUI::draw_light_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                                const char *name) {
  bool show_value = show_encoder_value(encoder);
  draw_light_encoder(x, y, encoder->cur, name, show_value);
}

void MCLGUI::draw_light_encoder(uint8_t x, uint8_t y, uint8_t value,
                                const char *name, bool show_value) {
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  oled_display.setTextColor(WHITE);

  uint8_t x_offset = x;

  if (show_value) {
    if (value < 10) {
      x_offset += 2;
    }
    if (value < 100) {
      x_offset += 2;
    }

    oled_display.setCursor(x_offset, y);

    oled_display.print(value);
  } else {

    if (strlen(name) == 2) {
      x_offset += 2;
    }
    oled_display.setCursor(x_offset, y);
    oled_display.print(name);
  }
  y += 2;

  draw_encoder(x, y, value);

  oled_display.setFont(oldfont);
#endif
}

void MCLGUI::draw_microtiming(uint8_t speed, uint8_t timing) {
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  oled_display.setTextColor(WHITE);
  SeqTrack seq_track;
  uint8_t timing_mid = seq_track.get_timing_mid(speed);
  uint8_t heights_len;
  uint8_t degrees = timing_mid * 2;
  uint8_t heights[16];
  // Triplets
  if (speed == SEQ_SPEED_2X) {
    uint8_t heights_lowres[6] = {11, 4, 6, 10, 4, 8};
    memcpy(&heights, &heights_lowres, 6);
    heights_len = 6;
  } else if (speed == SEQ_SPEED_3_4X) {
    uint8_t heights_triplets[16] = {11, 2, 4, 8, 6,  10, 6, 2,
                                    10, 2, 6, 8, 10, 4,  6, 2};
    memcpy(&heights, &heights_triplets, 16);
    heights_len = 16;
  } else if (speed == SEQ_SPEED_3_2X) {
    uint8_t heights_triplets2[8] = {11, 4, 8, 10, 2, 8, 4, 2};
    memcpy(&heights, &heights_triplets2, 8);
    heights_len = 8;
  } else {
    uint8_t heights_highres[12] = {11, 2, 4, 8, 6, 2, 10, 2, 6, 8, 4, 2};
    memcpy(&heights, &heights_highres, 12);
    heights_len = 12;
  }
  uint8_t y_pos = 11;
  uint8_t a = 0;
  uint8_t w = 96;
  uint8_t x_pos = 64 - (w / 2);
  uint8_t x_w = (w / (degrees));
  bool degree_scalar = false;
  if (x_w == 0) {
    x_w = 1;
    degree_scalar = true;
  }

  uint8_t x = x_pos;
  char K[4];
  strcpy(K, "--");
  K[3] = '\0';

   if (timing == 0) {
   } else if ((timing < timing_mid) && (timing != 0)) {
     itoa(timing_mid - timing, K + 1, 10);
   } else {
     K[0] = '+';
     itoa(timing - timing_mid, K + 1, 10);
   }


  oled_display.fillRect(8, 1, 128 - 16, 32 - 2, BLACK);
  oled_display.drawRect(8 + 1, 1 + 1, 128 - 16 - 2, 32 - 2 - 2, WHITE);

  oled_display.setCursor(x_pos + 34, 10);
  oled_display.print("uTIMING: ");
  oled_display.print(K);
  oled_display.drawLine(x, y_pos + heights[0], x + w, y_pos + heights[0],
                        WHITE);
  for (uint8_t n = 0; n <= degrees; n++) {
    oled_display.drawLine(x, y_pos + heights[0], x,
                          y_pos + heights[0] - heights[a], WHITE);
    a++;

    if (n == timing) {
      oled_display.fillRect(x - 1, y_pos + heights[0] + 3, 3, 3, WHITE);
      oled_display.drawPixel(x, y_pos + heights[0] + 2, WHITE);
    }

    if (a == heights_len) {
      a = 0;
    }
    if (degree_scalar) {
      if (n % 2 == 0) {
        x += x_w;
      }
    } else {
      x += x_w;
    }
  }
  oled_display.setFont(oldfont);
#endif
}

void MCLGUI::draw_keyboard(uint8_t x, uint8_t y, uint8_t note_width,
                           uint8_t note_height, uint8_t num_of_notes,
                           uint64_t note_mask) {
#ifdef OLED_DISPLAY
  const uint16_t chromatic = 0b0000010101001010;
  const uint8_t half = note_height / 2;
  const uint8_t y2 = y + note_height - 1;
  const uint8_t wm1 = note_width - 1;

  uint8_t note_type = 0;

  bool last_black = false;

  // draw first '|'
  oled_display.drawFastVLine(x, y, note_height, WHITE);

  for (uint8_t n = 0; n < num_of_notes; n++) {

    bool pressed = IS_BIT_SET64(note_mask, n);
    bool black = IS_BIT_SET16(chromatic, note_type);

    if (black) {
      // previous '|' has already filled the center col.
      oled_display.drawRect(x - 1, y + 1, 3, half - 1, WHITE);
      if (pressed) {
        oled_display.drawFastVLine(x, y + 1, half - 2, BLACK);
      }
    } else {
      if (pressed && last_black) {
        oled_display.fillRect(x + 2, y, note_width - 2, half, WHITE);
        oled_display.fillRect(x + 1, y + half, wm1, half + 1, WHITE);
        oled_display.drawPixel(x + 1, y, WHITE);
      } else if (pressed) {
        oled_display.fillRect(x, y, note_width, note_height, WHITE);
      } else {
        // draw ']'
        oled_display.drawFastHLine(x, y, note_width, WHITE);
        oled_display.drawFastHLine(x, y2, note_width, WHITE);
        oled_display.drawFastVLine(x + wm1, y, note_height, WHITE);
      }

      x += wm1;
    }

    last_black = black;

    note_type++;
    if (note_type == 12) {
      note_type = 0;
    }
  }
#endif
}

void MCLGUI::draw_trigs(uint8_t x, uint8_t y, uint8_t offset,
                        uint64_t pattern_mask, uint8_t step_count,
                        uint8_t length, uint64_t mute_mask) {
#ifdef OLED_DISPLAY
  for (int i = 0; i < 16; i++) {

    uint8_t idx = i + offset;
    bool in_range = idx < length;

    if (note_interface.notes[i] == 1) {
      // TI feedback
      oled_display.fillRect(x + 1, y + 1, seq_w - 2, trig_h - 2, WHITE);
      // oled_display.fillRect(x, y, seq_w, trig_h, WHITE);
    } else if (!in_range) {
      // don't draw
    } else {
      if (IS_BIT_SET64(pattern_mask, i + offset) &&
          ((i + offset != step_count) || (MidiClock.state != 2))) {
        oled_display.drawRect(x, y, seq_w, trig_h, WHITE);
        if (IS_BIT_SET64(mute_mask, i + offset)) {
          oled_display.drawPixel(x + 1, y + 1, WHITE);
          oled_display.drawPixel(x + 2, y + 2, WHITE);
          oled_display.drawPixel(x + 3, y + 3, WHITE);
        }
        /*If the bit is set, there is a trigger at this position. */
        else {
          oled_display.fillRect(x, y, seq_w, trig_h, WHITE);
        }
        /*
        oled_display.drawRect(x, y, seq_w, trig_h, WHITE);
        oled_display.drawPixel(x + 1, y + 1, WHITE);
        oled_display.drawPixel(x + 3, y + 1, WHITE);
        oled_display.drawPixel(x + 1, y + 3, WHITE);
        oled_display.drawPixel(x + 3, y + 3, WHITE);
        oled_display.drawPixel(x + 2, y + 2, WHITE);
       */
      } else {
        oled_display.drawRect(x, y, seq_w, trig_h, WHITE);
      }
    }

    x += seq_w + 1;
  }
#endif
}

void MCLGUI::draw_ext_track(uint8_t x, uint8_t y, uint8_t offset,
                            uint8_t ext_trackid, bool show_current_step) {
#ifdef OLED_DISPLAY
#ifdef EXT_TRACKS
  int8_t note_held = 0;
  auto &active_track = mcl_seq.ext_tracks[ext_trackid];
  for (int i = 0; i < active_track.length; i++) {

    uint8_t step_count = active_track.step_count;
    uint8_t noteson = 0;
    uint8_t notesoff = 0;
    bool in_range = (i >= offset) && (i < offset + 16);
    bool right_most = (i == active_track.length - 1);

    for (uint8_t a = 0; a < 4; a++) {
      if (active_track.notes[a][i] > 0) {
        noteson++;
      }
      if (active_track.notes[a][i] < 0) {
        notesoff++;
      }
    }

    note_held += noteson;
    note_held -= notesoff;

    if (!in_range) {
      continue;
    }

    if (note_interface.notes[i - offset] == 1) {
      oled_display.fillRect(x, y, seq_w, trig_h, WHITE);
    } else if (!note_held) { // --
      oled_display.drawFastHLine(x - 1, y + 2, seq_w + 2, WHITE);
    } else { // draw top, bottom
      oled_display.drawFastHLine(x - 1, y, seq_w + 2, WHITE);
      oled_display.drawFastHLine(x - 1, y + trig_h - 1, seq_w + 2, WHITE);
    }

    if (noteson > 0 || notesoff > 0) { // left |
      oled_display.drawFastVLine(x - 1, y, trig_h, WHITE);
    }

    if (right_most && note_held) { // right |
      oled_display.drawFastVLine(x + seq_w, y, trig_h, WHITE);
    }

    if ((step_count == i) && (MidiClock.state == 2) && show_current_step) {
      oled_display.fillRect(x, y, seq_w, trig_h, INVERT);
    }

    x += seq_w + 1;
  }
#endif // EXT_TRACKS
#endif
}

void MCLGUI::draw_leds(uint8_t x, uint8_t y, uint8_t offset, uint64_t lock_mask,
                       uint8_t step_count, uint8_t length,
                       bool show_current_step) {
#ifdef OLED_DISPLAY
  for (int i = 0; i < 16; i++) {

    uint8_t idx = i + offset;
    bool in_range = idx < length;
    bool current =
        show_current_step && step_count == idx && MidiClock.state == 2;
    bool locked = in_range && IS_BIT_SET64(lock_mask, i + offset);

    //    if (note_interface.notes[i] == 1) {
    // TI feedback
    //     oled_display.drawRect(x, y, seq_w, led_h, WHITE);
    if (!in_range) {
      // don't draw
    } else if (current ^ locked) {
      // highlight
      oled_display.fillRect(x, y, seq_w, led_h, WHITE);
    } else {
      // (current && locked) or (not current && not locked), frame only
      oled_display.drawRect(x, y, seq_w, led_h, WHITE);
    }

    x += seq_w + 1;
  }
#endif
}

void MCLGUI::draw_panel_toggle(const char *s1, const char *s2, bool s1_active) {
#ifdef OLED_DISPLAY
  oled_display.setFont(&TomThumb);
  if (s1_active) {
    oled_display.fillRect(pane_label_x, pane_label_md_y, pane_label_w,
                          pane_label_h, WHITE);
    oled_display.setCursor(pane_label_x + 1, pane_label_md_y + 6);
    oled_display.setTextColor(BLACK);
    oled_display.print(s1);
    oled_display.setTextColor(WHITE);
  } else {
    oled_display.setCursor(pane_label_x + 1, pane_label_md_y + 6);
    oled_display.setTextColor(WHITE);
    oled_display.print(s1);
    oled_display.fillRect(pane_label_x, pane_label_ex_y, pane_label_w,
                          pane_label_h, WHITE);
    oled_display.setTextColor(BLACK);
  }
  oled_display.setCursor(pane_label_x + 1, pane_label_ex_y + 6);
  oled_display.print(s2);
  oled_display.setTextColor(WHITE);
#endif
}

void MCLGUI::draw_panel_labels(const char *info1, const char *info2) {
#ifdef OLED_DISPLAY
  oled_display.setFont(&TomThumb);
  oled_display.fillRect(0, pane_info1_y, pane_w, pane_info_h, WHITE);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(1, pane_info1_y + 6);
  oled_display.print(info1);
  oled_display.fillRect(0, pane_info2_y, pane_w, pane_info_h, BLACK);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(1, pane_info2_y + 6);
  oled_display.print(info2);
#endif
}

void MCLGUI::draw_panel_status(bool recording, bool playing) {
#ifdef OLED_DISPLAY
  if (recording) {
    oled_display.fillRect(pane_cir_x1, pane_tri_y, 4, 5, WHITE);
    oled_display.drawPixel(pane_cir_x1, pane_tri_y, BLACK);
    oled_display.drawPixel(pane_cir_x2, pane_tri_y, BLACK);
    oled_display.drawPixel(pane_cir_x1, pane_tri_y + 4, BLACK);
    oled_display.drawPixel(pane_cir_x2, pane_tri_y + 4, BLACK);
  } else if (playing) {
    oled_display.drawLine(pane_tri_x, pane_tri_y, pane_tri_x, pane_tri_y + 4,
                          WHITE);
    oled_display.fillTriangle(pane_tri_x + 1, pane_tri_y, pane_tri_x + 3,
                              pane_tri_y + 2, pane_tri_x + 1, pane_tri_y + 4,
                              WHITE);
  } else {
    oled_display.fillRect(pane_tri_x, pane_tri_y, 4, 5, WHITE);
  }
#endif
}

void MCLGUI::clear_leftpane() {
#ifdef OLED_DISPLAY
  oled_display.fillRect(0, 0, pane_w, 32, BLACK);
#endif
}

void MCLGUI::clear_rightpane() {
#ifdef OLED_DISPLAY
  oled_display.fillRect(pane_w, 0, 128 - pane_w, 32, BLACK);
#endif
}

void MCLGUI::draw_panel_number(uint8_t number) {
#ifdef OLED_DISPLAY
  oled_display.setTextColor(WHITE);
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(pane_trackid_x, pane_trackid_y);
  if (number < 10) {
    oled_display.print('0');
  }
  oled_display.print(number);
#endif
}

//  ================ SPRITES ================

const unsigned char encoder_small_0[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0x80, 0x20, 0x80,
    0x20, 0x80, 0x20, 0x4e, 0x40, 0x4e, 0x40, 0x31, 0x80, 0x0e, 0x00};
// 'encoder1', 11x11px
const unsigned char encoder_small_1[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0x80, 0x20, 0x80,
    0x20, 0x80, 0x20, 0x5c, 0x40, 0x4c, 0x40, 0x31, 0x80, 0x0e, 0x00};
// 'encoder2', 11x11px
const unsigned char encoder_small_2[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0x80, 0x20, 0x80,
    0x20, 0x90, 0x20, 0x58, 0x40, 0x48, 0x40, 0x31, 0x80, 0x0e, 0x00};
// 'encoder3', 11x11px
const unsigned char encoder_small_3[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0x80, 0x20, 0x80,
    0x20, 0xb0, 0x20, 0x58, 0x40, 0x40, 0x40, 0x31, 0x80, 0x0e, 0x00};
// 'encoder4', 11x11px
const unsigned char encoder_small_4[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0x80, 0x20, 0xb0,
    0x20, 0xb0, 0x20, 0x58, 0x40, 0x40, 0x40, 0x31, 0x80, 0x0e, 0x00};
// 'encoder5', 11x11px
const unsigned char encoder_small_5[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0x80, 0x20, 0xb0,
    0x20, 0xb0, 0x20, 0x50, 0x40, 0x40, 0x40, 0x31, 0x80, 0x0e, 0x00};
// 'encoder6', 11x11px
const unsigned char encoder_small_6[] PROGMEM = {
    0x0e, 0x00, 0x31, 0x80, 0x40, 0x40, 0x40, 0x40, 0xb0, 0x20, 0xb0,
    0x20, 0xb0, 0x20, 0x40, 0x40, 0x40, 0x40, 0x31, 0x80, 0x0e, 0x00};

// 'wheel1', 19x19px
const unsigned char wheel_top[] PROGMEM = {
    0x03, 0xf8, 0x00, 0x0e, 0x0e, 0x00, 0x1e, 0x0f, 0x00, 0x3e, 0x0f, 0x80,
    0x7f, 0x1f, 0xc0, 0x7f, 0x1f, 0xc0, 0xff, 0xbf, 0xe0, 0xff, 0xff, 0xe0,
    0xff, 0xbf, 0xe0, 0xff, 0x5f, 0xe0, 0xff, 0xbf, 0xe0, 0xf8, 0xe3, 0xe0,
    0x60, 0xe0, 0xe0, 0x40, 0xe0, 0x40, 0x61, 0xf0, 0xc0, 0x31, 0xf1, 0x80,
    0x1b, 0xf7, 0x00, 0x0f, 0xfe, 0x00, 0x03, 0xf8, 0x00};
// 'wheel2', 19x19px
const unsigned char wheel_angle[] PROGMEM = {
    0x03, 0xf8, 0x00, 0x0f, 0xfe, 0x00, 0x1f, 0xfb, 0x00, 0x3f, 0xf1, 0x80,
    0x7f, 0xf0, 0xc0, 0x7f, 0xe0, 0x40, 0xff, 0xe0, 0xe0, 0x8f, 0xe3, 0xe0,
    0x83, 0xbf, 0xe0, 0x81, 0x5f, 0xe0, 0x83, 0xbf, 0xe0, 0x8f, 0xff, 0xe0,
    0xff, 0xbf, 0xe0, 0x7f, 0x1f, 0xc0, 0x7f, 0x1f, 0xc0, 0x3e, 0x0f, 0x80,
    0x1e, 0x0f, 0x00, 0x0e, 0x0e, 0x00, 0x03, 0xf8, 0x00};
// 'wheel3', 19x19px
const unsigned char wheel_side[] PROGMEM = {
    0x03, 0xf8, 0x00, 0x0f, 0xfe, 0x00, 0x1b, 0xff, 0x00, 0x31, 0xff, 0x80,
    0x61, 0xff, 0xc0, 0x40, 0xff, 0xc0, 0xe0, 0xff, 0xe0, 0xf8, 0xfe, 0x20,
    0xff, 0xb8, 0x20, 0xff, 0x50, 0x20, 0xff, 0xb8, 0x20, 0xf8, 0xfe, 0x20,
    0xe0, 0xff, 0xe0, 0x40, 0xff, 0xc0, 0x61, 0xff, 0xc0, 0x31, 0xff, 0x80,
    0x1b, 0xff, 0x00, 0x0f, 0xfe, 0x00, 0x03, 0xf8, 0x00};

// 'chroma', 24x25px
const unsigned char icon_chroma[] PROGMEM = {
    0x00, 0x00, 0x00, 0x75, 0x77, 0x00, 0x45, 0x55, 0x03, 0x47, 0x65,
    0x0f, 0x75, 0x57, 0x39, 0x00, 0x00, 0xe7, 0x00, 0x03, 0x9c, 0x00,
    0x0f, 0xf1, 0x00, 0x3f, 0xc2, 0x00, 0xff, 0x09, 0x03, 0xfc, 0x64,
    0x0f, 0xf0, 0xb2, 0x37, 0xc2, 0x59, 0xd7, 0x09, 0x2d, 0xdc, 0x64,
    0x96, 0x70, 0xb2, 0x59, 0x82, 0x59, 0x67, 0xc9, 0x2d, 0x9e, 0xec,
    0x96, 0x78, 0x76, 0x59, 0xe0, 0x3b, 0x67, 0x80, 0x1d, 0x9e, 0x00,
    0x0e, 0x78, 0x00, 0x06, 0xe0, 0x00, 0x02, 0x80, 0x00};

// 'rec', 24x15px
const unsigned char icon_rec[] PROGMEM = {
    0x3f, 0xff, 0xfc, 0x40, 0x00, 0x02, 0x40, 0x00, 0x02, 0x80, 0x00, 0x01,
    0xb9, 0xe7, 0x39, 0xa5, 0x08, 0x7d, 0xa5, 0x08, 0x7d, 0xbd, 0xc8, 0x7d,
    0xa5, 0x08, 0x7d, 0xa5, 0x08, 0x7d, 0xa5, 0xe7, 0x39, 0x80, 0x00, 0x01,
    0x40, 0x00, 0x02, 0x40, 0x00, 0x02, 0x3f, 0xff, 0xfc};

// 'grid', 24x15px
const unsigned char icon_grid[] PROGMEM = {
    0xc0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x36, 0xdb, 0x6c, 0x36, 0xdb, 0x6c,
    0x00, 0x00, 0x00, 0x36, 0xdb, 0x6c, 0x36, 0xdb, 0x6c, 0x00, 0x00, 0x00,
    0x36, 0xdb, 0x6c, 0x36, 0xdb, 0x6c, 0x00, 0x00, 0x00, 0x36, 0xdb, 0x6c,
    0x36, 0xdb, 0x6c, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03};

// 'lfo', 24x24px
const unsigned char icon_lfo[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0xff, 0x10, 0x1e, 0x00,
    0x08, 0x60, 0x00, 0x08, 0x80, 0x00, 0x09, 0x00, 0x00, 0xaa, 0xaa, 0xaa,
    0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x08, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x3e, 0x1c, 0xc0, 0x7e, 0x3e,
    0xc0, 0x60, 0x63, 0xc0, 0x60, 0x63, 0xc0, 0x7c, 0x63, 0xe0, 0x7c, 0x63,
    0x70, 0x60, 0x63, 0x3e, 0x60, 0x3e, 0x0e, 0x60, 0x1c, 0x00, 0x00, 0x00};

// 'loudness', 24x16px
const unsigned char icon_loudness[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x11, 0xff, 0x88, 0x25,
    0xc3, 0xa4, 0x29, 0x99, 0x94, 0x29, 0xa5, 0x94, 0x29, 0xb5,
    0x94, 0x29, 0x99, 0x94, 0x25, 0xc3, 0xa4, 0x11, 0xff, 0x88,
    0x01, 0xff, 0x80, 0x01, 0xe7, 0x80, 0x01, 0xe7, 0x80, 0x01,
    0xff, 0x80, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00

};

// 'wavd', 24x19px
const unsigned char icon_wavd[] PROGMEM = {
    0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0xcc, 0x00, 0x00, 0xaa, 0x70, 0x00,
    0x99, 0x11, 0x10, 0x00, 0x12, 0xa8, 0x00, 0x12, 0xa8, 0x20, 0x12, 0xa8,
    0x51, 0x12, 0xa8, 0x8a, 0x74, 0xa9, 0x04, 0x10, 0xaa, 0x00, 0x10, 0xaa,
    0x00, 0x10, 0xaa, 0xee, 0x10, 0xaa, 0xaa, 0x10, 0x44, 0xaa, 0x70, 0x00,
    0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// 'para', 24x19px
const unsigned char icon_para[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x60, 0x00, 0x1c, 0x50,
    0x00, 0x63, 0x08, 0x00, 0x63, 0x08, 0x00, 0xff, 0x84, 0x10, 0xff, 0x84,
    0x10, 0xff, 0x80, 0x08, 0x7f, 0x00, 0x08, 0x7f, 0x00, 0x05, 0x1c, 0x00,
    0x03, 0x00, 0x00, 0x07, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x01, 0xdd, 0xc0,
    0x01, 0xdd, 0xc0, 0x01, 0xdd, 0xc0, 0x00, 0x00, 0x00};

// 'step', 24x25px
const unsigned char icon_step[] PROGMEM = {
    0x00, 0x00, 0xd7, 0x00, 0x03, 0xdd, 0x00, 0x0c, 0xf5, 0x00, 0x38,
    0x77, 0x00, 0xe0, 0x3c, 0x03, 0x80, 0x7f, 0x0f, 0x01, 0xde, 0x39,
    0x87, 0x38, 0xd6, 0xcc, 0xe1, 0xd9, 0xfb, 0x86, 0x7f, 0xfe, 0x19,
    0xa7, 0x98, 0x67, 0xde, 0x61, 0x9e, 0xe9, 0x86, 0x78, 0x76, 0x19,
    0xe0, 0x3a, 0x67, 0x80, 0x1d, 0x9e, 0x00, 0x0e, 0x78, 0x00, 0x06,
    0xe0, 0x00, 0x02, 0x80, 0x00, 0x00, 0x77, 0x77, 0x00, 0x42, 0x45,
    0x00, 0x72, 0x67, 0x00, 0x12, 0x44, 0x00, 0x72, 0x74};

// 'mixer', 24x16px
const unsigned char icon_mixer[] PROGMEM = {
    0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x0e, 0x00, 0x00, 0xee, 0x00, 0x0e,
    0xee, 0x00, 0x0e, 0xee, 0x00, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// 'gateboxlarge', 24x25px
const unsigned char icon_gatebox[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x1e, 0x80, 0x00, 0x7e,
    0xe0, 0x01, 0xfe, 0xe0, 0x07, 0xff, 0x90, 0x03, 0xfe, 0x70, 0x0c,
    0xf9, 0xf0, 0x0f, 0x26, 0xf0, 0x0f, 0xdc, 0xf0, 0xef, 0xd3, 0x37,
    0x0f, 0xdf, 0xc0, 0x0f, 0x9f, 0xf0, 0x0e, 0x5f, 0xe0, 0x09, 0xdf,
    0x80, 0x07, 0xde, 0x00, 0x01, 0xd8, 0x00, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x3d, 0xef, 0x48, 0x21, 0x29, 0x48, 0x2d, 0xe9, 0x30,
    0x25, 0x29, 0x48, 0x3d, 0xef, 0x48, 0x00, 0x00, 0x00};

// 'rythmecho', 24x25px
const unsigned char icon_rhytmecho[] PROGMEM = {
    0x00, 0x00, 0x00, 0x1f, 0xff, 0xfe, 0x3f, 0xff, 0xfe, 0x3f, 0xff,
    0xfc, 0x00, 0x01, 0xfc, 0x07, 0xe1, 0xf8, 0x07, 0xe3, 0xf8, 0x0f,
    0xc7, 0xf0, 0x0f, 0xff, 0xf0, 0x1f, 0xff, 0xe0, 0x1f, 0xff, 0x00,
    0x3f, 0x7f, 0x80, 0x3f, 0x1f, 0xc0, 0x7e, 0x0f, 0xe0, 0x7e, 0x07,
    0xf0, 0x00, 0x00, 0x00, 0x7b, 0xdb, 0x7c, 0x7b, 0xdb, 0x7c, 0x63,
    0x1b, 0x6c, 0x7b, 0x1f, 0x6c, 0x7b, 0x1f, 0x6c, 0x63, 0x1b, 0x6c,
    0x7b, 0xdb, 0x7c, 0x7b, 0xdb, 0x7c, 0x00, 0x00, 0x00};

// 'route', 24x16px
const unsigned char icon_route[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf1, 0xf8, 0x07, 0xe3, 0xf0,
    0x06, 0x03, 0x00, 0x06, 0x03, 0x00, 0x1f, 0x8f, 0xc0, 0x0f, 0x07, 0x80,
    0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x07, 0x80, 0x19, 0x8c, 0xc0,
    0x0f, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// 'sound', 24x19px
const unsigned char icon_sound[] PROGMEM = {
    0x00, 0x03, 0xe0, 0x00, 0x3f, 0xe0, 0x01, 0xff, 0xe0, 0x01, 0xfc, 0xe0,
    0x01, 0xc0, 0xe0, 0x01, 0xc0, 0xe0, 0x01, 0xc0, 0xe0, 0x01, 0xc0, 0xe0,
    0x01, 0xc0, 0xe0, 0x01, 0xc0, 0xe0, 0x01, 0xc3, 0xe0, 0x01, 0xc7, 0xe0,
    0x01, 0xc7, 0xe0, 0x07, 0xc7, 0xc0, 0x0f, 0xc3, 0x80, 0x0f, 0xc0, 0x00,
    0x0f, 0x80, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00};

// 'ram_2_icon2', 24x25px
const unsigned char icon_ram2[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xf8, 0x20, 0x00,
    0x04, 0x20, 0x00, 0x04, 0x20, 0x00, 0x04, 0x23, 0xff, 0xc4, 0x26,
    0x43, 0x24, 0x25, 0x42, 0xa4, 0x24, 0xc2, 0x64, 0x23, 0x81, 0xc4,
    0x20, 0x00, 0x04, 0x20, 0x00, 0x04, 0x20, 0xff, 0x04, 0x1f, 0xff,
    0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x6c, 0xe0, 0x0f,
    0x6d, 0xf0, 0x06, 0x6d, 0xb0, 0x06, 0x7d, 0xb0, 0x06, 0x7d, 0xb0,
    0x06, 0x7d, 0xf0, 0x06, 0x38, 0xe0, 0x00, 0x00, 0x00};
// 'ram_1_icon', 24x25px
const unsigned char icon_ram1[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xf8, 0x20, 0x00,
    0x04, 0x20, 0x00, 0x04, 0x20, 0x00, 0x04, 0x23, 0xff, 0xc4, 0x24,
    0xc2, 0x64, 0x25, 0x42, 0xa4, 0x26, 0x43, 0x24, 0x23, 0x81, 0xc4,
    0x20, 0x00, 0x04, 0x20, 0x00, 0x04, 0x20, 0xff, 0x04, 0x1f, 0xff,
    0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x38, 0x70, 0x0f,
    0xbc, 0xf0, 0x0d, 0xb6, 0xc0, 0x0d, 0xb6, 0xf0, 0x0d, 0xb6, 0xc0,
    0x0f, 0xb6, 0xf0, 0x07, 0x36, 0x70, 0x00, 0x00, 0x00};

// 'md_rev', 34x24px
const unsigned char icon_md[] PROGMEM = {
    0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0xa0, 0x00, 0x00, 0x00,
    0x06, 0xb0, 0x00, 0x00, 0x00, 0x1a, 0xe8, 0x00, 0x00, 0x00, 0x6b, 0xac,
    0x00, 0x00, 0x01, 0xee, 0xbe, 0x00, 0x00, 0x07, 0x3a, 0xe7, 0x00, 0x00,
    0x1c, 0x1b, 0x9c, 0x80, 0x00, 0x70, 0x0e, 0x70, 0x80, 0x01, 0xc0, 0x1f,
    0xc3, 0x40, 0x07, 0x80, 0x6f, 0x0c, 0xc0, 0x1c, 0xc1, 0x9c, 0x33, 0xc0,
    0x6b, 0x66, 0x70, 0xcf, 0x00, 0x6c, 0xfd, 0xc3, 0x3c, 0x00, 0xbf, 0xff,
    0x0c, 0xf0, 0x00, 0xd3, 0xcc, 0x33, 0xc0, 0x00, 0xef, 0x30, 0xcf, 0x00,
    0x00, 0x74, 0xc3, 0x3c, 0x00, 0x00, 0x3b, 0x0c, 0xf0, 0x00, 0x00, 0x1d,
    0x33, 0xc0, 0x00, 0x00, 0x0e, 0xcf, 0x00, 0x00, 0x00, 0x07, 0x3c, 0x00,
    0x00, 0x00, 0x03, 0x70, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00};

// 'analog4_rev', 34x24px
const unsigned char icon_a4[] PROGMEM = {
    0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00,
    0x06, 0xb0, 0x00, 0x00, 0x00, 0x1a, 0xd8, 0x00, 0x00, 0x00, 0x6b, 0xac,
    0x00, 0x00, 0x01, 0xae, 0xb6, 0x00, 0x00, 0x06, 0xba, 0xff, 0x00, 0x00,
    0x1e, 0xeb, 0x8c, 0x80, 0x00, 0x73, 0xae, 0x30, 0x80, 0x01, 0xc1, 0xb8,
    0xc3, 0x40, 0x07, 0x03, 0xe3, 0x0c, 0xc0, 0x1a, 0x0d, 0xec, 0x33, 0xc0,
    0x6b, 0x37, 0xf0, 0xcf, 0x00, 0x6f, 0xdc, 0xc3, 0x3c, 0x00, 0xbc, 0xf3,
    0x0c, 0xf0, 0x00, 0xd3, 0xcc, 0x33, 0xc0, 0x00, 0xef, 0xf0, 0xcf, 0x00,
    0x00, 0x76, 0xc3, 0x3c, 0x00, 0x00, 0x3b, 0x0c, 0xf0, 0x00, 0x00, 0x1d,
    0x33, 0xc0, 0x00, 0x00, 0x0e, 0xcf, 0x00, 0x00, 0x00, 0x07, 0x3c, 0x00,
    0x00, 0x00, 0x03, 0x70, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00};

MCLGUI mcl_gui;
