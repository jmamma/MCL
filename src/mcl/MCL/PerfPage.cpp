#include "PerfPage.h"
#include "DeviceManager.h"
#include "DeviceParamResolver.h"
#include "CommonPages.h"
#include "MCLMemory.h"
#include "MCLGUI.h"
#include "MCLClipBoard.h"
#include "SeqPages.h"
#include "MCLStrings.h"

#define LEARN_MIN 1
#define LEARN_MAX 2
#define LEARN_OFF 0

static constexpr uint8_t PERF_PARAM_EDITOR_PARAM_COUNT = 24;

namespace {

uint8_t perf_editor_dest() {
#if defined(__AVR__)
  return last_md_track + 1;
#else
  uint8_t dest = DeviceParamResolver::perf_dest_from_idx(DeviceIdx::Primary,
                                                          last_md_track + 1);
  return dest == 255 ? 0 : dest + 1;
#endif
}

} // namespace

void PerfPage::setup() {
  DEBUG_PRINT_FN();
  page_mode = PERF_DESTINATION;
  perf_param1.init();
  perf_param2.init();
  perf_param3.init();
  perf_param4.init();

  perf_encoders[0] = &perf_param1;
  perf_encoders[1] = &perf_param2;
  perf_encoders[2] = &perf_param3;
  perf_encoders[3] = &perf_param4;

  perf_param2.active_scene_a = 2;
  perf_param2.active_scene_b = 3;
  perf_param3.active_scene_a = 4;
  perf_param3.active_scene_b = 5;
  perf_param4.active_scene_a = 6;
  perf_param4.active_scene_b = 7;

  isSetup = true;
}

void PerfPage::init() {
  DEBUG_PRINT_FN();
  PerfPageParent::init();
  key_interface.on();
  last_mask = last_blink_mask = 0;
  show_menu = false;
  last_page_mode = 255;
  DeviceParamResolver::set_perf_rec_mode(3);
  config_encoders();
}

void PerfPage::set_led_mask() {
  uint16_t mask = 0;

  PerfEncoder *e = perf_encoders[perf_id];

  if (show_menu) {
    SET_BIT16(mask, perf_id);
  } else {
    if (e->active_scene_a < NUM_SCENES) {
      SET_BIT16(mask, e->active_scene_a);
    }
    if (e->active_scene_b < NUM_SCENES) {
      SET_BIT16(mask, e->active_scene_b);
    }
  }
  if (last_mask != mask) {
    mcl_gui.set_trigleds(mask, TRIGLED_EXCLUSIVENDYNAMIC);
  }
  if (show_menu) {
    last_mask = mask;
    return;
  }
  uint16_t blink_mask = 0;

  blink_mask |= e->perf_data.get_active_scene_mask();
  blink_mask &= ~mask;

  if (last_blink_mask != blink_mask) {
    mcl_gui.set_trigleds(blink_mask, TRIGLED_EXCLUSIVENDYNAMIC, true);
  }

  last_blink_mask = blink_mask;
  last_mask = mask;
}

void PerfPage::cleanup() {
  PerfPageParent::cleanup();
  key_interface.off();
  DeviceParamResolver::set_perf_rec_mode(0);
}
void PerfPage::func_enc_check() {
  if (key_interface.is_key_down(MDX_KEY_FUNC)) {
    for (uint8_t n = 0; n < 4; n++) {
      PerfEncoder *e = perf_encoders[n];
      if (e->hasChanged()) {
        e->cur = e->cur < e->old ? 0 : 127;
        e->old = e->cur;
        e->send();
      }
    }
  }
}
void PerfPage::config_encoder_range(uint8_t i) {
  ((MCLEncoder *)encoders[i])->max = DeviceParamResolver::perf_target_count();
  ((MCLEncoder *)encoders[i + 1])->min = 0;

  uint8_t param_count =
      DeviceParamResolver::perf(encoders[i]->cur).param_count();
  ((MCLEncoder *)encoders[i + 1])->max = param_count ? param_count - 1 : 0;
}

void PerfPage::config_encoders(uint8_t show_val) {

  encoders[0] = perf_encoders[perf_id];
  encoders[1] = &fx_param2;
  encoders[2] = &fx_param3;
  encoders[3] = &fx_param4;

  if (page_mode == PERF_DESTINATION) {

    PerfData *d = &perf_encoders[perf_id]->perf_data;
    encoders[1]->cur = d->src;
    encoders[2]->cur = d->param;
    config_encoder_range(1);
    encoders[3]->cur = d->min;
    ((MCLEncoder *)encoders[3])->max = 127;
  } else if (learn) {
    uint8_t scene = learn - 1;

    last_page_mode = page_mode;

    uint8_t c = page_mode - 1;

    PerfEncoder *e = perf_encoders[perf_id];
    PerfParam *p = &e->perf_data.scenes[scene].params[c];

    encoders[1]->cur = p->dest;
    encoders[2]->cur = p->param;

    uint8_t v = p->val;

    if (v == 255) {
      v = 0;
    } else {
      v++;
    }

    encoders[3]->cur = v;
    ((MCLEncoder *)encoders[3])->max = 128;

    config_encoder_range(1);
  }

  if (!show_val) {
    init_encoders_used_clock();
  }
}
void PerfPage::update_params() {
  if (show_menu) {
    return;
  }

  config_encoder_range(1);

  if (encoders[1]->hasChanged() && encoders[1]->cur == 0) {
    encoders[2]->cur = 0;
  }

  if (page_mode > PERF_DESTINATION) {
    if (learn) {
      uint8_t c = page_mode - 1;
      uint8_t scene = learn - 1;

      PerfParam *p = &perf_encoders[perf_id]->perf_data.scenes[scene].params[c];
      p->dest = encoders[1]->cur;
      p->param = encoders[2]->cur;
      if (encoders[3]->hasChanged() && BUTTON_DOWN(Buttons.ENCODER4)) { GUI.ignoreNextEvent(Buttons.ENCODER4); }
      if (encoders[3]->cur > 0) {
        if (p->val == 255) { perf_encoders[perf_id]->perf_data.scenes[scene].count++; }
        p->val = encoders[3]->cur - 1;
      }
    }
  } else {
    if (encoders[1]->hasChanged() || encoders[2]->hasChanged() || encoders[3]->hasChanged()) {
      PerfData *d = &perf_encoders[perf_id]->perf_data;
      d->update_src(encoders[1]->cur, encoders[2]->cur, encoders[3]->cur);
    }
  }
}

void PerfPage::loop() {
  set_led_mask();
  if (show_menu) {
    perf_menu_page.loop();
    return;
  }
  func_enc_check();
  encoder_send();
  update_params();
}

void PerfPage::display() {
  oled_display.setFont(&TomThumb);
  if (show_menu) {
    constexpr uint8_t width = 52;
    oled_display.fillRect(128 - width - 2, 0, width + 2, 32, BLACK);
    perf_menu_page.draw_menu(128 - width, 8, width);
    return;
  }

  oled_display.clearDisplay();

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  mcl_gui.draw_knob_frame();

  char info1[8] = ""; // Use an appropriate size for your needs
  char info2[12]; // Use an appropriate size for your needs
  mclstr_copy_progmem(info2, mclstr_parameter, sizeof(info2));

  PerfEncoder *e = perf_encoders[perf_id];

  static char perf_label[4] = " A";
  perf_label[1] = 'A' + perf_id;
  mcl_gui.draw_knob(0, encoders[0], perf_label, false, false);

  if (learn) {
    draw_dest(1, encoders[1]->cur);
    draw_param(2, encoders[1]->cur, encoders[2]->cur);
    mclstr_copy_progmem(info1, mclstr_lck_arrow, sizeof(info1));
    mcl_gui.put_value_at(page_mode, info1 + 4);

    const char *lock_label = mclstr_lck;
    uint8_t v = encoders[3]->cur;
    bool is_lock = encoders[3]->cur != 0;
    if (!is_lock) {
      lock_label = mclstr_off;
      // Show the "non-lock" value
      v = 0; //perf_encoders[perf_id]->perf_data.scenes[scene].params[c].val;
    } else {
      v -= 1;
    }

    bool show_value = mcl_gui.show_encoder_value(encoders[3]);
    mcl_gui.draw_light_encoder(MCLGUI::knob_x0 + 3 * MCLGUI::knob_w + 7, 6, v,
                               lock_label, is_lock, show_value);

  } else {
    draw_dest(1, encoders[1]->cur, false);
    draw_param(2, encoders[1]->cur, encoders[2]->cur);
    mcl_gui.draw_knob(3, encoders[3], mclstr_thr);
    strcpy(info2, e->name);
  }

  // oled_display.fillCircle(6, 6, 6, WHITE);
  oled_display.fillRect(0, 0, 10, 12, WHITE);
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(2, 10);
  // oled_display.setCursor(4, 10);
  oled_display.setTextColor(BLACK, WHITE);
  oled_display.print((char)(0x3C + perf_id));

  if (learn) {
    mcl_gui.draw_panel_number(learn);
  }
  oled_display.setTextColor(WHITE, BLACK);
  mcl_gui.draw_panel_labels(info1, info2);

  if (key_interface.is_key_down(MDX_KEY_LEFT) ||
      key_interface.is_key_down(MDX_KEY_RIGHT)) {
    oled_display.setCursor(54, MCLGUI::pane_info2_y + 4);
    mcl_print_P(mclstr_select_label);
  }

  oled_display.setCursor(80, MCLGUI::pane_info2_y + 4);
  static char scene_label[] = "SCENE: A    B";
  scene_label[7] = e->active_scene_a == 255 ? '-' : '1' + e->active_scene_a;
  scene_label[12] = e->active_scene_b == 255 ? '-' : '1' + e->active_scene_b;
  oled_display.print(scene_label);
  oled_display.writeFastHLine(109, MCLGUI::pane_info2_y + 1, 5, WHITE);
  oled_display.writeFastVLine(109 + ((e->cur * 5) / 128), MCLGUI::pane_info2_y,
                              3, WHITE);
}


void PerfPage::encoder_check() {
  if (!isSetup || GUI.currentPage() == this || mcl.currentPage() == MIXER_PAGE) return;
  encoder_send();
}

void PerfPage::encoder_send() {
  for (uint8_t i = 0; i < 4; i++) {
    if (perf_encoders[i]->hasChanged() || perf_encoders[i]->resend) { perf_encoders[i]->send(); }
  }
}

void PerfPage::learn_param(uint8_t dest, uint8_t param, uint8_t value) {
  uint8_t perf_dest = dest + 1;
  DevicePerfTarget target = DeviceParamResolver::perf(perf_dest);
  if (perf_dest == 0 || param >= target.param_count()) {
    return;
  }

  for (uint8_t i = 0; i < 4; i++) {
    PerfEncoder *e = perf_encoders[i];
    PerfData *d = &e->perf_data;
    if (dest + 1 == d->src && param == d->param) {
      // Controller param, start value;
      uint8_t min = d->min;
      uint8_t max = 127;
      if (value >= min) {
        int16_t cur = value - min;
        int16_t range = max - min;
        uint8_t val = (cur * (int16_t)127) / range;
        e->cur = val;
        //perf_encoders[i]->send();
        if (mcl.currentPage() == PERF_PAGE_0) {
          update_params();
        }
      }
    }
  }

  if (mcl.currentPage() == PERF_PAGE_0) {
    if (learn) {
      PerfData *d = &perf_encoders[perf_id]->perf_data;
      uint8_t scene = learn - 1;
      uint8_t n = d->add_param(dest, param, scene, value);
      if (n < 255) {
        uint8_t key = 0;
        if (target.key_for_param(param, &key)) {
          key_interface.ignoreNextEvent(key);
        }
        page_mode = n + 1;
        undo = 255;
        config_encoders(true);
      }
    }

    // MIDI LEARN current mode;
    uint8_t a = 1;

    if (encoders[a]->cur == 0 && encoders[a + 1]->cur > 0) {
      encoders[a]->cur = dest + 1;
      encoders[a + 1]->cur = param;
      encoders[a + 2]->cur = 0;
      update_params();
      config_encoders();
    }
  }
}

void rename_perf() {
  const char *my_title = "PerfCtrl Name:";

  mcl_gui.wait_for_input(perf_page.perf_encoders[perf_page.perf_id]->name,
                         my_title, 8);
}

void PerfPage::send_locks(uint8_t scene) {
  uint8_t editor_dest = perf_editor_dest();
  if (editor_dest == 0) {
    return;
  }

  uint8_t params[PERF_PARAM_EDITOR_PARAM_COUNT];
  memset(params, 255, sizeof(params));

  for (uint8_t n = 0; n < NUM_PERF_PARAMS; n++) {
    PerfParam *p = &perf_encoders[perf_id]->perf_data.scenes[scene].params[n];
    uint8_t dest = p->dest;
    uint8_t param = p->param;

    if (param >= sizeof(params)) {
      continue;
    }

    if (dest == editor_dest) {
      params[param] = p->val;
    }
  }
  if (DeviceParamResolver::perf(editor_dest)
          .begin_param_editor(params, sizeof(params))) {
    seq_step_page.disable_paramupdate_events();
  }
}

bool PerfPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t track = event->source;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (track >= NUM_SCENES) {
        return true;
      }

      if (show_menu) {
        if (track < 4) {
          perf_id = track;
        }
        return true;
      }

      PerfEncoder *e = perf_encoders[perf_id];

      if (key_interface.is_key_down(MDX_KEY_LEFT)) {
        e->active_scene_a = e->active_scene_a == track ? 255 : track;
        return true;
      }
      if (key_interface.is_key_down(MDX_KEY_RIGHT)) {
        e->active_scene_b = e->active_scene_b == track ? 255 : track;
        return true;
      }

      learn = track + 1;
      uint8_t scene = track;
      send_locks(scene);
      config_encoders();
      if (page_mode == PERF_DESTINATION) {
        page_mode = last_page_mode == 255 ? 1 : last_page_mode;
        config_encoders();
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (show_menu) {
        return true;
      }
      if (note_interface.notes_all_off()) {
        learn = LEARN_OFF;
        seq_step_page.enable_paramupdate_events();
        DeviceParamResolver::end_perf_param_editor();
        page_mode = PERF_DESTINATION;
        config_encoders();
      }
    }
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (key_interface.is_key_down(MDX_KEY_PATSONG)) {
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
      if (learn == 0) {
        return true;
      }
      uint8_t scene = learn - 1;

      uint8_t editor_dest = perf_editor_dest();
      if (editor_dest == 0) {
        return true;
      }

      uint8_t param = 0;
      DevicePerfTarget editor_target = DeviceParamResolver::perf(editor_dest);
      if (!editor_target.param_from_key(key, &param)) {
        return true;
      }
      uint8_t data_dest = editor_dest - 1;

      PerfData *d = &perf_encoders[perf_id]->perf_data;
      if (event->mask == EVENT_BUTTON_RELEASED) {
        d->clear_param_scene(data_dest, param, scene);
      }
      if (event->mask == EVENT_BUTTON_PRESSED) {
        if (d->find_match(data_dest, param, scene) == 255) {
          key_interface.ignoreNextEvent(key);
          uint8_t value = 0;
          if (editor_target.get_param(param, &value)) {
            d->add_param(data_dest, param, scene, value);
          }
        }
      }
      send_locks(scene);
      config_encoders();
      return true;
    }
    }
    if (event->mask == EVENT_BUTTON_PRESSED) {

      uint8_t t = note_interface.get_first_md_note();
      if (t < NUM_SCENES) {
        switch (key) {
        case MDX_KEY_COPY: {
          oled_display.textbox_P(mclstr_copy, mclstr_scene);
          mcl_clipboard.copy_scene(
              &perf_encoders[perf_id]->perf_data.scenes[t]);
          undo = 255;
          return true;
        }
        case MDX_KEY_PASTE: {
          if (undo < NUM_SCENES) {
            return true;
          }
          if (mcl_clipboard.paste_scene(
                  &perf_encoders[perf_id]->perf_data.scenes[t])) {
            oled_display.textbox_P(mclstr_paste, mclstr_scene);
            config_encoders();
            send_locks(t);
          }
          return true;
        }
        case MDX_KEY_CLEAR: {
          if (t == undo) {
            if (mcl_clipboard.paste_scene(
                    &perf_encoders[perf_id]->perf_data.scenes[undo])) {
              oled_display.textbox_P(mclstr_undo, mclstr_clear);
              undo = 255;
              goto end_clear;
            }
            undo = 255;
            return true;
          } else {
            undo = t;
            mcl_clipboard.copy_scene(
                &perf_encoders[perf_id]->perf_data.scenes[t]);
          }
          oled_display.textbox_P(mclstr_clear, mclstr_scene);
          perf_encoders[perf_id]->perf_data.clear_scene(t);
        end_clear:
          config_encoders();
          send_locks(t);
          return true;
        }
        case MDX_KEY_YES: {
          PerfEncoder *e = perf_encoders[perf_id];
          if (t >= NUM_SCENES) {
            return true;
          }
          PerfScene *s1 = &e->perf_data.scenes[t], *s2 = nullptr;
          e->send_params(0, s1, s2);
          return true;
        }
        }
      }
      switch (key) {
      case MDX_KEY_UP: {
        if (learn && page_mode < NUM_PERF_PARAMS) {
          page_mode++;
          config_encoders();
        }
        return true;
      }
      case MDX_KEY_DOWN: {
        if (learn && page_mode > 1) {
          page_mode--;
          config_encoders();
        }
        return true;
      }
      }
    }
    return false;
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.ENCODER4)) {
      // if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
      if (learn) {
        PerfData *d = &perf_encoders[perf_id]->perf_data;
        uint8_t scene = learn - 1;
        if (encoders[1]->cur != 0 && encoders[2]->cur != 255) {
          d->clear_param_scene(encoders[1]->cur - 1, encoders[2]->cur, scene);
          config_encoders();
        }
      }
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      page_mode++;
      if (page_mode > NUM_PERF_PARAMS) {
        page_mode = 0;
      }
      config_encoders();
      return true;
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      show_menu = true;
      encoders[0] = &seq_menu_value_encoder;
      encoders[1] = &seq_menu_entry_encoder;
      perf_menu_page.init();
      return true;
    }
    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
      if (show_menu) {
        void (*row_func)() = perf_menu_page.menu.get_row_function(
            perf_menu_page.encoders[1]->cur);
        if (row_func != NULL) {
          DEBUG_PRINTLN("calling menu func");
          (*row_func)();
        }
        init();
      }
      return true;
    }
  }

  return false;
}
