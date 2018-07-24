#include "MCL.h"
#include "OscPage.h"

void osc_mod_handler(Encoder *enc) {}

void OscPage::setup() {}

float OscPage::get_freq() {
  float fzero = 440;
  float a = pow(2.00,1.00/12.00);
  DEBUG_PRINTLN(a);
  float n = enc2.cur - 64;
  float fn = fzero * pow(a,n);
  return fn;
}
float OscPage::get_phase() { return enc3.cur; }
uint8_t OscPage::get_osc_type() { return enc1.cur; }
void OscPage::init() {
  DEBUG_PRINTLN("seq extstep init");
  create_chars_mixer();
  classic_display = false;
  md_exploit.on();
  note_interface.state = true;
}

void OscPage::cleanup() { md_exploit.off(); }
bool OscPage::handleEvent(gui_event_t *event) {
  if (BUTTON_PRESSED(Buttons.BUTTON3)) {
    wav_designer.render();
    return true;
  }

  if (BUTTON_PRESSED(Buttons.ENCODER1)) {
    GUI.setPage(&(wav_designer.pages[0]));

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER2)) {
    GUI.setPage(&(wav_designer.pages[1]));

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER3)) {
    GUI.setPage(&(wav_designer.pages[2]));

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER4)) {

    GUI.setPage(&(wav_designer.mixer));

    return true;
  }

  return false;
}

void OscPage::loop() {
  MCLEncoder *enc_ = &enc4;
  int dir = 0;
  uint8_t newval;
  for (int i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      if ((enc_->getValue() - enc_->old) < 0) {
        dir = -1;
      } else {
        dir = 1;
      }
      if (enc1.cur == 1) {
        newval = sine_levels[i] + dir;
        if ((newval <= 127) && (newval >= 0)) {
          sine_levels[i] = newval;
        }
      }
      if (enc1.cur == 5) {
      }
    }
  }
  enc_->cur = 64 + dir;
  enc_->old = 64;
}
void OscPage::display() {
  oled_display.clearDisplay();
  GUI.setLine(GUI.LINE1);


  GUI.put_value_at(0, enc1.cur);
  GUI.put_value_at(4, enc2.cur);
 // DEBUG_PRINTLN(get_freq());
  GUI.put_value_at(9, enc3.cur);

  GUI.put_value_at(12, enc4.cur);

  GUI.put_value_at1(15, id);

/* switch (enc1.cur) {
  case 0:
    GUI.put_string_at(0, "--");
    break;
  case 1:
    GUI.put_string_at(0, "SIN");
    break;
  case 2:
    GUI.put_string_at(0, "TRI");
    break;
  case 3:
    GUI.put_string_at(0, "PUL");
    break;
  case 4:
    GUI.put_string_at(0, "SAW");
    break;
  case 5:
    GUI.put_string_at(0, "CUS");
    break;
  }
*/

  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  draw_levels();
  oled_display.display();
}

void OscPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
#ifdef OLED_DISPLAY

    scaled_level = (uint8_t)(((float)sine_levels[i] / (float)127) * 15);
    oled_display.fillRect(0 + i * 6, 12 + (15 - scaled_level), 4,
                          scaled_level + 1, WHITE);
#else

    scaled_level = (int)(((float)sine_levels[i] / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);

    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }

#endif
  }
  GUI.put_string_at(0, str);
}
