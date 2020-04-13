#include "MCL_impl.h"
#include "WavEditPage.h"

MCLEncoder wav_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder wav_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<9> wav_menu_page(&wav_menu_layout, &wav_menu_value_encoder, &wav_menu_entry_encoder);

const menu_t<4> wav_menu_layout PROGMEM = {
    "STP",
    {
        {"CLEAR:", 0, 2, 2, (uint8_t *)&opt_clear_step, (Page *)NULL, opt_clear_step_locks_handler, { {0, "--",}, {1, "LCKS"}}},
        {"COPY STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_copy_step_handler, {}},
        {"PASTE STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_paste_step_handler, {}},
        {"MUTE STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_mute_step_handler, {}},
    },
    step_menu_handler,
};

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

  fov_offset = max_visible_length / 2;

  selection_start = fov_offset - max_visible_length / 2;
  selection_end = fov_offset + max_visible_length / 2;

  for (uint8_t i = 0; i < 4; i++) {
  encoders[i]->cur = 64;
  encoders[i]->old = 64;
  }

  render(max_visible_length, fov_offset);
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
    uint32_t length = selection_end - selection_start;
    fov_offset = selection_start + (length / 2);
    render(length, fov_offset);

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

void WavEditPage::render(uint32_t length, int32_t sample_offset) {
  int32_t sampleLength =
      (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
      (wav_file.header.fmt.bitRate / 8);

  uint32_t sampleFormat = wav_file.header.fmt.bitRate;

  uint32_t sample_max = (pow(2, wav_file.header.fmt.bitRate) / 2);
  wav_sample_t c0_min_sample;
  wav_sample_t c0_max_sample;
  wav_sample_t c1_min_sample;
  wav_sample_t c1_max_sample;

  fov_samples_per_pixel = length / fov_w;

  if (fov_samples_per_pixel < 1) {
    fov_samples_per_pixel = 1;
    fov_length = fov_w;
  } else {
    fov_length = length;
  }

  int32_t sample_index = sample_offset - (length / 2);

  float scalar = (float)(fov_h / 2) / (float)sample_max;

  if (draw_mode == WAV_DRAW_STEREO) {
    scalar /= 2;
  }

  for (uint8_t n = 0; n < fov_w; n++) {
    // Check that we're not searching for -ve sample index space.
    if (sample_index < (fov_length / 2) + sample_offset) {
      if ((sample_index < 0) || (sample_index >= sampleLength)) {
        wav_buf[0][n][0] = WAV_NO_VAL;
        wav_buf[0][n][1] = WAV_NO_VAL;
        wav_buf[1][n][0] = WAV_NO_VAL;
        wav_buf[1][n][1] = WAV_NO_VAL;

      } else {
        wav_file.find_peaks(fov_samples_per_pixel, sample_index, &c0_max_sample,
                            &c0_min_sample, &c1_max_sample, &c1_min_sample);
        wav_buf[0][n][0] = (float)c0_max_sample.val * scalar;
        wav_buf[0][n][1] = (float)c0_min_sample.val * scalar;
        if (wav_file.header.fmt.numChannels < 2) {
          wav_buf[1][n][0] = WAV_NO_VAL;
          wav_buf[1][n][1] = WAV_NO_VAL;
        } else {
          wav_buf[1][n][0] = (float)c1_max_sample.val * scalar;
          wav_buf[1][n][1] = (float)c1_min_sample.val * scalar;
        }
      }
    } else {
      wav_buf[0][n][0] = WAV_NO_VAL;
      wav_buf[0][n][1] = WAV_NO_VAL;
      wav_buf[1][n][0] = WAV_NO_VAL;
      wav_buf[1][n][1] = WAV_NO_VAL;
    }

    sample_index += fov_samples_per_pixel;
  }
}

void WavEditPage::loop() {
  if (wav_edit_param3.hasChanged()) {
    uint32_t sampleLength =
        (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
        (wav_file.header.fmt.bitRate / 8);

    int32_t diff =
        (wav_edit_param3.cur - wav_edit_param3.old) * fov_samples_per_pixel;

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
    wav_edit_param3.cur = 64;
    wav_edit_param3.old = 64;
  }

  if (wav_edit_param4.hasChanged()) {
    uint32_t sampleLength =
        (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
        (wav_file.header.fmt.bitRate / 8);

    int32_t diff =
        (wav_edit_param4.cur - wav_edit_param4.old) * fov_samples_per_pixel;

    if (selection_end + diff > sampleLength) {
      selection_end = sampleLength;
    } else if (selection_end + diff <= selection_start) {
      selection_end = selection_start + 1;
    } else {
      selection_end += diff;
    }
    wav_edit_param4.cur = 64;
    wav_edit_param4.old = 64;
  }

  if (wav_edit_param1.hasChanged()) {
    // Horizontal translation
    int16_t diff = wav_edit_param1.cur - wav_edit_param1.old;
    fov_offset += diff * fov_samples_per_pixel;

    render(fov_length, fov_offset);
    wav_edit_param1.cur = 64;
    wav_edit_param1.old = 64;
  }
  if (wav_edit_param2.hasChanged()) {

    int16_t diff = wav_edit_param2.cur - wav_edit_param2.old;

    uint32_t sampleLength =
        (wav_file.header.data.chunk_size / wav_file.header.fmt.numChannels) /
        (wav_file.header.fmt.bitRate / 8);

    uint32_t max_visible_length =
        ((float)wav_file.header.fmt.sampleRate * WAV_SECONDS);

    if (sampleLength < max_visible_length) {
      max_visible_length = sampleLength;
    }
    uint32_t length;

    if (diff < 0) {
      length = fov_length * 2;
    }
    if (diff > 0) {
      length = fov_length / 2;
    }

    if (length < fov_w) {
      length = fov_w;
    }
    if (length > max_visible_length) {
      length = max_visible_length;
    }
    render(length, fov_offset);
    wav_edit_param2.cur = 64;
    wav_edit_param2.old = 64;
  }
}

uint8_t WavEditPage::get_selection_start() { return wav_edit_param3.cur; }

uint8_t WavEditPage::get_selection_end() {
  return wav_edit_param3.cur + wav_edit_param4.cur;
}

uint8_t WavEditPage::get_selection_width() { return wav_edit_param4.cur; }

wav_sample_t WavEditPage::get_selection_sample_start() {
  wav_sample_t sample_start;
  int32_t pos = selection_start;
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
  sample_end.pos = selection_end;
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
  uint8_t y;

  bool draw_selection = true;

  int32_t start = (int32_t)fov_offset - (int32_t)(fov_length / 2);
  int32_t end = (int32_t)fov_offset + (int32_t)(fov_length / 2);

  DEBUG_DUMP(selection_start);
  DEBUG_DUMP(start);
  DEBUG_DUMP(selection_end);
  DEBUG_DUMP(end);
  if (selection_start < start) {
    selection_x1 = 0;
  } else if (selection_start > end) {
    draw_selection = false;
  } else {
    selection_x1 = (selection_start - start) / fov_samples_per_pixel;
  }

  if (selection_end < start) {
    draw_selection = false;
  } else {
    if (selection_end > end) {
      selection_x2 = fov_w;
    } else {
      selection_x2 = (selection_end - start) / fov_samples_per_pixel;
      if (selection_x2 > fov_w) {
        selection_x2 = fov_w;
      }
    }
  }
  oled_display.clearDisplay();

  if (draw_selection) {
    oled_display.fillRect(selection_x1, 0, selection_x2 - selection_x1, 32,
                          WHITE);
  }

  uint8_t x = 0;

  uint8_t color = WHITE;
  bool first_val = false;
  uint8_t n = 0;
  uint8_t c = 0;

  uint8_t channels;
  uint8_t axis_y = fov_h / 2;
  if (draw_mode != WAV_DRAW_STEREO) {
    c = draw_mode;
    channels = 1;
  } else {
    axis_y /= 2;
    channels = 2;
  }
  for (uint8_t a = 0; a < channels; a++) {
    y = axis_y;
    first_val = false;
    for (uint8_t n = 0; n < fov_w; n++) {
      if ((n >= selection_x1) && (n <= selection_x2) && (draw_selection)) {
        color = BLACK;
      } else {
        color = WHITE;
      }

      // Draw axis
      if (wav_buf[c][n][0] != WAV_NO_VAL) {
        oled_display.drawPixel(n, axis_y, color);
      }

      // Draw sampled waveform
      if (fov_samples_per_pixel > 1) {
        oled_display.drawLine(n, axis_y - wav_buf[c][n][0], n,
                              axis_y - wav_buf[c][n][1], color);
      } else {
        // Draw real waveform.
        int8_t val = 0;
        if (abs(wav_buf[c][n][1]) > abs(wav_buf[c][n][0])) {
          val = wav_buf[c][n][1];
        } else {
          val = wav_buf[c][n][0];
        }
        if (val != WAV_NO_VAL) {
          if ((first_val == false)) {
            x = n;
            y -= val;
            first_val = true;
          }
          oled_display.drawLine(x, y, n, axis_y - val, color);
          oled_display.drawLine(x, y - 1, n, axis_y - val - 1, color);
          oled_display.drawLine(x, y + 1, n, axis_y - val + 1, color);
          y = axis_y - val;
        }
        x = n;
      }
    }
    if (draw_mode == WAV_DRAW_STEREO) {
      c += 1;
      axis_y += fov_h / 2;
    }
  }
  wav_sample_t current_sample = get_selection_sample_start();

  float seconds = current_sample.pos / (float)wav_file.header.fmt.sampleRate;
  int16_t minutes = seconds / 60;
  int16_t ms = ((float)seconds - int(seconds)) * 1000;

  char str[10];
  uint8_t i = 0;
  for (i = 0; i < 10 && wav_file.filename[i] != '.'; i++) {
    str[i] = wav_file.filename[i];
  }
  str[i] = '\0';
  oled_display.setCursor(88, 6);
  oled_display.print(str);

  oled_display.setCursor(88, 18);

  uint16_t sample_rate = (wav_file.header.fmt.sampleRate / 1000);
  oled_display.print(sample_rate);
  oled_display.print(".");

  uint8_t decimal = ((((float)wav_file.header.fmt.sampleRate / (float)1000) -
                      (float)sample_rate) *
                     (float)10.0) +
                    0.5;
  oled_display.print(decimal);
  oled_display.print("k ");

  oled_display.print(wav_file.header.fmt.bitRate);
  oled_display.print("/");
  oled_display.print(wav_file.header.fmt.numChannels);

  oled_display.setCursor(88, 24);
  oled_display.print(current_sample.pos);

  oled_display.setCursor(88, 30);

  oled_display.print(minutes);
  oled_display.print(":");
  oled_display.print(int(seconds));
  oled_display.print(":");
  oled_display.print(ms);

  oled_display.setFont(oldfont);
  oled_display.display();

#endif
}
