#include "MCL.h"
#include "OscMixerPage.h"

void OscMixerPage::setup() {}

void OscMixerPage::init() {
  create_chars_mixer();
  classic_display = false;
  md_exploit.on();
  note_interface.state = true;
}

void OscMixerPage::cleanup() { md_exploit.off(); }
bool OscMixerPage::handleEvent(gui_event_t *event) {
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

    return true;
  }

  return false;
}

void OscMixerPage::loop() {}
void OscMixerPage::display() {
  oled_display.clearDisplay();
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "                ");

  GUI.put_value_at2(0, enc1.cur);

  GUI.put_value_at2(4, enc2.cur);

  GUI.put_value_at2(8, enc3.cur);

  GUI.put_value_at(12, enc4.cur);

  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  draw_levels();
  oled_display.display();
}
float OscMixerPage::get_max_gain() {
  float max_gain = (float)0x7FFE / num_of_channels;
  return max_gain;
}
float OscMixerPage::get_gain(uint8_t channel) {
  MCLEncoder *enc_ = (MCLEncoder *)(encoders[channel]);
  float max_gain = (float)0x7FFE / num_of_channels;
  return ((float)enc_->cur / (float)127) * max_gain;
}
void OscMixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 4; i++) {
#ifdef OLED_DISPLAY

    scaled_level = (uint8_t)(((float)encoders[i]->cur / (float)127) * 15);
    oled_display.fillRect(0 + i * 6, 12 + (15 - scaled_level), 4,
                          scaled_level + 1, WHITE);
#else

    scaled_level = (int)(((float)encoders[i]->cur / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);

    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }

#endif
  }
  GUI.put_string_at(0, str);
}
