#include "MCL_impl.h"
#include "WavEditPage.h"

MCLEncoder wav_edit_param1(0, WAV_DRAW_WIDTH, ENCODER_RES_SYS);
MCLEncoder wav_edit_param2(0, WAV_DRAW_WIDTH, ENCODER_RES_SYS);
MCLEncoder wav_edit_param3(0, 255, ENCODER_RES_SYS);
MCLEncoder wav_edit_param4(0, 255, ENCODER_RES_SYS);

WavEditPage wav_edit_page(&wav_edit_param1, &wav_edit_param2, &wav_edit_param3,
                          &wav_edit_param4);

void WavEditPage::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
}

void WavEditPage::open(char *file) { wav_file.open(file, false); }

void WavEditPage::init() {
  uint32_t sampleLength =
      (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
      (wav_file.header.fmt.bitRate / 8);

  uint32_t max_visible_length =
      ((float)wav_file.header.fmt.sampleRate * WAV_SECONDS);

  if (sampleLength < max_visible_length) {
    max_visible_length = sampleLength;
  }

  start = 0;
  end = sampleLength;

  offset = 0;
  encoders[0]->cur = 0;
  encoders[1]->cur = WAV_DRAW_WIDTH;
  encoders[2]->cur = 64;
  encoders[2]->old = 64;
  samples_per_pixel = max_visible_length / WAV_DRAW_WIDTH;

  render(start, end, offset, samples_per_pixel);
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void WavEditPage::cleanup() {
  wav_file.close();
  DEBUG_PRINT_FN();
}
bool WavEditPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    start = (encoders[0]->cur) * samples_per_pixel;
    end = (encoders[1]->cur) * samples_per_pixel;
    render(start, end, offset, samples_per_pixel);
    samples_per_pixel = ((end - start) / WAV_DRAW_WIDTH);
    if (samples_per_pixel < 1) {
      samples_per_pixel = 1;
    }
    encoders[0]->cur = 0;
    encoders[1]->cur = WAV_DRAW_WIDTH;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.popPage();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    return true;
  }

  return false;
}

#define WAV_NO_VAL 127

void WavEditPage::render(uint32_t sample_start, uint32_t sample_end,
                         int32_t sample_offset, uint32_t samples_per_pixel) {

  int32_t sampleLength =
      (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
      (wav_file.header.fmt.bitRate / 8);

  uint32_t sampleFormat = wav_file.header.fmt.bitRate;

  uint16_t sample_max = (pow(2, wav_file.header.fmt.bitRate) / 2);
  wav_sample_t min_sample;
  wav_sample_t max_sample;

  float scalar = (float)(WAV_DRAW_HEIGHT / 2) / (float)sample_max;

  int32_t sample_index = sample_start;

  DEBUG_PRINTLN("re-rendering");
  DEBUG_PRINTLN(sample_index + sample_offset);
  DEBUG_PRINTLN(sampleLength);
  for (uint8_t n = 0; n < WAV_DRAW_WIDTH; n++) {
    // Check that we're not searching for -ve sample index space.
    if (sample_index + sample_offset < sampleLength) {
      if (sample_index + sample_offset < 0) {
          wav_buf[n][0] = WAV_NO_VAL;
          wav_buf[n][1] = WAV_NO_VAL;
      } else {
        wav_file.find_peaks(0, samples_per_pixel, sample_index + sample_offset,
                            &max_sample, &min_sample);
        wav_buf[n][0] = (float)max_sample.val * scalar;
        wav_buf[n][1] = (float)min_sample.val * scalar;
      }
    } else {
      wav_buf[n][0] = WAV_NO_VAL;
      wav_buf[n][1] = WAV_NO_VAL;
    }

    sample_index += samples_per_pixel;
  }
}

void WavEditPage::loop() {
  if (encoders[0]->hasChanged()) {
    if (encoders[0]->cur >= encoders[1]->cur) {
      encoders[0]->cur = encoders[1]->cur - 1;
    }
  }
  if (encoders[1]->hasChanged()) {
    if (encoders[1]->cur <= encoders[0]->cur) {
      encoders[1]->cur = encoders[0]->cur + 1;
    }
  }
  if (encoders[2]->hasChanged()) {
    // Horizontal translation
    int16_t diff = encoders[2]->cur - encoders[2]->old;
    offset += diff * samples_per_pixel;
    render(start, end, offset, samples_per_pixel);
    encoders[2]->cur = 64;
    encoders[2]->old = 64;
  }
  if (encoders[3]->hasChanged()) {
    // ZOOM
    uint32_t sampleLength =
        (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
        (wav_file.header.fmt.bitRate / 8);

    uint32_t max_visible_length =
        ((float)wav_file.header.fmt.sampleRate * WAV_SECONDS);

    if (sampleLength < max_visible_length) {
      max_visible_length = sampleLength;
    }

    uint32_t visibleLength = (max_visible_length) / pow(2, encoders[3]->cur);
    samples_per_pixel = (visibleLength / WAV_DRAW_WIDTH);
    if (samples_per_pixel < 1) {
      samples_per_pixel = 1;
      encoders[3]->cur = encoders[3]->old;
      return;
    }

    start = (max_visible_length / 2) - (visibleLength / 2);
    end = (max_visible_length / 2) + (visibleLength / 2);

    render(start, end, offset, samples_per_pixel);
    //  encoders[0]->cur = 0;
    //  encoders[1]->cur = 127;
  }
}

wav_sample_t WavEditPage::get_selection_sample_start() {
  wav_sample_t sample_start;
  int32_t pos = ((encoders[0]->cur) * samples_per_pixel) + offset + start;
  if (pos < 0) {
    sample_start.pos = 0;
  } else {
    sample_start.pos = pos;
  }
  sample_start.val = wav_buf[encoders[0]->cur][1];
  return sample_start;
}

wav_sample_t WavEditPage::get_selection_sample_end() {
  uint32_t sampleLength =
      (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
      (wav_file.header.fmt.bitRate / 8);

  wav_sample_t sample_end;
  sample_end.pos = ((encoders[1]->cur) * samples_per_pixel) + offset + start;
  if (sample_end.pos > sampleLength) {
    sample_end.pos = sampleLength;
  }
  sample_end.val = wav_buf[encoders[1]->cur][1];
  return sample_end;
}

void WavEditPage::display() {
  // oled_display.clearDisplay()
  //

#ifdef OLED_DISPLAY

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);
  oled_display.fillRect(0, 0, encoders[0]->cur, 32, BLACK);
  oled_display.fillRect(encoders[0]->cur, 0,
                        encoders[1]->cur - encoders[0]->cur, 32, WHITE);
  oled_display.fillRect(encoders[1]->cur, 0, 128 - encoders[1]->cur, 32, BLACK);

  draw_wav();
  oled_display.setFont(oldfont);
  oled_display.display();
#endif
}

void WavEditPage::draw_wav() {
#ifdef OLED_DISPLAY
  uint8_t x = 0;
  uint8_t y = WAV_DRAW_HEIGHT / 2;

  uint8_t color = WHITE;
  bool first_val = false;

  for (uint8_t n = 0; n < WAV_DRAW_WIDTH; n++) {
    if ((n <= encoders[0]->cur) || (n >= encoders[1]->cur)) {
      color = WHITE;
    } else {
      color = BLACK;
    }

    oled_display.drawPixel(n, (WAV_DRAW_HEIGHT / 2), color);

    if (samples_per_pixel > 1) {
      oled_display.drawLine(n, (WAV_DRAW_HEIGHT / 2) - wav_buf[n][0], n,
                            (WAV_DRAW_HEIGHT / 2) - wav_buf[n][1], color);
    } else {
      int8_t val = 0;
      if (abs(wav_buf[n][1]) > abs(wav_buf[n][0])) {
        val = wav_buf[n][1];
      } else {
        val = wav_buf[n][0];
      }
      if (val != WAV_NO_VAL) {
        if ((first_val == false)) {
          x = n;
          y -= val;
          first_val = true;
        }
        oled_display.drawLine(x, y, n, (WAV_DRAW_HEIGHT / 2) - val, color);
        oled_display.drawLine(x, y - 1, n, (WAV_DRAW_HEIGHT / 2) - val - 1,
                              color);
        oled_display.drawLine(x, y + 1, n, (WAV_DRAW_HEIGHT / 2) - val + 1,
                              color);
        y = (WAV_DRAW_HEIGHT / 2) - val;
      }
      x = n;
    }
  }
  wav_sample_t current_sample = get_selection_sample_start();

  oled_display.setCursor(100, 26);
  oled_display.print(current_sample.pos);
#endif
}
