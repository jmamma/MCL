#include "MCL.h"
#include "WavEditPage.h"

MCLEncoder wav_edit_param1(0, 255, ENCODER_RES_SYS);

WavEditPage wav_edit_page(&wav_edit_param1);

void WavEditPage::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
}


void WavEditPage::init() {
  wav_file.open("test.wav", false);
  DEBUG_PRINTLN("seq extstep init");
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void WavEditPage::cleanup() { DEBUG_PRINT_FN(); }
bool WavEditPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    return true;
  }

  return false;
}

#define BUF_SIZE 256;
void WavEditPage::loop() {
  if (encoders[0]->hasChanged()) {

  uint32_t sampleLength = (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
                 (wav_file.header.fmt.bitRate / 8);
  uint32_t sampleFormat = wav_file.header.fmt.bitRate;

  uint32_t samples_per_pixel = ((sampleLength / pow(2,encoders[0]->cur)) / WAV_DRAW_WIDTH);

  uint32_t sample_index = 0;

  uint16_t sample_max = (pow(2, wav_file.header.fmt.bitRate) / 2);
  int16_t min_value;
  int16_t max_value;

  float scalar =(float)  ( WAV_DRAW_HEIGHT / 2) / (float) sample_max;

  DEBUG_PRINTLN("re-rendering");
  for (uint8_t n = 0; n < WAV_DRAW_WIDTH; n++) {
          wav_file.find_peaks(0, samples_per_pixel, sample_index, &max_value, &min_value);
          //wav_buf[n][0] = ((float) max_value / (float)sample_max) * (float) (WAV_DRAW_HEIGHT / 2);
          //wav_buf[n][1] = ((float) min_value / (float)sample_max) * (float) (WAV_DRAW_HEIGHT / 2);
          wav_buf[n][0] = (float) max_value * scalar;
          wav_buf[n][1] = (float) min_value * scalar;
          sample_index += samples_per_pixel;
    }
  }
}
void WavEditPage::display() {
  // oled_display.clearDisplay();
  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.fillRect(0, 0, 128, 32, BLACK);
#endif
  }
   draw_wav();
#ifdef OLED_DISPLAY
   oled_display.display();
#endif
}

void WavEditPage::draw_wav() {
#ifdef OLED_DISPLAY
  uint8_t x = 0;
  uint8_t y = 0;


  for (uint8_t n = 0; n < 128; n++) {
   oled_display.drawLine(n, (WAV_DRAW_HEIGHT / 2) + wav_buf[n][0], n , (WAV_DRAW_HEIGHT / 2) + wav_buf[n][1], WHITE);
  }

#endif
}

