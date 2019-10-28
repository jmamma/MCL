#include "MCL.h"
#define SHOW_VALUE_TIMEOUT 2000

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
  for (uint8_t y = from; y < to; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
}

void MCLGUI::draw_horizontal_dashline(uint8_t y, uint8_t from, uint8_t to) {
  for (uint8_t x = from; x < to; x += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
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
  uint8_t length = round(((float)(n_window - 1) / (float)(n_items - 1)) * 32);
  uint8_t y = round(((float)(n_current) / (float)(n_items - 1)) * 32);
  mcl_gui.draw_vertical_separator(x + 1);
  oled_display.fillRect(x + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x, y, 5, length, WHITE);
}

static char title_buf[16];

//  ref: Design/popup_menu.png
void MCLGUI::draw_popup(const char *title, bool deferred_display) {

  strcpy(title_buf, title);
  m_toupper(title_buf);

  oled_display.setFont(&TomThumb);

  // draw menu body
  oled_display.fillRect(s_menu_x - 1, s_menu_y - 1, s_menu_w + 2, s_menu_h + 2,
                        BLACK);
  oled_display.drawRect(s_menu_x, s_menu_y, s_menu_w, s_menu_h, WHITE);
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 1, s_menu_w - 2, 4, WHITE);

  // draw the title '____/**********\____' part
  oled_display.drawRect(s_title_x, s_menu_y - 3, s_title_w, 3, BLACK);
  oled_display.drawRect(s_title_x, s_menu_y - 2, s_title_w, 2, WHITE);
  oled_display.drawPixel(s_title_x, s_menu_y - 2, BLACK);
  oled_display.drawPixel(s_title_x + s_title_w - 1, s_menu_y - 2, BLACK);

  oled_display.setTextColor(BLACK);
  // auto len = strlen(title_buf) * 5;
  // oled_display.setCursor(s_title_x + (s_title_w - len) / 2 , s_menu_y + 3);
  oled_display.setCursor(s_title_x + 2, s_menu_y + 4);
  oled_display.println(title_buf);
  oled_display.setTextColor(WHITE);
  if (!deferred_display) {
    oled_display.display();
  }
}

void MCLGUI::clear_popup() {
  // XXX too slow
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 4, s_menu_w - 2, s_menu_h - 5,
                        BLACK);
}

static constexpr uint8_t s_progress_x = 31;
static constexpr uint8_t s_progress_y = 16;
static constexpr uint8_t s_progress_w = 64;
static constexpr uint8_t s_progress_h = 5;

static uint8_t s_progress_cookie = 0;

void MCLGUI::draw_progress(const char *msg, uint8_t cur, uint8_t _max,
                           bool deferred_display) {
  draw_popup(msg, true);

  oled_display.fillRect(s_progress_x + 1, s_progress_y + 1, s_progress_w - 2,
                        s_progress_h - 2, BLACK);

  float prog = (float)cur / (float)_max;
  auto progx = (uint8_t)(s_progress_x + 1 + prog * (s_progress_w - 2));
  // draw the progress
  oled_display.fillRect(s_progress_x + 1, s_progress_y + 1,
                        progx - s_progress_x + 1, s_progress_h - 2, WHITE);
  s_progress_cookie = (s_progress_cookie + 1) % 3;

  // draw the '///////' pattern, note the cookie
  for (uint8_t i = s_progress_cookie + s_progress_x + 1; i < progx; i += 3) {
    oled_display.drawLine(i, s_progress_y + s_progress_h - 2, i + 2,
                          s_progress_y + 1, BLACK);
  }

  oled_display.drawRect(s_progress_x, s_progress_y, s_progress_w, s_progress_h,
                        WHITE);
  if (!deferred_display) {
    oled_display.display();
  }
}

//  ref: Design/infobox.png
void MCLGUI::draw_infobox(const char *line1, const char *line2,
                          const int line2_offset) {
  auto oldfont = oled_display.getFont();

  oled_display.fillRect(info_x1 - 1, info_y1 - 1, info_w + 3, info_h + 3,
                        BLACK);
  oled_display.drawRect(info_x1, info_y1, info_w, info_h, WHITE);
  oled_display.drawFastHLine(info_x1 + 1, info_y2 + 1, info_w, WHITE);
  oled_display.drawFastVLine(info_x2 + 1, info_y1 + 1, info_h - 1, WHITE);
  oled_display.fillRect(info_x1 + 1, info_y1 + 1, info_w - 2, 6, WHITE);

  oled_display.fillCircle(circle_x, circle_y, 6, WHITE);
  oled_display.fillRect(circle_x - 1, circle_y - 3, 2, 4, BLACK);
  oled_display.fillRect(circle_x - 1, circle_y + 2, 2, 2, BLACK);

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(info_x1 + 4, info_y1 + 6);
  strcpy(title_buf, line1);
  m_toupper(title_buf);
  oled_display.println(title_buf);

  oled_display.setTextColor(WHITE);
  oled_display.setCursor(info_x1 + 23, info_y1 + 17 + line2_offset);
  oled_display.println(line2);

  oled_display.setFont(oldfont);
}

void MCLGUI::draw_encoder(uint8_t x, uint8_t y, uint8_t value) {

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

void MCLGUI::draw_light_encoder(uint8_t x, uint8_t y, Encoder *encoder,
                                const char *name) {
  bool show_value = show_encoder_value(encoder);
  draw_light_encoder(x, y, encoder->cur, name, show_value);
}

void MCLGUI::draw_light_encoder(uint8_t x, uint8_t y, uint8_t value,
                                const char *name, bool show_value) {
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
}

void MCLGUI::draw_keyboard(uint8_t x, uint8_t y, uint8_t note_width,
                           uint8_t note_height, uint8_t num_of_notes,
                           uint64_t note_mask) {
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
}
