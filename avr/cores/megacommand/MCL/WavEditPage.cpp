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

  start = 0;
  end = sampleLength;

  offset = 0;
  encoders[0]->cur = 0;
  encoders[1]->cur = 127;
  encoders[2]->cur = 64;
  encoders[2]->old = 64;
  samples_per_pixel = max_visible_length / WAV_DRAW_WIDTH;

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
    start = encoders[0]->cur * samples_per_pixel;
    end = encoders[1]->cur * samples_per_pixel;
    render(start, end, offset, samples_per_pixel);
    samples_per_pixel = ((end - start) / WAV_DRAW_WIDTH);
    if (samples_per_pixel < 1) {
      samples_per_pixel = 1;
    }
    encoders[0]->cur = 0;
    encoders[1]->cur = 127;
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
                         uint32_t offset, uint32_t samples_per_pixel) {

  uint32_t sampleFormat = wav_file.header.fmt.bitRate;

  int32_t sample_index = sample_start + offset;
  uint8_t pixel_offset = 0;

  uint16_t sample_max = (pow(2, wav_file.header.fmt.bitRate) / 2);
  int32_t min_value;
  int32_t max_value;
  uint32_t start;
  uint32_t length;

  if (fov_samples_per_pixel < 1) {
    fov_samples_per_pixel = 1;
    fov_length = fov_w;
  } else {
    fov_length = length;
  }

  int32_t sample_index = sample_offset - (length / 2);

  float scalar = (float)(fov_h / 2) / (float)sample_max;

  DEBUG_PRINTLN("re-rendering");
  for (uint8_t n = 0; n < WAV_DRAW_WIDTH; n++) {
    // Check that we're not searching for -ve sample index space.
    if (sample_index < 0) {
      min_value = 0;
      max_value = 0;
    } else {
      start = sample_index;
      length = samples_per_pixel;
      wav_file.find_peaks(0, length, start, &max_value, &min_value);
    }

  // wav_buf[n][0] = ((float) max_value / (float)sample_max) * (float)
  // (WAV_DRAW_HEIGHT / 2); wav_buf[n][1] = ((float) min_value /
  // (float)sample_max) * (float) (WAV_DRAW_HEIGHT / 2);
  next:
    wav_buf[n][0] = (float)max_value * scalar;
    wav_buf[n][1] = (float)min_value * scalar;
    sample_index += samples_per_pixel;
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

    // Don't allow for translation when entire waveform is visible.
    uint32_t visibleLength = (max_visible_length) / pow(2, encoders[3]->cur);
    samples_per_pixel = (visibleLength / WAV_DRAW_WIDTH);
    if (samples_per_pixel < 1) {
      samples_per_pixel = 1;
      encoders[3]->cur = encoders[3]->old;
      return;
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

    start = (max_visible_length / 2) - (visibleLength / 2);
    end -= (max_visible_length / 2) + (visibleLength / 2);

    render(start, end, offset, samples_per_pixel);
    //  encoders[0]->cur = 0;
    //  encoders[1]->cur = 127;
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
      color = BLACK;
    }
    if (samples_per_pixel > 1) {
      oled_display.drawLine(n, (WAV_DRAW_HEIGHT / 2) + wav_buf[n][0], n,
                            (WAV_DRAW_HEIGHT / 2) + wav_buf[n][1], color);
    } else {
      int8_t val = 0;
      if (abs(wav_buf[n][1]) > abs(wav_buf[n][0])) {
        val = wav_buf[n][1];
      } else {
        val = wav_buf[n][0];
      }
      oled_display.drawLine(x, y, n, (WAV_DRAW_HEIGHT / 2) + val, color);
      x = n;
      y = (WAV_DRAW_HEIGHT / 2) + val;
    }
  }

#endif
}
