#include "TextInputPage.h"
#include "MCLGUI.h"

constexpr auto sz_allowedchar = 69;

// idx -> chr
inline char _getchar(uint8_t i) {
  if (i >= sz_allowedchar)
    i = sz_allowedchar - 1;
  return i
      ["abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_&@-=!"];
}

// chr -> idx
uint8_t _findchar(char chr) {
  // Check to see that the character chosen is in the list of allowed
  // characters
  for (auto x = 0; x < sz_allowedchar; ++x) {
    if (chr == _getchar(x)) {
      return x;
    }
  }
  // Ensure the encoder does not go out of bounds, by resetting it to a
  // character within the allowed characters list
  return sz_allowedchar - 1;
}

void TextInputPage::setup() {}

void TextInputPage::init() {
  oled_display.setTextColor(WHITE, BLACK);
  mcl_gui.draw_popup(title,false,24);
}

void TextInputPage::init_text(char *text_, const char *title_, uint8_t len) {
  textp = text_;
  title = title_;
  length = len;
  max_length = len;
  memset(text,0,sizeof(text));
  strncpy(text, text_, len);
  //Replace null characeters with space, it will be added back in upon exit.
  bool after = false;
  for (uint8_t n = 0; n < len; n++) {
    if (text[n] == '\0' || after) { text[n] = ' '; after = true; }
  }
  text[sizeof(text) - 1] = '\0';
  cursor_position = 0;
  config_normal();
}

void TextInputPage::update_char() {
  auto chr = text[cursor_position];
  auto match = _findchar(chr);
  encoders[1]->old = encoders[1]->cur = match;
  encoders[0]->old = encoders[0]->cur;
}

// normal:
// E0 -> cursor
// E1 -> choose char
void TextInputPage::config_normal() {
  ((MCLEncoder *)encoders[0])->max = length - 1;
  ((MCLEncoder *)encoders[1])->max = sz_allowedchar - 1;
  normal_mode = true;
  // restore E0
  encoders[0]->cur = cursor_position;
  // restore E1
  update_char();
#ifdef OLED_DISPLAY
  // redraw popup body
#endif
  // update clock
  last_clock = g_clock_ms;
}

// charpane layout:
// TomThumb: 5x6
// Boundary: (1, 1, 126, 30)
// Dimension: (126, 30)
// draw each char in a 7x7 cell (padded) to fill the boundary.

constexpr auto charpane_w = 18;
constexpr auto charpane_h = 4;
constexpr auto charpane_padx = 1;
constexpr auto charpane_pady = 6;

// input: col and row
// output: coordinates on screen
static void calc_charpane_coord(uint8_t &x, uint8_t &y) {
  x = 2 + (x * 7);
  y = 2 + (y * 7);
}

void TextInputPage::loop() {
  if (normal_mode) {
  if (encoders[0]->hasChanged()) {
    update_char();
    last_clock = g_clock_ms;
  }

  if (encoders[1]->hasChanged()) {
    last_clock = g_clock_ms;
    encoders[1]->old = encoders[1]->cur;
    text[cursor_position] = _getchar(encoders[1]->getValue());
  }

  }
  else {
    if (encoders[1]->cur == ((MCLEncoder *)encoders[1])->max) {
      ((MCLEncoder *)encoders[0])->max = charpane_w - 4;
    }
    else {
      ((MCLEncoder *)encoders[0])->max = charpane_w - 1;
    }
  }
}

// charpane:
// E0 -> x axis [0..17]
// E1 -> y axis [0..3]
void TextInputPage::config_charpane() {
  // char pane not supported on 1602 displays

  ((MCLEncoder *)encoders[0])->max = charpane_w - 1;
  ((MCLEncoder *)encoders[1])->max = charpane_h - 1;
  normal_mode = false;
  auto chridx = _findchar(text[cursor_position]);
  // restore E0
  encoders[0]->cur = chridx % charpane_w;
  encoders[0]->old = encoders[0]->cur;
  // restore E1
  encoders[1]->cur = chridx / charpane_w;
  encoders[1]->old = encoders[1]->cur;

}

void TextInputPage::display_normal() {
  constexpr auto s_text_x = MCLGUI::s_menu_x + 8;
  constexpr auto s_text_y = MCLGUI::s_menu_y + 12;

  // update cursor position
  cursor_position = encoders[0]->getValue();

  // Check to see that the character chosen is in the list of allowed
  // characters
  auto time = clock_diff(last_clock, g_clock_ms);

  // mcl_gui.clear_popup(); <-- E_TOOSLOW
  oled_display.fillRect(s_text_x, s_text_y, 6 * length, 8, BLACK);
  oled_display.setCursor(s_text_x, s_text_y);
  oled_display.println(text);
  if (time < FLASH_SPEED) {
    // the default font is 6x8
    auto tx = s_text_x + 6 * cursor_position;
    oled_display.fillRect(tx, s_text_y, 6, 8, WHITE);
    oled_display.setCursor(s_text_x + 6 * cursor_position, s_text_y);
    oled_display.setTextColor(BLACK);
    oled_display.print(text[cursor_position]);
    oled_display.setTextColor(WHITE);
  }
  if (time > FLASH_SPEED * 2) {
    last_clock = g_clock_ms;
  }
}

void TextInputPage::display_charpane() {
  oled_display.setFont(&TomThumb);
  oled_display.clearDisplay();
  oled_display.drawRect(0, 0, 128, 32, WHITE);
  uint8_t chidx = 0;
  for (uint8_t y = 0; y < charpane_h; ++y) {
    for (uint8_t x = 0; x < charpane_w; ++x) {
      auto sx = x, sy = y;
      calc_charpane_coord(sx, sy);
      oled_display.setCursor(sx + charpane_padx, sy + charpane_pady);
      oled_display.print(_getchar(chidx));
      ++chidx;
    }
  }

    // draw new highlight
    uint8_t sx = encoders[0]->cur;
    uint8_t sy = encoders[1]->cur;
    calc_charpane_coord(sx, sy);
    oled_display.fillRect(sx, sy, 5, 7, INVERT);
    // update text. in charpane mode, cursor_position remains constant
    uint8_t chridx = encoders[0]->cur + encoders[1]->cur * charpane_w;
    text[cursor_position] = _getchar(chridx);

  last_clock = g_clock_ms;
}

void TextInputPage::display() {
  if (normal_mode)
    display_normal();
  else
    display_charpane();

}

bool TextInputPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return true;
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t inc = 1;
      switch (key) {
      case MDX_KEY_YES:
        //  trig_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES;
      case MDX_KEY_NO:
        //  trig_interface.ignoreNextEvent(MDX_KEY_NO);
        goto NO;
      case MDX_KEY_UP:
        encoders[1]->cur += normal_mode ? inc : -1 * inc;
        break;
      case MDX_KEY_DOWN:
        encoders[1]->cur += normal_mode ? -1 * inc : inc;
        break;
      case MDX_KEY_LEFT:
        encoders[0]->cur -= inc;
        break;
      case MDX_KEY_RIGHT:
        encoders[0]->cur += inc;
        break;
      case MDX_KEY_FUNC:
      case MDX_KEY_BANKGROUP:
        goto shift;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_FUNC:
      case MDX_KEY_BANKGROUP:
        goto shift_release;
      }
    }
  }

#ifdef OLED_DISPLAY
  // in char-pane mode, do not handle any events
  // except shift-release event.
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {

    if (!normal_mode) {
    shift_release:
      oled_display.clearDisplay();
      // before exiting charpane, advance current cursor to the next.
      ++cursor_position;
      if (cursor_position >= length) {
        cursor_position = length - 1;
      }
      // then, config normal input line
      init();
      config_normal();
      return true;
    }
    return false;
  }
#endif
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
  NO:
    DEBUG_PRINTLN("pop a");
    return_state = false;
    GUI.ignoreNextEvent(event->source);
    mcl.popPage();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
  shift:
    config_charpane();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    if (cursor_position == length - 1 && !isspace(text[cursor_position])) {
      // delete last
      text[cursor_position] = ' ';
    } else {
      if (cursor_position == 0) {
        // move the cursor, and delete first
        cursor_position = 1;
      }
      // backspace
      for (uint8_t i = cursor_position - 1; i < length - 1; ++i) {
        text[i] = text[i + 1];
      }
      text[length - 1] = ' ';
      --cursor_position;
    }
    config_normal();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
  YES:
    return_state = true;
    uint8_t cpy_len = length;
    for (uint8_t n = length - 1; n > 0 && text[n] == ' '; n--) {
      cpy_len -= 1;
    }
    strncpy(textp, text, cpy_len);
    textp[cpy_len] = '\0';
    GUI.ignoreNextEvent(event->source);
    mcl.popPage();
    return true;
  }

  // if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  // Toggle upper + lower case
  // if (encoders[1]->cur <= 25) {
  // encoders[1]->cur += 26;
  //} else if (encoders[1]->cur <= 51) {
  // encoders[1]->cur -= 26;
  //}
  // return true;
  //}

  // if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
  // Clear text
  // for (uint8_t n = 1; n < length; n++) {
  // text[n] = ' ';
  //}
  // text[0] = 'a';
  // encoders[0]->cur = 0;
  // DEBUG_PRINTLN(text);
  // update_char();
  // return true;
  //}

  return false;
}
