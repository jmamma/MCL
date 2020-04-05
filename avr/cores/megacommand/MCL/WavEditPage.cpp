#include "MCL.h"
#include "WavEditPage.h"

MCLEncoder wav_edit_param1(0, 128, ENCODER_RES_SYS);
MCLEncoder wav_edit_param2(0, 128, ENCODER_RES_SYS);
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
  end = max_visible_length;

  selection_start = start;
  selection_end = end;

  offset = 0;
  encoders[0]->cur = 64;
  encoders[0]->old = 64;
  encoders[1]->cur = 64;
  encoders[1]->old = 64;
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
    uint32_t sampleLength =
        (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
        (wav_file.header.fmt.bitRate / 8);

    int32_t diff = (encoders[0]->cur - encoders[0]->old) * samples_per_pixel;

    if ((int32_t)selection_start + diff < 0) {
      selection_start = 0;
    } else {
      selection_start += diff;
      if (selection_end + diff > sampleLength) {
        selection_end = sampleLength;
      } else {
        selection_end += diff;
      }
    }
    encoders[0]->cur = 64;
    encoders[0]->old = 64;
  }

  if (encoders[1]->hasChanged()) {
    uint32_t sampleLength =
        (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
        (wav_file.header.fmt.bitRate / 8);

    int32_t diff = (encoders[1]->cur - encoders[1]->old) * samples_per_pixel;

    if (selection_end + diff > sampleLength) {
      selection_end = sampleLength;
    } else if (selection_end + diff < selection_start) {
      selection_end = selection_start;
    } else {
      selection_end += diff;
    }
    encoders[1]->cur = 64;
    encoders[1]->old = 64;
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

uint8_t WavEditPage::get_selection_start() { return encoders[0]->cur; }

uint8_t WavEditPage::get_selection_end() {
  return encoders[0]->cur + encoders[1]->cur;
}

uint8_t WavEditPage::get_selection_width() { return encoders[1]->cur; }

wav_sample_t WavEditPage::get_selection_sample_start() {
  wav_sample_t sample_start;
  int32_t pos = selection_start + offset;
  if (pos < 0) {
    sample_start.pos = 0;
  } else {
    sample_start.pos = pos;
  }
  // TODO
  // sample_start.val = wav_buf[get_selection_start()][1];
  return sample_start;
}

wav_sample_t WavEditPage::get_selection_sample_end() {
  uint32_t sampleLength =
      (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
      (wav_file.header.fmt.bitRate / 8);

  wav_sample_t sample_end;
  sample_end.pos = selection_end + offset;
  if (sample_end.pos > sampleLength) {
    sample_end.pos = sampleLength;
  }
  // TODO
  // sample_end.val = wav_buf[get_selection_end()][1];
  return sample_end;
}

void WavEditPage::display() {
  // oled_display.clearDisplay()
  //

#ifdef OLED_DISPLAY

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  uint8_t selection_x1;
  uint8_t selection_x2;

  bool draw_selection = true;

  if (selection_start < start) {
    selection_x1 = 0;
  } else if (selection_start > end) {
    draw_selection = false;
  } else {
    selection_x1 = (selection_start - start) / samples_per_pixel;
  }

  if (selection_end < start) {
    draw_selection = false;
  } else {
    if (selection_end > end) {
      selection_x2 = WAV_DRAW_WIDTH;
    } else {
      selection_x2 = (selection_end - start) / samples_per_pixel;
      if (selection_x2 > WAV_DRAW_WIDTH) {
        selection_x2 = WAV_DRAW_WIDTH;
      }
    }
  }
  oled_display.clearDisplay();

  if (draw_selection) {
    oled_display.fillRect(selection_x1, 0, selection_x2 - selection_x1, 32,
                          WHITE);
  }

  uint8_t x = 0;
  uint8_t y = WAV_DRAW_HEIGHT / 2;

  uint8_t color = WHITE;
  bool first_val = false;

  for (uint8_t n = 0; n < WAV_DRAW_WIDTH; n++) {
    if ((n >= selection_x1) && (n <= selection_x2) && (draw_selection)) {
      color = BLACK;
    } else {
      color = WHITE;
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
  oled_display.setFont(oldfont);
  oled_display.display();

#endif
}
