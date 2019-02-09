#include "MCL.h"
#include "TextInputPage.h"

void TextInputPage::setup() {}

void TextInputPage::init() {
#ifdef OLED_DISPLAY
  oled_display.setFont();
  oled_display.clearDisplay();
#endif
  last_clock = slowclock;
  encoders[0]->cur = 0;
}

void TextInputPage::init_text(char *text_, char *title_, uint8_t len) {
  textp = text_;
  title = title_;
  length = len;
  m_strncpy(text, text_, len);
  ((MCLEncoder *)encoders[0])->max = length;
  update_char();
}

void TextInputPage::update_char() {
  uint8_t x = 0;
  char allowedchar[70] =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_&@-=!";
      // Check to see that the character chosen is in the list of allowed
      // characters
  while ((text[encoders[0]->cur] != allowedchar[x]) && (x < 69)) {
    x++;
  }

  // Ensure the encoder does not go out of bounds, by resetting it to a
  // character within the allowed characters list
  encoders[1]->setValue(x);
  // Update the projectname.
  encoders[0]->old = encoders[0]->cur;
}

void TextInputPage::display() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  char allowedchar[70] =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_&@-=!";
      // Check to see that the character chosen is in the list of allowed
      // characters
      if (encoders[0]->hasChanged()) {

    update_char();
  }
  if (encoders[1]->hasChanged()) {
    last_clock = slowclock;
  }
  //    if ((encoders[2]->hasChanged())){
  text[encoders[0]->getValue()] = allowedchar[encoders[1]->getValue()];
  //  }

  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, title);
  GUI.setLine(GUI.LINE2);
  char tmp_str[18];
  m_strncpy(tmp_str, &text[0], 18);
  if (clock_diff(last_clock, slowclock) > FLASH_SPEED) {
    tmp_str[encoders[0]->getValue()] = ' ';
  }
  if (clock_diff(last_clock, slowclock) > FLASH_SPEED * 2) {
    last_clock = slowclock;
  }
  GUI.put_string_at_len(0, tmp_str, length);
}
bool TextInputPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    text_input_page.return_state = true;
    m_strncpy(text_input_page.textp, &(text_input_page.text[0]), text_input_page.length);
    GUI.popPage();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON2) ||
      EVENT_RELEASED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    text_input_page.return_state = false;
    GUI.popPage();
  }
  return false;
}
