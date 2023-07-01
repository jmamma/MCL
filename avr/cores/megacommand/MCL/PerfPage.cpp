#include "MCL_impl.h"
#include "ResourceManager.h"
#include "MCLMemory.h"

#define LEARN_MIN 1
#define LEARN_MAX 2
#define LEARN_OFF 0

void PerfPage::setup() {
  DEBUG_PRINT_FN();
  page_mode = PERF_DESTINATION;
  perf_encoders[0] = &perf_param1;
  perf_encoders[1] = &perf_param2;
  perf_encoders[2] = &perf_param3;
  perf_encoders[3] = &perf_param4;
}

void PerfPage::init() {
  DEBUG_PRINT_FN();
  PerfPageParent::init();
  trig_interface.on();
  last_mask = last_blink_mask = 0;
  show_menu = false;
  last_page_mode = 255;
}

void PerfPage::set_led_mask() {
  uint16_t mask = 0;

  PerfEncoder *e = perf_encoders[perf_id];
  SET_BIT16(mask, e->active_scene_a);
  SET_BIT16(mask, e->active_scene_b);

  if (last_mask != mask) {
    MD.set_trigleds(mask, TRIGLED_EXCLUSIVENDYNAMIC);
  }
  bool blink = true;

  uint16_t blink_mask = 0;

  blink_mask |= e->perf_data.get_active_scene_mask();
  blink_mask &= ~mask;

  if (last_blink_mask != blink_mask) {
    MD.set_trigleds(blink_mask, TRIGLED_EXCLUSIVENDYNAMIC, blink);
  }

  last_blink_mask = blink_mask;
  last_mask = mask;
}

void PerfPage::cleanup() { PerfPageParent::cleanup(); trig_interface.off(); }

void PerfPage::config_encoder_range(uint8_t i) {
  ((PerfEncoder *)encoders[i])->max = NUM_MD_TRACKS + 4 + 16;
  ((PerfEncoder *)encoders[i + 1])->min = 0;

  uint8_t dest = encoders[i]->cur - 1;
  if (dest >= NUM_MD_TRACKS + 4) {
    ((MCLEncoder *)encoders[i + 1])->max = 127;
  } else if (dest >= NUM_MD_TRACKS) {
    ((MCLEncoder *)encoders[i + 1])->max = 7;
  } else {
    ((MCLEncoder *)encoders[i + 1])->max = 23;
  }
}

void PerfPage::config_encoders(uint8_t show_val) {
  encoders[1] = &fx_param2;
  encoders[2] = &fx_param3;
  encoders[3] = &fx_param4;

  if (page_mode > PERF_DESTINATION) {
    if (learn) {
      uint8_t scene = learn - 1;

      last_page_mode = page_mode;
      encoders[0] = &fx_param1;
      uint8_t c = page_mode - 1;

      PerfEncoder *e = perf_encoders[perf_id];
      PerfParam *p = &e->perf_data.scenes[scene].params[c];

      encoders[0]->cur = p->dest;
      encoders[1]->cur = p->param;

      uint8_t v = p->val;

      if (v == 255) { v = 0; }
      else { v++; }

      encoders[2]->cur = v;
      ((PerfEncoder *)encoders[2])->max = 128;

      config_encoder_range(0);
    }
  }

  if (page_mode == PERF_DESTINATION) {
    encoders[0] = perf_encoders[perf_id];
    ((PerfEncoder *)encoders[0])->max = 127;

    PerfData *d = &perf_encoders[perf_id]->perf_data;
    encoders[1]->cur = d->src;
    encoders[2]->cur = d->param;
    config_encoder_range(1);
    encoders[3]->cur = d->min;
    ((PerfEncoder *)encoders[3])->max = 127;
  }

  if (!show_val) {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    encoders[i]->old = encoders[i]->cur;
    ((LightPage *)this)->encoders_used_clock[i] = slowclock + SHOW_VALUE_TIMEOUT + 1;
  }
  }
}
void PerfPage::update_params() {

  uint8_t c = page_mode - 1;
  if (page_mode > PERF_DESTINATION) {
    config_encoder_range(0);

    if (encoders[0]->hasChanged() && encoders[0]->cur == 0) {
      encoders[1]->cur = 0;
    }

    if (learn) {
      uint8_t scene = learn - 1;

      PerfEncoder *e = perf_encoders[perf_id];
      PerfParam *p = &perf_encoders[perf_id]->perf_data.scenes[scene].params[c];
      p->dest = encoders[0]->cur;
      p->param = encoders[1]->cur;
      if (encoders[2]->cur > 0) {
        p->val = encoders[2]->cur - 1;
      }
    }
  } else {
    config_encoder_range(1);

    if (encoders[1]->hasChanged() && encoders[1]->cur == 0) {
      encoders[2]->cur = 0;
    }
    PerfData *d = &perf_encoders[perf_id]->perf_data;
    d->src = encoders[1]->cur;
    d->param = encoders[2]->cur;
    d->min = encoders[3]->cur;
  }
}

void PerfPage::loop() {
  if (show_menu) {
    perf_menu_page.loop();
    return;
  }
  update_params();
  set_led_mask();
}

void PerfPage::display() {
  if (show_menu) {
    constexpr uint8_t width = 52;
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    perf_menu_page.draw_menu(128 - width, 8, width);
    return;
  }

  oled_display.clearDisplay();

  auto oldfont = oled_display.getFont();

  mcl_gui.draw_panel_number(perf_id + 1);

  uint8_t x = mcl_gui.knob_x0 + 5;
  uint8_t y = 8;
  uint8_t lfo_height = 7;
  uint8_t width = 13;

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  mcl_gui.draw_knob_frame();

  const char *info1 = "";
  const char *info2 = "PARAMETER";

  uint8_t scene = learn - 1;

  if (page_mode > PERF_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);

    PerfEncoder *e = perf_encoders[perf_id];

    info1 = "LCK>  ";
    mcl_gui.put_value_at(page_mode, info1 + 4);

    char *str1;
    str1 = "VAL";
    uint8_t v = encoders[2]->cur;
    bool is_lock = encoders[2]->cur != 0;
    if (!is_lock) {
      str1 = "OFF";
      //Show the "non-lock" value
      uint8_t c = page_mode - 1;
      v = perf_encoders[perf_id]->perf_data.scenes[scene].params[c].val;
    }
    else {
      v -= 1;
    }

    bool show_value = mcl_gui.show_encoder_value(encoders[2]);
    mcl_gui.draw_light_encoder(MCLGUI::knob_x0 + 2 * MCLGUI::knob_w + 7, 6, v, str1, is_lock, show_value);

 }
  if (learn) {
    oled_display.fillRect(0,0,10,12, WHITE);
    oled_display.setFont(&Elektrothic);
    oled_display.setCursor(2, 10);
    oled_display.setTextColor(BLACK, WHITE);
    oled_display.print((char) (0x3C + scene));
  }
  if (page_mode == PERF_DESTINATION) {
    mcl_gui.draw_knob(0, encoders[0], "VAL");
    draw_dest(1, encoders[1]->cur, false);
    draw_param(2, encoders[1]->cur, encoders[2]->cur);
    mcl_gui.draw_knob(3, encoders[3], "MIN");
    info2 = "CONTROL";
  }

  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(oldfont);
  mcl_gui.draw_panel_labels(info1, info2);

  PerfEncoder *e = perf_encoders[perf_id];
  oled_display.setCursor(80, MCLGUI::pane_info2_y + 4);
  char *str3 = "SCENE: A -> B";
  str3[7] = 'A' + e->active_scene_a;
  str3[12] = 'A' + e->active_scene_b;
  oled_display.print(str3);

  oled_display.display();
}

void PerfPage::learn_param(uint8_t dest, uint8_t param, uint8_t value) {
  // Intercept controller param.
  PerfData *d = &perf_encoders[perf_id]->perf_data;
  if (dest + 1 == d->src && param == d->param) {
    // Controller param, start value;
    uint8_t min = d->min;
    uint8_t max = 127;
    if (value >= min) {
      uint8_t cur = value - min;
      int8_t range = max - min;
      uint8_t val = ((float)cur / (float)range) * 127.0f;
      perf_encoders[perf_id]->cur = val;
      perf_encoders[perf_id]->send_params(val);
      if (mcl.currentPage() == PERF_PAGE_0) {
        update_params();
      }
    }
  }
  if (mcl.currentPage() == PERF_PAGE_0) {

    if (learn) {
      uint8_t scene = learn - 1;
      uint8_t n = d->add_param(dest, param, scene, value);
      if (n < 255) {
         if (dest + 1 <= NUM_MD_TRACKS) {
           trig_interface.ignoreNextEvent(param - MD.currentSynthPage * 8 + 16);
         }
        page_mode = n + 1;
        config_encoders(true);
      }
    }

    // MIDI LEARN current mode;
    uint8_t a = page_mode == PERF_DESTINATION ? 1 : 0;

    if (encoders[a]->cur == 0 && encoders[a + 1]->cur > 0) {
      encoders[a]->cur = dest + 1;
      encoders[a + 1]->cur = param;
      encoders[a + 2]->cur = 0;
      update_params();
      config_encoders();
    }
  }
}

void PerfPage::send_locks(uint8_t scene) {
  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];
  uint8_t params[24];
  memset(params, 255, sizeof(params));

  for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
      PerfParam *p = &perf_encoders[perf_id]->perf_data.scenes[scene].params[n];
      uint8_t dest = p->dest;
      uint8_t param = p->param;

      if (param >= 24) {
        continue;
      }

      if (dest == last_md_track + 1) {
          params[param] = p->val;
      }
  }
  seq_step_page.disable_paramupdate_events();
  MD.activate_encoder_interface(params);
}

bool PerfPage::handleEvent(gui_event_t *event) {

  if (PerfPageParent::handleEvent(event)) {
    return true;
  }

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    uint8_t id = track / NUM_SCENES;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (track > NUM_SCENES) { return true; }
      uint8_t b = track - (track / NUM_SCENES)*NUM_SCENES;

      learn = b + 1;
      uint8_t scene = b;
      send_locks(scene);
      config_encoders();
      if (page_mode == PERF_DESTINATION) {
          //uint8_t id = perf_encoders[perf_id]->perf_data.find_empty() + 1;
          //if (id == 255) { id = 16; }
          id = 0;
          page_mode = last_page_mode == 255 ? id : last_page_mode;
          config_encoders();
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {

      if (note_interface.notes_all_off()) {
        learn = LEARN_OFF;
        seq_step_page.enable_paramupdate_events();
        MD.deactivate_encoder_interface();
        page_mode = PERF_DESTINATION;
        config_encoders();
      }
    }
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (trig_interface.is_key_down(MDX_KEY_PATSONG)) {
       return perf_menu_page.handleEvent(event);
    }
    switch (key) {
    // ENCODER BUTTONS
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17: {
      if (learn == 0) { return true; }
      uint8_t scene = learn - 1;

      uint8_t param = MD.currentSynthPage * 8 + key - 0x10;

      PerfData *d = &perf_encoders[perf_id]->perf_data;
      if (event->mask == EVENT_BUTTON_RELEASED) {
          d->clear_param_scene(last_md_track, param, scene);
      }
      if (event->mask == EVENT_BUTTON_PRESSED) {
          if (d->find_match(last_md_track, param, scene) == 255) {
            trig_interface.ignoreNextEvent(key);
            d->add_param(last_md_track, param, scene, MD.kit.params[last_md_track][param]);
          }
      }
      send_locks(scene);
      config_encoders();
      return true;
    }
    case MDX_KEY_NO: {
        char *str = "CLEAR SCENE";
        oled_display.textbox(str, "");
        MD.popup_text(str);
        for (uint8_t n = 0; n < 4; n++) {
          if (note_interface.is_note_on(n)) { perf_encoders[perf_id]->perf_data.clear_scene(n); }
        }
        config_encoders();
        break;
    }
    case MDX_KEY_YES: {
        PerfEncoder *e = perf_encoders[perf_id];
        uint8_t n;
        for (n = 0; n < 4; n++) {
          if (note_interface.is_note_on(n)) {
            if (n > 1) { e->active_scene_b = n; e->cur = 127; break; }
            else { e->active_scene_a = n; e->cur = 0; break; }
          }
        }
        e->send_params(e->cur);
      break;
    }
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    page_mode++;
    if (page_mode > 16) {
      page_mode = 0;
    }
    config_encoders();
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
     show_menu = true;
      encoders[0] = &seq_menu_value_encoder;
      encoders[1] = &seq_menu_entry_encoder;
      perf_menu_page.init();
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
     show_menu = false;
     config_encoders();
  }

  return false;
}
