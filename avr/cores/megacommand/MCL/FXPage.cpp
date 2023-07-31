#include "MCL_impl.h"
#include "ResourceManager.h"

#define FX_TYPE 0
#define FX_PARAM 1

PageIndex FXPage::last_page = FX_PAGE_A;

void FXPage::setup() { DEBUG_PRINT_FN(); }

void FXPage::init() {
  DEBUG_PRINT_FN();
  oled_display.clearDisplay();
  oled_display.setFont();
  trig_interface.off();
  update_encoders();
  MD.set_key_repeat(0);
  R.Clear();
  R.use_icons_page();
  R.use_machine_param_names();
  R.use_icons_knob();
  last_page = mcl.currentPage();
}
void FXPage::update_encoders() {

  for (uint8_t n = 0; n < GUI_NUM_ENCODERS; n++) {
    ((MCLEncoder *)encoders[n])->max = 127;

    uint8_t a = ((uint8_t)page_mode * GUI_NUM_ENCODERS) + n;
    uint8_t fx_param = params[a].param;

    switch (params[a].type) {
    case MD_FX_ECHO:
      encoders[n]->cur = MD.kit.delay[fx_param];
      break;
    case MD_FX_REV:
      encoders[n]->cur = MD.kit.reverb[fx_param];
      break;
    case MD_FX_EQ:
      encoders[n]->cur = MD.kit.eq[fx_param];
      break;
    case MD_FX_DYN:
      encoders[n]->cur = MD.kit.dynamics[fx_param];
      break;
    }
    encoders[n]->old = encoders[n]->cur;
  }

  init_encoders_used_clock();
}

void FXPage::cleanup() {
  //  md_exploit.off();
  MD.set_key_repeat(1);
  oled_display.clearDisplay();
}

void FXPage::loop() {

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    uint8_t n = i + (page_mode * GUI_NUM_ENCODERS);

    if (encoders[i]->hasChanged()) {
      uint8_t fx_param = params[n].param;
      uint8_t fx_type = params[n].type;

      uint8_t val;
      MD.sendFXParam(fx_param, encoders[i]->cur, fx_type);
      switch (fx_type) {
      case MD_FX_ECHO:
        MD.kit.delay[fx_param] = encoders[i]->cur;
        break;
      case MD_FX_DYN:
        MD.kit.dynamics[fx_param] = encoders[i]->cur;
        break;
      case MD_FX_REV:
        MD.kit.reverb[fx_param] = encoders[i]->cur;
        break;
      case MD_FX_EQ:
        MD.kit.eq[fx_param] = encoders[i]->cur;
        break;
      }
    }
  }
}
void FXPage::display() {

  char str[4];
  PGM_P param_name = NULL;
  oled_display.clearDisplay();
  auto oldfont = oled_display.getFont();

  uint8_t *icon = R.icons_page->icon_rhytmecho;
  if (page_id == 1) {
    icon = R.icons_page->icon_gatebox;
  }

  oled_display.drawBitmap(0, 0, icon, 24, 18, WHITE);
  mcl_gui.draw_knob_frame();

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    uint8_t n = i + ((page_mode ? 1 : 0) * GUI_NUM_ENCODERS);

    uint8_t fx_param = params[n].param;
    uint8_t fx_type = params[n].type;
    param_name = fx_param_name(fx_type, fx_param);
    strncpy(str, param_name, 4);

    mcl_gui.draw_knob(i, encoders[i], str);
    //  mcl_gui.draw_light_encoder(30 + 20 * i, 18, encoders[i], str);
  }
  oled_display.setFont(&TomThumb);
  const char *info1;;
  const char *info2;
  if (page_mode) {
    info1 = "FX A";
   } else {
     info1 = "FX B";
  }
  info2 = &fx_page_title[0];
  mcl_gui.draw_panel_labels(info1, info2);
  oled_display.display();
  oled_display.setFont(oldfont);
}

bool FXPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port)->id != DEVICE_MD) {
      return true;
    }
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
        case MDX_KEY_NO:
        mcl.setPage(MIXER_PAGE);
        break;
      }
    }
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
        case MDX_KEY_SCALE:
        case MDX_KEY_DOWN:
        if (mcl.currentPage() == FX_PAGE_B) {
          goto toggle_mode;
        }
        else {
          mcl.setPage(FX_PAGE_B);
        }
        break;
        case MDX_KEY_LEFT:
        if (mcl.currentPage() == FX_PAGE_A) {
          goto toggle_mode;
        }
        else {
          mcl.setPage(FX_PAGE_A);
        }
        break;
      }
    }
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
      //mcl.setPage(GRID_PAGE);
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    toggle_mode:
    page_mode = !(page_mode);
    update_encoders();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mcl.setPage(PAGE_SELECT_PAGE);
    return true;
  }

  return false;
}
