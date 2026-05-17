#include "MenuPage.h"
#include "MCLGUI.h"
#include "DeviceManager.h"
#include "DevicePanelRef.h"
#include "../Drivers/MidiDevice.h"
#include "ResourceManager.h"
#include "MCLSysConfig.h"
#include "MidiSetup.h"

namespace {

#ifdef PLATFORM_TBD
bool is_grid_y_device_value(uint8_t *dest_var) {
  return dest_var == &mcl_cfg.grid_y_device;
}

uint8_t grid_y_device_to_menu_value(uint8_t stored_value) {
  switch (stored_value) {
  case GRID_Y_DEVICE_GENER:
    return 0;
  case GRID_Y_DEVICE_ELEKT:
    return 1;
  case GRID_Y_DEVICE_TBD:
    return 2;
  case GRID_Y_DEVICE_OFF:
    return 3;
  default:
    return 0;
  }
}

uint8_t grid_y_device_from_menu_value(uint8_t menu_value) {
  switch (menu_value) {
  case 0:
    return GRID_Y_DEVICE_GENER;
  case 1:
    return GRID_Y_DEVICE_ELEKT;
  case 2:
    return GRID_Y_DEVICE_TBD;
  case 3:
    return GRID_Y_DEVICE_OFF;
  default:
    return GRID_Y_DEVICE_GENER;
  }
}
#endif

#ifdef PLATFORM_TBD
uint8_t menu_value_from_stored(uint8_t *dest_var, uint8_t stored_value) {
  if (is_grid_y_device_value(dest_var)) {
    return grid_y_device_to_menu_value(stored_value);
  }
  return stored_value;
}

uint8_t stored_value_from_menu(uint8_t *dest_var, uint8_t menu_value) {
  if (is_grid_y_device_value(dest_var)) {
    return grid_y_device_from_menu_value(menu_value);
  }
  return menu_value;
}
#endif

} // namespace

void MenuPageBase::init() {
  init(true);
}

void MenuPageBase::init(bool generate_row_names) {
  DEBUG_PRINTLN("MenuPageBase::init");
  DevicePanelRef::set_primary_key_repeat(1);
  R.Clear();
  R.use_machine_names_short(); // for grid page
  R.use_icons_knob(); // for grid page
  R.use_menu_options();
  R.use_menu_layouts();
  DEBUG_PRINT("R.Size() = ");
  DEBUG_PRINTLN(R.Size());
  R.restore_menu_layout_deps();
  if (generate_row_names) {
    gen_menu_row_names();
  }

  MenuBase *m = get_menu();
  encoders[1]->cur = selected_item;
  ((MCLEncoder *)encoders[1])->max = m->get_number_of_items() - 1;

  if (((MCLEncoder *)encoders[1])->cur > ((MCLEncoder *)encoders[1])->max) {
    ((MCLEncoder *)encoders[1])->cur = 0;
    cur_row = 0;
  }
  selected_item = encoders[1]->cur;

  uint8_t range = m->get_option_range(encoders[1]->cur);
  ((MCLEncoder *)encoders[0])->max = range > 0 ? range - 1 : 0;
  ((MCLEncoder *)encoders[0])->min =
      m->get_option_min(encoders[1]->cur);

  uint8_t *dest_var = m->get_dest_variable(encoders[1]->cur);
  if (dest_var != NULL) {
#ifdef PLATFORM_TBD
    encoders[0]->setValue(menu_value_from_stored(dest_var, *dest_var));
#else
    encoders[0]->setValue(*dest_var);
#endif
  }
  encoders[0]->old = encoders[0]->cur;
  encoders[1]->old = encoders[1]->cur;
}

void MenuPageBase::gen_menu_device_names() {
  MenuBase *m = get_menu();
  menu_option_t *p =
      (menu_option_t *)R.Allocate(sizeof(menu_option_t) * NUM_DEVS);
  m->set_custom_options(p,0);

  MidiDevice *devs[] = { device_manager.primary_device(), device_manager.secondary_device() };
  for (uint8_t n = 0; n < NUM_DEVS; n++) {
    p->pos = n;
#ifdef PLATFORM_TBD
    if (devs[0] == devs[1]) {
      if (strcmp(devs[n]->name, "TBD") == 0) {
        strcpy(p->name, n == 0 ? "TB1" : "TB2");
      } else {
        strncpy(p->name, devs[n]->name, sizeof(p->name) - 3);
        p->name[sizeof(p->name) - 3] = '\0';
        strncat(p->name, n == 0 ? ":1" : ":2",
                sizeof(p->name) - strlen(p->name) - 1);
      }
    } else {
#endif
      strcpy(p->name, devs[n]->name);
#ifdef PLATFORM_TBD
    }
#endif
    p++;
  }
}

void MenuPageBase::gen_menu_transpose_names() {
  MenuBase *m = get_menu();
  menu_option_t *p = (menu_option_t *)R.Allocate(sizeof(menu_option_t) * 50);
  m->set_custom_options(p,1);

  for (int8_t i = -12; i <= 12; ++i) {
    // Generate both regular and ALL entries in same loop
    for (uint8_t all = 0; all < 2; ++all) {
      p->pos = i + (all ? 37 : 12); // 0 transpose at index 12

      uint8_t idx = 0;
      if (all) {
        p->name[idx++] = 'A';
        p->name[idx++] = 'L';
        p->name[idx++] = 'L';
        p->name[idx++] = ' ';
      }

      p->name[idx++] = (i < 0) ? '-' : '+';
      uint8_t num = abs(i);
      if (num >= 10) {
        p->name[idx++] = '1';
        num -= 10;
      }
      p->name[idx++] = '0' + num;
      p->name[idx] = '\0';

      ++p;
    }
  }

}

void MenuPageBase::gen_menu_row_names() {
  MenuBase *m = get_menu();
  menu_option_t *p = (menu_option_t *)R.Allocate(sizeof(menu_option_t) * 128);
  m->set_custom_options(p,0);
  for (uint8_t row_id = 0; row_id < 128; ++row_id) {
    char bank = 'A' + row_id / 16;
    uint8_t i = row_id % 16 + 1;

    p->pos = row_id;
    p->name[0] = bank;

    if (i < 10) {
      p->name[1] = '0';
      p->name[2] = '0' + i;
    } else {
      p->name[1] = '1';
      p->name[2] = '0' + i - 10;
    }

    p->name[3] = '\0';
    ++p;
  }
}

void MenuPageBase::cleanup() {
  selected_item = encoders[1]->cur;
  key_interface.ignoreNextEventClear(MDX_KEY_YES);
  key_interface.ignoreNextEventClear(MDX_KEY_NO);
}

void MenuPageBase::loop() {
  MenuBase *m = get_menu();

  if (encoders[1]->hasChanged()) {
    uint8_t range = m->get_option_range(encoders[1]->cur);
    ((MCLEncoder *)encoders[0])->max = range > 0 ? range - 1 : 0;
    ((MCLEncoder *)encoders[0])->min =
        m->get_option_min(encoders[1]->cur);

    uint8_t diff = encoders[1]->cur - encoders[1]->old;
    int8_t new_val = cur_row + diff;
    if (new_val > visible_rows - 1) {
      new_val = visible_rows - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
    selected_item = encoders[1]->cur;
    uint8_t *dest_var = m->get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
#ifdef PLATFORM_TBD
      encoders[0]->setValue(menu_value_from_stored(dest_var, *dest_var));
#else
      encoders[0]->setValue(*dest_var);
#endif
    } else {
      encoders[0]->setValue(0);
    }
  }
  if (encoders[0]->hasChanged()) {
    uint8_t *dest_var = m->get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
#ifdef PLATFORM_TBD
      *dest_var = stored_value_from_menu(dest_var, encoders[0]->cur);
#else
      *dest_var = encoders[0]->cur;
#endif
    }
  }
}

void MenuPageBase::draw_scrollbar(uint8_t x_offset) {
  mcl_gui.draw_vertical_scrollbar(x_offset, get_menu()->get_number_of_items(),
                                  visible_rows, encoders[1]->cur - cur_row);
}

void MenuPageBase::draw_item(MenuBase *m, uint8_t item_n,
                             uint8_t number_of_items) {
  const char *name = m->get_item_name(item_n);
  if (name != nullptr) {
    oled_display.print(name);
  }

  if (item_n > number_of_items - 1) {
    return;
  }

  if (m->get_option_range(item_n) > 0) {

    mcl_print_P(mclstr_space);
    uint8_t *pdest = m->get_dest_variable(item_n);
    uint8_t option_value = *pdest;
#ifdef PLATFORM_TBD
    option_value = menu_value_from_stored(pdest, option_value);
#endif
    const char *option_name = m->get_option_name(item_n, option_value);
    if (option_name == NULL) {
      oled_display.println(option_value);
    } else {
      oled_display.println(option_name);
    }
  }
}

void MenuPageBase::draw_menu(uint8_t x_offset, uint8_t y_offset,
                             uint8_t width, uint8_t scrollbar_width) {
  oled_display.setCursor(x_offset, y_offset);
  MenuBase *m = get_menu();
  uint8_t number_of_items = m->get_number_of_items();
  bool show_scrollbar = scrollbar_width > 0 && number_of_items > visible_rows;
  uint8_t item_width = width;
  if (!show_scrollbar && scrollbar_width > 0) {
    item_width += scrollbar_width + 3;
  }
  uint8_t max_items;
  if (number_of_items > visible_rows) {
    max_items = visible_rows;
  } else {
    max_items = number_of_items;
  }
  for (uint8_t n = 0; n < max_items; n++) {

    oled_display.setCursor(x_offset, y_offset + 8 * n);
    if (n == cur_row) {
      oled_display.setTextColor(BLACK, WHITE);
      oled_display.fillRect(max(0, oled_display.getCursorX() - 3),
                            max(0, oled_display.getCursorY() - 6),
                            item_width, 7, WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
    }
    draw_item(m, encoders[1]->cur - cur_row + n, number_of_items);
  }

  if (show_scrollbar) {
    uint8_t first_row = encoders[1]->cur - cur_row;
    uint8_t bar_x = x_offset + item_width;
    uint8_t bar_y = y_offset - 6;
    uint8_t bar_h = visible_rows * 8 - 1;
    uint8_t thumb_h = ((uint16_t)visible_rows * bar_h) / number_of_items;
    if (thumb_h < 3) {
      thumb_h = 3;
    }
    uint8_t travel = bar_h - thumb_h;
    uint8_t max_first = number_of_items - visible_rows;
    uint8_t thumb_y = bar_y + ((uint16_t)first_row * travel) / max_first;
    oled_display.fillRect(bar_x, bar_y, scrollbar_width, bar_h, BLACK);
    oled_display.fillRect(bar_x, thumb_y, scrollbar_width, thumb_h, WHITE);
  }

  // draw_item.read(getRow());

  oled_display.setTextColor(WHITE, BLACK);
}

void MenuPageBase::display() {

  MenuBase *m = get_menu();
  uint8_t number_of_items = m->get_number_of_items();
  uint8_t x_offset = 43;
  oled_display.clearDisplay();
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.println(m->get_name());
  mcl_gui.draw_vertical_dashline(x_offset - 6);

  draw_menu(x_offset, 8);

  if (number_of_items > visible_rows) {
    draw_scrollbar(120);
  }
}

bool MenuPageBase::enter() {
  DEBUG_PRINT_FN();
  MenuBase *m = get_menu();
  void (*row_func)() = m->get_row_function(encoders[1]->cur);
  PageIndex page_callback = m->get_page_callback(encoders[1]->cur);
  if (page_callback != NULL_PAGE) {
    DEBUG_PRINTLN("menu pushing page");
    DEBUG_PRINTLN((uint16_t)page_callback);
    mcl.pushPage(page_callback);
    return true;
  }
  if (row_func != NULL) {
    DEBUG_PRINTLN(F("calling callback func"));
    (*row_func)();
    return true;
  }
  return false;
}

void MenuPageBase::exit() {
  if (GUI.currentPage() != this) {
    return;
  }
  MenuBase *m = get_menu();
  void (*exit_func)() = m->get_exit_function();
  if (exit_func != NULL) {
    (*exit_func)();
  }
  mcl.popPage();
}

bool MenuPageBase::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {

    return true;
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t inc = 1;
      if (key_interface.is_key_down(MDX_KEY_FUNC)) {
        inc = 8;
      }
      switch (key) {
      case MDX_KEY_YES:
        key_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES;
      case MDX_KEY_NO:
        key_interface.ignoreNextEvent(MDX_KEY_NO);
        goto NO;
      case MDX_KEY_UP:
        encoders[1]->cur -= inc;
        break;
      case MDX_KEY_DOWN:
        encoders[1]->cur += inc;
        break;
      case MDX_KEY_LEFT:
        encoders[0]->cur -= inc;
        break;
      case MDX_KEY_RIGHT:
        encoders[0]->cur += inc;
        break;
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      GUI.ignoreNextEvent(event->source);
    YES:
      DEBUG_PRINTLN("YES");
      enter();
      return true;
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      GUI.ignoreNextEvent(event->source);
    NO:
      exit();
      return true;
    }
  }
  return false;
}
