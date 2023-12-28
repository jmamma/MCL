#include "MCL_impl.h"
#include "ResourceManager.h"

void MCLGUI::put_value_at2(uint8_t value, char *str) {
   str[0] = (value % 100) / 10 + '0';
   str[1] = (value % 10) + '0';
}

void MCLGUI::put_value_at(uint8_t value, char *str) {
  if (value < 10) {
    str[0] = value + '0';
    str[1] = '\0';
  } else if (value < 100) {
    str[0] = value / 10 + '0';
    str[1] = value % 10 + '0';
    str[2] = '\0';
  } else if (value < 1000) {
    str[0] = value / 100 + '0';
    str[1] = (value / 10) % 10 + '0';
    str[2] = value % 10 + '0';
    str[3] = '\0';
  }
}

void MCLGUI::draw_textbox(const char *text, const char *text2) {
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
}

bool MCLGUI::wait_for_input(char *dst, const char *title, uint8_t len) {
  text_input_page.init();
  text_input_page.init_text(dst, title, len);
  mcl.pushPage(TEXT_INPUT_PAGE);
  while (mcl.currentPage() == TEXT_INPUT_PAGE) {
    GUI.loop();
  }
  m_trim_space(dst);
  return text_input_page.return_state;
}

bool MCLGUI::wait_for_confirm(const char *title, const char *text) {
  questiondialog_page.init(title, text);
  mcl.pushPage(QUESTIONDIALOG_PAGE);
  while (mcl.currentPage() == QUESTIONDIALOG_PAGE) {
    GUI.loop();
  }
  return questiondialog_page.return_state;
}

void MCLGUI::wait_for_project() {
  again:
  mcl.setPage(START_MENU_PAGE);
  while (mcl.currentPage() == START_MENU_PAGE || mcl.currentPage() == TEXT_INPUT_PAGE || mcl.currentPage() == LOAD_PROJ_PAGE) {
    GUI.loop();
  }
  if (!proj.project_loaded) { goto again; }
  DEBUG_PRINTLN("finished");
}


void MCLGUI::draw_vertical_dashline(uint8_t x, uint8_t from, uint8_t to) {
  for (uint8_t y = from; y < to; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
}

void MCLGUI::draw_horizontal_dashline(uint8_t y, uint8_t from, uint8_t to) {
  for (uint8_t x = from; x < to; x += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
}

void MCLGUI::draw_horizontal_arrow(uint8_t x, uint8_t y, uint8_t w) {
  oled_display.drawFastHLine(x, y, w, WHITE);
  oled_display.drawFastVLine(x + w - 2, y - 1, 3, WHITE);
  oled_display.drawFastVLine(x + w - 3, y - 2, 5, WHITE);
}

void MCLGUI::draw_vertical_separator(uint8_t x) {
  auto x_ = x + 2;
  for (uint8_t y = 0; y < 32; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
    oled_display.drawPixel(x_, y, WHITE);
  }
  x_ = x + 1;
  for (uint8_t y = 1; y < 32; y += 2) {
    oled_display.drawPixel(x_, y, WHITE);
  }
}

void MCLGUI::draw_vertical_scrollbar(uint8_t x, uint8_t n_items,
                                     uint8_t n_window, uint8_t n_current) {
  //uint8_t length = round(((float)(n_window - 1) / (float)(n_items - 1)) * 32);
  //uint8_t y = round(((float)(n_current) / (float)(n_items - 1)) * 32);

  uint8_t length = 1 + ((uint16_t) (n_window - 1) * 32) / (n_items - 1);
  uint8_t y = (((uint16_t) n_current * 32) / (n_items - 1));
  if (y + length > 32) { length--; }

  mcl_gui.draw_vertical_separator(x + 1);
  oled_display.fillRect(x + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x, y, 5, length, WHITE);
}

void MCLGUI::draw_knob_frame() {
  for (uint8_t x = knob_x0; x <= knob_xend; x += knob_w) {
    mcl_gui.draw_vertical_dashline(x, 0, knob_y);
    oled_display.drawPixel(x, knob_y, WHITE);
    oled_display.drawPixel(x, knob_y + 1, WHITE);
  }
  mcl_gui.draw_horizontal_dashline(knob_y, knob_x0 + 1, knob_xend + 1);
}

void MCLGUI::draw_knob(uint8_t i, const char *title, const char *text) {
  uint8_t x = knob_x0 + i * knob_w;
  draw_text_encoder(x, knob_y0, title, text);
}

void MCLGUI::draw_knob(uint8_t i, Encoder *enc, const char *title, bool highlight) {
  uint8_t x = knob_x0 + i * knob_w;
  draw_light_encoder(x + 7, 6, enc, title,highlight);
}

static char title_buf[16];

//  ref: Design/popup_menu.png
void MCLGUI::draw_popup(const char *title, bool deferred_display, uint8_t h) {
  strcpy(title_buf, title);
  m_toupper(title_buf);

  if (h == 0) {
    h = s_menu_h;
  }
  oled_display.setFont(&TomThumb);

  // draw menu body
  oled_display.fillRect(s_menu_x - 1, s_menu_y + 1, s_menu_w + 2, h + 4, BLACK);
  oled_display.drawRect(s_menu_x, s_menu_y + 2, s_menu_w, h + 2, WHITE);
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 3, s_menu_w - 2, 4, WHITE);
  oled_display.drawPixel(s_menu_x, s_menu_y + 2, BLACK);
  oled_display.drawPixel(s_menu_x + s_menu_w - 1, s_menu_y + 2, BLACK);

  // draw the title '____/**********\____' part
  oled_display.drawRect(s_title_x, s_menu_y, s_title_w, 2, BLACK);
  oled_display.fillRect(s_title_x, s_menu_y + 0, s_title_w, 2, WHITE);
  oled_display.drawPixel(s_title_x, s_menu_y + 0, BLACK);
  oled_display.drawPixel(s_title_x + s_title_w - 1, s_menu_y, BLACK);

  oled_display.setTextColor(BLACK);

  auto len = strlen(title_buf);
  uint8_t whitespace = 0;
  for (uint8_t n = 0; n < len; n++) {
    if (title_buf[n] == ' ') { whitespace++; }
  }
  len -= whitespace;
  oled_display.setCursor(s_title_x + (s_title_w - len * 4) / 2, s_menu_y + 6);
  // oled_display.setCursor(s_title_x + 2, s_menu_y + 3);
  oled_display.println(title_buf);
  oled_display.setTextColor(WHITE);
  if (!deferred_display) {
    oled_display.display();
  }
}

void MCLGUI::clear_popup(uint8_t h) {
  if (h == 0) {
    h = s_menu_h;
  }
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 4, s_menu_w - 2, h - 5, BLACK);
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
                               uint8_t x_pos, uint8_t y_pos, uint8_t width, uint8_t height, bool border) {

  oled_display.fillRect(x_pos + 1, y_pos + 1, width - 2,
                        height - 2, BLACK);

  float prog = (float)cur / (float)_max;
  auto progx = (uint8_t)(x_pos + 1 + prog * (width - 2));
  // draw the progress
  oled_display.fillRect(x_pos + 1, y_pos + 1, progx - x_pos - 1,
                        height - 2, WHITE);

  uint8_t shift = 1;

  // draw the '///////' pattern, using circular shifting
  uint8_t x = 0;

  uint8_t bitmask = s_progress_cookie;
  uint8_t temp_bitmask = s_progress_cookie;

  for (uint8_t i = x_pos + 1; i <= progx; i += 1) {

    for (uint8_t n = 0; n < height - 2; n++) {
      uint8_t a = height - 2 - n;

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
  if (border) oled_display.drawRect(x_pos, y_pos, width, height, WHITE);

  if (!deferred_display) {
    oled_display.display();
  }
}

//  ref: Design/infobox.png
void MCLGUI::draw_infobox(const char *line1, const char *line2,
                          const int line2_offset) {
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

  oled_display.fillRect(dlg_circle_x - 4, dlg_circle_y - 4, 9, 9, WHITE);

  oled_display.fillRect(dlg_circle_x - 1, dlg_circle_y - 3, 3, 4, BLACK);
  oled_display.fillRect(dlg_circle_x - 1, dlg_circle_y + 2, 3, 2, BLACK);

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(dlg_info_x1 + 4, dlg_info_y1 + 6);
  strcpy(title_buf, line1);
  m_toupper(title_buf);
  oled_display.println(title_buf);

  oled_display.setTextColor(WHITE);
  oled_display.setCursor(dlg_info_x1 + 23, dlg_info_y1 + 15 + line2_offset);
  oled_display.println(line2);

  oled_display.setFont(oldfont);
}

void MCLGUI::draw_encoder(uint8_t x, uint8_t y, uint8_t value, bool highlight) {
  bool vert_flip = false;
  bool horiz_flip = false;
  uint8_t image_w = 11;
  uint8_t image_h = 11;

  // Scale encoder values to 123. encoder animation does not start and stop on
  // 0.
  value = (((uint16_t)value * (uint16_t) 122) / 128);

  //value = (uint8_t)((float)value * .95);
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

  uint8_t *icon = R.icons_knob->encoder_small_0;
  if (value < 4) {
  } else if (value < 9) {
    icon =  R.icons_knob->encoder_small_1;
  } else if (value < 14) {
    icon = R.icons_knob->encoder_small_2;
  } else if (value < 19) {
    icon = R.icons_knob->encoder_small_3;
  } else if (value < 24) {
    icon = R.icons_knob->encoder_small_4;
  } else if (value < 30) {
    icon = R.icons_knob->encoder_small_5;
  } else {
    icon = R.icons_knob->encoder_small_6;
  }

  oled_display.drawBitmap(x, y, icon, image_w, image_h, WHITE,
                            vert_flip, horiz_flip);

  if (highlight) { oled_display.fillRect(x, y,11,11,INVERT); }
}

void MCLGUI::draw_encoder(uint8_t x, uint8_t y, Encoder *encoder) {
  draw_encoder(x, y, encoder->cur);
}

bool MCLGUI::show_encoder_value(Encoder *encoder, int timeout) {
  uint8_t match = 255;

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS && match == 255; i++) {
    if (((LightPage *)GUI.currentPage())->encoders[i] == encoder) {
      match = i;
    }
  }

  if (match != 255) {
    if (clock_diff(((LightPage *)GUI.currentPage())->encoders_used_clock[match],
                   slowclock) < timeout || BUTTON_DOWN(Buttons.ENCODER1 + match)) {
      return true;
    } else {
      ((LightPage *)GUI.currentPage())->encoders_used_clock[match] =
          slowclock + timeout + 1;
    }
  }

  return false;
}

void MCLGUI::draw_text_encoder(uint8_t x, uint8_t y, const char *name,
                               const char *value, bool highlight) {
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(x + 4, y + 6);
  oled_display.print(name);

  oled_display.setFont();
  oled_display.setCursor(x + 4, y + 8);
  oled_display.print(value);

  if (highlight) { oled_display.fillRect(x,y,23,16, INVERT); }
}

void MCLGUI::draw_md_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                             const char *name) {
  bool show_value = show_encoder_value(encoder);
  draw_md_encoder(x, y, encoder->cur, name, show_value);
}

void MCLGUI::draw_md_encoder(uint8_t x, uint8_t y, uint8_t value,
                             const char *name, bool show_value) {
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
}

void MCLGUI::draw_light_encoder(uint8_t x, uint8_t y, Encoder *encoder, const char *name, bool highlight) {
  bool show_value = show_encoder_value(encoder);
  draw_light_encoder(x, y, encoder->cur, name, highlight, show_value);
}

void MCLGUI::draw_light_encoder(uint8_t x, uint8_t y, uint8_t value,
                                const char *name, bool highlight, bool show_value) {
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
  if (highlight) { oled_display.fillRect(x - 2, 0, 16,20,INVERT); }

  oled_display.setFont(oldfont);
}

void MCLGUI::draw_microtiming(uint8_t speed, uint8_t timing) {
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  oled_display.setTextColor(WHITE);
  SeqTrack seq_track;
  uint8_t timing_mid = seq_track.get_timing_mid(speed);
  uint8_t degrees = timing_mid * 2;
  uint8_t heights[16];
  // Triplets

  uint8_t heights_lowres[6] = {11, 4, 6, 10, 4, 8};
  uint8_t heights_triplets[16] = {11, 2, 4, 8, 6,  10, 6, 2,
                                    10, 2, 6, 8, 10, 4,  6, 2};
  uint8_t heights_triplets2[8] = {11, 4, 8, 10, 2, 8, 4, 2};
  uint8_t heights_highres[12] = {11, 2, 4, 8, 6, 2, 10, 2, 6, 8, 4, 2};

  uint8_t *h = heights_highres;
  uint8_t heights_len = 12;

  if (speed == SEQ_SPEED_2X) {
    h = heights_lowres;
    heights_len = 6;
  } else if (speed == SEQ_SPEED_3_4X) {
    h = heights_triplets;
    heights_len = 16;
  } else if (speed == SEQ_SPEED_3_2X) {
    h = heights_triplets2;
    heights_len = 8;
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
    put_value_at(timing_mid - timing, K + 1);
  } else {
    K[0] = '+';
    put_value_at(timing - timing_mid, K + 1);
  }

  oled_display.fillRect(8, 1, 128 - 16, 32 - 2, BLACK);
  oled_display.drawRect(8 + 1, 1 + 1, 128 - 16 - 2, 32 - 2 - 2, WHITE);

  oled_display.setCursor(x_pos + 34, 10);
  oled_display.print(F("uTIMING: "));
  oled_display.print(K);
  oled_display.drawLine(x, y_pos + h[0], x + w, y_pos + h[0],
                        WHITE);
  for (uint8_t n = 0; n <= degrees; n++) {
    oled_display.drawLine(x, y_pos + h[0], x,
                          y_pos + h[0] - h[a], WHITE);
    a++;

    if (n == timing) {
      oled_display.fillRect(x - 1, y_pos + h[0] + 3, 3, 3, WHITE);
      oled_display.drawPixel(x, y_pos + h[0] + 2, WHITE);
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
}

void MCLGUI::draw_keyboard(uint8_t x, uint8_t y, uint8_t note_width,
                           uint8_t note_height, uint8_t num_of_notes,
                           uint64_t *note_mask) {
  const uint16_t chromatic = 0b0000010101001010;
  const uint8_t half = note_height / 2;
  const uint8_t y2 = y + note_height - 1;
  const uint8_t wm1 = note_width - 1;

  uint8_t note_type = 0;

  bool last_black = false;

  // draw first '|'
  oled_display.drawFastVLine(x, y, note_height, WHITE);

  uint8_t first_note = 0;
  for (uint8_t n = 0; n < 128; n++) {
    if (IS_BIT_SET128_P(note_mask, n)) {
      first_note = n;
      break;
    }
  }

  uint8_t offset = (first_note / 24) * 24;

  offset = min(127 - num_of_notes, offset);

  for (uint8_t n = 0; n < num_of_notes; n++) {

    bool pressed = IS_BIT_SET128_P(note_mask, n + offset);
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
}
void MCLGUI::draw_trigs(uint8_t x, uint8_t y, const uint16_t &trig_selection) {

  for (uint8_t i = 0; i < 16; i++) {
    if (IS_BIT_SET16(trig_selection,i)) {
      oled_display.fillRect(x, y, seq_w, trig_h, WHITE);
    }
    else{
      oled_display.drawRect(x, y, seq_w, trig_h, WHITE);
    }
    x += seq_w + 1;
  }

}

void MCLGUI::draw_trigs(uint8_t x, uint8_t y, uint8_t offset,
                        const uint64_t &pattern_mask, uint8_t step_count,
                        uint8_t length, const uint64_t &mute_mask,
                        const uint64_t &slide_mask) {
  for (uint8_t i = 0; i < 16; i++) {

    uint8_t idx = i + offset;
    bool in_range = idx < length;

    if (note_interface.is_note_on(i)) {
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
}

void MCLGUI::draw_track_type_select(uint8_t track_type_select) {
  char dev[6];
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
//  oled_display.clearDisplay();

  uint8_t x = 0;
  //oled_display.fillRect(0, 0, 128, 7, WHITE);
  oled_display.fillRect(s_title_x + 10, 0, 50, 7, WHITE);
  oled_display.setCursor(s_title_x + (s_title_w - 12 * 4) / 2 + 2, 6);
  // oled_display.setCursor(s_title_x + 2, s_menu_y + 3);
  oled_display.setTextColor(BLACK);
  oled_display.println("GROUP SELECT");

  oled_display.fillRect(0, 8, 128, 23, BLACK);
  MCLGIF *gif;

  for (uint8_t i = 0; i < 5; i++) {

    bool select = IS_BIT_SET(track_type_select, i);

    uint8_t *icon = nullptr;
    uint8_t offset = 3;
    int8_t y_offset = 0;
    switch (i) {
    case 0:
      icon = devs[0]->gif_data();
      gif = devs[0]->gif();
      gif->set_bmp(icon);
      break;
    case 1:
      icon = devs[1]->gif_data();
      gif = devs[1]->gif();
      gif->set_bmp(icon);
      offset = 4;
      break;
    case 2:
      gif = R.icons_logo->perf_gif;
      gif->set_bmp(R.icons_logo->perf_gif_data);
      offset = 3;
      break;
    case 3:
      gif = R.icons_logo->route_gif;
      gif->set_bmp(R.icons_logo->route_gif_data);
      offset = 5;
      break;
    case 4:
      gif = R.icons_logo->metronome_gif;
      gif->set_bmp(R.icons_logo->metronome_gif_data);
      offset = 4;
      y_offset = -3;
      break;
    }

    //icon = select ? gif->get_frame(0) : gif->get_next_frame();
    icon = gif->get_next_frame();

    if (icon) { oled_display.drawBitmap(x + offset, 15 + y_offset, icon, gif->w, gif->h, WHITE); }

    if (note_interface.is_note_on(i)) { gif->reset(); }

    if (select) {
   //   gif->reset();
      oled_display.fillRect(x, 9, 24, 21, INVERT);

       oled_display.drawRect(x + 1, 10, 22, 19, BLACK);
    } else {
       oled_display.drawRect(x, 9, 24, 21, WHITE);
    }
      oled_display.drawPixel(x,9,!select);
      oled_display.drawPixel(x + 23,9,!select);
      oled_display.drawPixel(x,9 + 20,!select);
      oled_display.drawPixel(x + 23,9 + 20,!select);

    x += 26;
  }
}

void MCLGUI::draw_leds(uint8_t x, uint8_t y, uint8_t offset,
                       const uint64_t &lock_mask, uint8_t step_count,
                       uint8_t length, bool show_current_step) {
  for (uint8_t i = 0; i < 16; i++) {

    uint8_t idx = i + offset;
    bool in_range = idx < length;
    bool current =
        show_current_step && step_count == idx && MidiClock.state == 2;
    bool locked = in_range && IS_BIT_SET64(lock_mask, i + offset);

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
}

void MCLGUI::draw_panel_toggle(const char *s1, const char *s2, bool s1_active) {
  oled_display.setFont(&TomThumb);

  oled_display.setCursor(pane_label_x + 1, pane_label_md_y + 6);
  if (s1_active) {
    oled_display.fillRect(pane_label_x, pane_label_md_y, pane_label_w,
                          pane_label_h, WHITE);
    oled_display.setTextColor(BLACK);
    oled_display.print(s1);
    oled_display.setTextColor(WHITE);
  } else {
    oled_display.setTextColor(WHITE);
    oled_display.print(s1);
    oled_display.fillRect(pane_label_x, pane_label_ex_y, pane_label_w,
                          pane_label_h, WHITE);
    oled_display.setTextColor(BLACK);
  }

  if (mcl.currentPage() == SEQ_PTC_PAGE || mcl.currentPage() == LFO_PAGE) {
    oled_display.setCursor(pane_label_x + 1, pane_label_ex_y + 6);
    oled_display.print(s2);
    oled_display.setTextColor(WHITE);
  }
}

void MCLGUI::draw_panel_labels(const char *info1, const char *info2) {
  oled_display.setFont(&TomThumb);
  oled_display.fillRect(0, pane_info1_y, pane_w, pane_info_h, WHITE);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(1, pane_info1_y + 6);
  oled_display.print(info1);
  oled_display.fillRect(0, pane_info2_y, pane_w, pane_info_h, BLACK);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(1, pane_info2_y + 6);
  oled_display.print(info2);
}

void MCLGUI::draw_panel_status(bool recording, bool playing) {
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
}

void MCLGUI::clear_leftpane() {
  oled_display.fillRect(0, 0, pane_w, 32, BLACK);
}

void MCLGUI::clear_rightpane() {
  oled_display.fillRect(pane_w, 0, 128 - pane_w, 32, BLACK);
}

void MCLGUI::draw_panel_number(uint8_t number) {
  oled_display.setTextColor(WHITE);
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(pane_trackid_x, pane_trackid_y);
  if (number < 10) {
    oled_display.print('0');
  }
  oled_display.print(number);
}

MCLGUI mcl_gui;
