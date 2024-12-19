#include "LFOPage.h"
#include "LFO.h"
#include "MD.h"
#include "MCLGUI.h"
#include "ResourceManager.h"
#include "MidiActivePeering.h"
#include "A4.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_OFFSET 2
#define LFO_DESTINATION 1
#define LFO_SETTINGS 0

void LFOPage::setup() {
  //  lfo_track = &mcl_seq.lfo_tracks[0];

  DEBUG_PRINT_FN();
}

void LFOPage::init() {
  PerfPageParent::init();

  MD.sync_seqtrack(lfo_track->length, lfo_track->speed,
                     lfo_track->step_count);
  if (lfo_track->mode != LFO_MODE_FREE) {
    trig_interface.on();
  }


}

void LFOPage::cleanup() {
  PerfPageParent::cleanup();
  trig_interface.off();
}

void LFOPage::config_encoder_range(uint8_t i) {

 ((MCLEncoder *)encoders[i])->max = NUM_MD_TRACKS + 4 + 16; 

  uint8_t dest = encoders[i]->cur - 1;
  if (dest >= NUM_MD_TRACKS + 4) {
    ((MCLEncoder *)encoders[i + 1])->max = 127;
  }
  else if (dest >= NUM_MD_TRACKS) {
    ((MCLEncoder *)encoders[i + 1])->max = 7;
  }
  else {
     ((MCLEncoder *)encoders[i + 1])->max = 23; 
  }
}

void LFOPage::config_encoders() {
  if (page_mode == LFO_DESTINATION) {
    encoders[0]->cur = lfo_track->params[0].dest;
    ((MCLEncoder *)encoders[0])->max = NUM_MD_TRACKS + 4;
    encoders[1]->cur = lfo_track->params[0].param;
    ((MCLEncoder *)encoders[1])->max = 23;
    encoders[2]->cur = lfo_track->params[1].dest;
    ((MCLEncoder *)encoders[2])->max = NUM_MD_TRACKS + 4;
    encoders[3]->cur = lfo_track->params[1].param;
    ((MCLEncoder *)encoders[3])->max = 23;

    config_encoder_range(0);
    config_encoder_range(2);
  }
  else if (page_mode == LFO_SETTINGS) {
    encoders[0]->cur = waveform;
    ((MCLEncoder *)encoders[0])->max = 5;

    encoders[1]->cur = lfo_track->speed;
    ((MCLEncoder *)encoders[1])->max = 127;

    encoders[2]->cur = lfo_track->params[0].depth;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->params[1].depth;
    ((MCLEncoder *)encoders[3])->max = 127;
  }
  else if (page_mode == LFO_OFFSET) {
    encoders[2]->cur = lfo_track->params[0].offset;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->params[1].offset;
    ((MCLEncoder *)encoders[3])->max = 127;
  }

  //  loop();

  init_encoders_used_clock();

}

void LFOPage::loop() {
  if (page_mode == LFO_DESTINATION) {
    config_encoder_range(0);
    config_encoder_range(2);

    if (encoders[0]->hasChanged()) {
      lfo_track->params[0].dest = encoders[0]->cur;
    }
    if (encoders[1]->hasChanged()) {
      lfo_track->params[0].param = encoders[1]->cur;
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->params[1].dest = encoders[2]->cur;
    }
    if (encoders[3]->hasChanged()) {
      lfo_track->params[1].param = encoders[3]->cur;
    }
  }
  // wav_tables need to be recalculated when depth or waveform changes.

  else if (page_mode == LFO_SETTINGS) {
    if (encoders[0]->hasChanged()) {
      lfo_track->set_wav_type(encoders[0]->cur);
    }

    if (encoders[1]->hasChanged()) {
      lfo_track->set_speed(encoders[1]->cur);
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->set_depth(0, encoders[2]->cur);
    }

    if (encoders[3]->hasChanged()) {
      lfo_track->set_depth(1, encoders[3]->cur);
    }
  }

  else if (page_mode == LFO_OFFSET) {
    if (encoders[2]->hasChanged()) {
      if (lfo_track->params[0].dest > NUM_MD_TRACKS + 4) {
        lfo_track->params[0].offset = encoders[2]->cur;
      }
      else { encoders[2]->cur = encoders[2]->old; }
    }

    if (encoders[3]->hasChanged()) {
       if (lfo_track->params[1].dest > NUM_MD_TRACKS + 4) {
         lfo_track->params[1].offset = encoders[3]->cur;
       }
       else { encoders[3]->cur = encoders[3]->old; }
    }
  }

}

void LFOPage::display() {
  oled_display.clearDisplay();
  uint8_t lfo_track_num = lfo_track->track_number;
  mcl_gui.draw_panel_number(lfo_track_num);
  mcl_gui.draw_panel_toggle("ON", "OFF", lfo_track->enable);
  draw_page_index(false, lfo_track->step_count / 4);

  uint8_t x = mcl_gui.knob_x0 + 5;
  uint8_t y = 8;
  uint8_t lfo_height = 7;
  uint8_t width = 13;

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  SeqPage::draw_knob_frame();
  const char *info1;
  const char *info2;


  if (page_mode == LFO_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);
    draw_dest(2, encoders[2]->cur);
    draw_param(3, encoders[2]->cur, encoders[3]->cur);
    info2 = "LFO>DST";
  }
  else if (page_mode == LFO_SETTINGS) {
    uint8_t inc = LFO_LENGTH / width;
    for (uint8_t n = 0; n < LFO_LENGTH; n += inc, x++) {
      if (n < LFO_LENGTH) {
        int16_t out = 0;

        switch (lfo_track->wav_type) {
          case IRAMP_WAV:
          case EXP_WAV:
             out = (int16_t)128 - lfo_track->wav_tables[lfo_track->wav_type - 2][n];
          break;
          default:
             out = lfo_track->wav_tables[lfo_track->wav_type][n];
          break;
        }
        uint8_t sample = ((int16_t) out * (int16_t) lfo_height) / 128;

        oled_display.drawPixel(x, y + lfo_height - sample, WHITE);
      }
    }
    x = mcl_gui.knob_x0 + 2;
    oled_display.setCursor(x + 5, 6);
    oled_display.print(F("WAV"));

    draw_knob(1, encoders[1], "SPD");
    draw_knob(2, encoders[2], "DEP1");
    draw_knob(3, encoders[3], "DEP2");
    info2 = "LFO>MOD";
  }
  else { //if (page_mode == LFO_OFFSET) {
    draw_knob(2, encoders[2], "OFS1");
    draw_knob(3, encoders[3], "OFS2");
    info2 = "LFO>OFS";
  }

  oled_display.setFont(&TomThumb);

  const uint64_t slide_mask = 0;
  const uint64_t mute_mask = 0;

  switch (lfo_track->mode) {
    case LFO_MODE_TRIG:
    info1 = "TRIG";
    break;
    case LFO_MODE_ONE:
    info1 = "ONE";
    break;
    default:
    info1 = "FREE";
    break;
  }

  if (lfo_track->mode == LFO_MODE_TRIG || lfo_track->mode == LFO_MODE_ONE) {
    draw_lock_mask(0, 0, lfo_track->step_count, lfo_track->length, true);
    draw_mask(0, lfo_track->pattern_mask, lfo_track->step_count,
              lfo_track->length, mute_mask, slide_mask, true);
    if ((uint16_t)lfo_track->pattern_mask != trigled_mask) {
      trigled_mask = (uint16_t)lfo_track->pattern_mask;
      MD.set_trigleds(lfo_track->pattern_mask, TRIGLED_STEPEDIT);
    }
  }

  mcl_gui.draw_panel_labels(info1, info2);

}

void LFOPage::learn_param(uint8_t track, uint8_t param, uint8_t value) {
  bool reconfig = false;
  if (mcl.currentPage() == LFO_PAGE) {
  if (page_mode == LFO_DESTINATION) {
    if (encoders[0]->cur == 0 && encoders[1]->cur > 0) {
      lfo_track->params[0].dest = track + 1;
      lfo_track->params[0].param = param;
      reconfig = true;
    }
    if (encoders[2]->cur == 0 && encoders[3]->cur > 0) {
      lfo_track->params[1].dest = track + 1;
      lfo_track->params[1].param = param;
      reconfig = true;
    }
  }
  }
  if (lfo_track->params[0].dest - 1 == track && lfo_track->params[0].param == param) {
    lfo_track->params[0].offset = value;
    reconfig = true;
  }
  if (lfo_track->params[1].dest - 1 == track && lfo_track->params[1].param == param) {
    lfo_track->params[1].offset = value;
    reconfig = true;
  }
  if (mcl.currentPage() == LFO_PAGE && reconfig) { config_encoders(); }
}


bool LFOPage::handleEvent(gui_event_t *event) {
  if (PerfPageParent::handleEvent(event)) { return true; }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    uint8_t page_select = 0;
    uint8_t step = track + (page_select * 16);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device == &Analog4) {
        // mcl.setPage(SEQ_EXTSTEP_PAGE)
        return true;
      }
      if (!IS_BIT_SET64(lfo_track->pattern_mask, step)) {

        SET_BIT64(lfo_track->pattern_mask, step);
      } else {
        DEBUG_PRINTLN(F("Trying to clear"));
        if (clock_diff(note_interface.note_hold[port], g_clock_ms) <
            TRIG_HOLD_TIME) {
          CLEAR_BIT64(lfo_track->pattern_mask, step);
        }
      }
    }
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES: {
        goto lfo_enable;
      }
      case MDX_KEY_UP: {
        if (page_mode < LFO_OFFSET) {
          page_mode++;
          config_encoders();
        }
        return true;
      }
      case MDX_KEY_DOWN: {
        if (page_mode > 0) {
          page_mode--;
          config_encoders();
        }
        return true;
      }
      }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    page_mode++;
    if (page_mode > LFO_OFFSET) { page_mode = 0; }
    config_encoders();
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    if (lfo_track->mode >= LFO_MODE_ONE) {
      lfo_track->mode = 0;
    } else {
      lfo_track->mode += 1;
    }
    if (lfo_track->mode == LFO_MODE_FREE) {
      trig_interface.off();
    } else {
      trig_interface.on();
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    lfo_enable:
    lfo_track->enable = !(lfo_track->enable);
    if (!lfo_track->enable) { lfo_track->reset_params(); }
  }
  return false;
}
