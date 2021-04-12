#include "MCL_impl.h"

void MCLGUI::draw_textbox(const char *text, const char *text2) {
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

void MCLGUI::delay_progress(uint16_t clock_) {
  uint16_t myclock = slowclock;
  while (clock_diff(myclock, slowclock) < clock_) {
    mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
  }
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
  char K[4] = "--";

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
                        const uint64_t &pattern_mask, uint8_t step_count,
                        uint8_t length, const uint64_t &mute_mask,
                        const uint64_t &slide_mask) {
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

      oled_display.drawRect(x, y, seq_w, trig_h, WHITE);
      if (((i + offset != step_count) || (MidiClock.state != 2))) {

        if (IS_BIT_SET64(slide_mask, i + offset)) {
          oled_display.drawPixel(x + 2, y + 2, WHITE);
        } else if (IS_BIT_SET64(mute_mask, i + offset)) {
          oled_display.drawPixel(x + 2, y + 1, WHITE);
          oled_display.drawPixel(x + 1, y + 2, WHITE);
          oled_display.drawPixel(x + 3, y + 2, WHITE);
          oled_display.drawPixel(x + 2, y + 3, WHITE);
        } else if (IS_BIT_SET64(pattern_mask, i + offset)) {
          oled_display.fillRect(x + 1, y + 1, seq_w - 1, trig_h - 1, WHITE);
        }
      }
    }

    x += seq_w + 1;
  }
#endif
}

void MCLGUI::draw_track_type_select(uint8_t x, uint8_t y,
                                    uint8_t track_type_select) {
  char dev[6];
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  for (uint8_t i = 0; i < 4; i++) {
    switch (i) {
    case 0:
      strcpy(dev, devs[0]->name);
      break;
    case 1:
      if (devs[1] != nullptr) {
        strcpy(dev, devs[1]->name);
      } else {
        strcpy(dev, "MI");
      }
      break;
    case 2:
      strcpy(dev, "FX");
      break;
    case 3:
      strcpy(dev, "TEMPO");
      break;
    }

    oled_display.setCursor(x, y);
    oled_display.print(dev);
    if (IS_BIT_SET(track_type_select, i)) {
      oled_display.fillRect(x, y + 4, seq_w, trig_h, WHITE);
    } else {
      oled_display.drawRect(x, y + 4, seq_w, trig_h, WHITE);
    }
    x += 16;
  }
}

void MCLGUI::draw_leds(uint8_t x, uint8_t y, uint8_t offset,
                       const uint64_t &lock_mask, uint8_t step_count,
                       uint8_t length, bool show_current_step) {
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
    oled_display.fillTriangle_3px(pane_tri_x + 1, pane_tri_y, WHITE);
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

MCLGUI mcl_gui;
