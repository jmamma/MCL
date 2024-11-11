#include "MCL_impl.h"
#include "ResourceManager.h"

void MenuPageBase::init() {
  DEBUG_PRINTLN("MenuPageBase::init");
  R.Clear();
  R.use_machine_names_short(); // for grid page
  R.use_icons_knob(); // for grid page
  R.use_menu_options();
  R.use_menu_layouts();
  DEBUG_PRINT("R.Size() = ");
  DEBUG_PRINTLN(R.Size());
  R.restore_menu_layout_deps();
  gen_menu_row_names();

  ((MCLEncoder *)encoders[1])->max = get_menu()->get_number_of_items() - 1;

  if (((MCLEncoder *)encoders[1])->cur > ((MCLEncoder *)encoders[1])->max) {
    ((MCLEncoder *)encoders[1])->cur = 0;
    cur_row = 0;
  }

  ((MCLEncoder *)encoders[0])->max =
      get_menu()->get_option_range(encoders[1]->cur) - 1;
  ((MCLEncoder *)encoders[0])->min =
      get_menu()->get_option_min(encoders[1]->cur);

  uint8_t *dest_var = get_menu()->get_dest_variable(encoders[1]->cur);
  if (dest_var != NULL) {
    encoders[0]->setValue(*dest_var);
  }
  encoders[0]->old = encoders[0]->cur;
  encoders[1]->old = encoders[1]->cur;
}

void MenuPageBase::gen_menu_device_names() {
  MenuBase *m = get_menu();
  menu_option_t *p =
      (menu_option_t *)R.Allocate(sizeof(menu_option_t) * NUM_DEVS);
  m->set_custom_options(p);

  for (uint8_t n = 0; n < NUM_DEVS; n++) {
    p->pos = n + 1;
    strcpy(p->name, midi_active_peering.get_device(n + 1)->name);
    p++;
  }
}

void MenuPageBase::gen_menu_row_names() {
  MenuBase *m = get_menu();
  menu_option_t *p = (menu_option_t *)R.Allocate(sizeof(menu_option_t) * 128);
  m->set_custom_options(p);
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

void MenuPageBase::setup() {}

void MenuPageBase::cleanup() {
  trig_interface.ignoreNextEventClear(MDX_KEY_YES);
  trig_interface.ignoreNextEventClear(MDX_KEY_NO);
}

void MenuPageBase::loop() {

  if (encoders[1]->hasChanged()) {
    ((MCLEncoder *)encoders[0])->max =
        get_menu()->get_option_range(encoders[1]->cur) - 1;
    ((MCLEncoder *)encoders[0])->min =
        get_menu()->get_option_min(encoders[1]->cur);

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
    uint8_t *dest_var = get_menu()->get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
      encoders[0]->setValue(*dest_var);
    } else {
      encoders[0]->setValue(0);
    }
  }
  if (encoders[0]->hasChanged()) {
    uint8_t *dest_var = get_menu()->get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
      *dest_var = encoders[0]->cur;
    }
  }
}

void MenuPageBase::draw_scrollbar(uint8_t x_offset) {
  mcl_gui.draw_vertical_scrollbar(x_offset, get_menu()->get_number_of_items(),
                                  visible_rows, encoders[1]->cur - cur_row);
}

void MenuPageBase::draw_item(uint8_t item_n, uint8_t row) {
  const char *name = get_menu()->get_item_name(item_n);
  if (name != nullptr) {
    oled_display.print(name);
  }
  uint8_t number_of_items = get_menu()->get_number_of_items();

  if (item_n > number_of_items - 1) {
    return;
  }

  uint8_t number_of_options = get_menu()->get_number_of_options(item_n);
  if (get_menu()->get_option_range(item_n) > 0) {

    oled_display.print(F(" "));
    uint8_t *pdest = get_menu()->get_dest_variable(item_n);
    const char *option_name = get_menu()->get_option_name(item_n, *pdest);
    if (option_name == NULL) {
      oled_display.println(*pdest);
    } else {
      oled_display.println(option_name);
    }
  }
}

void MenuPageBase::draw_menu(uint8_t x_offset, uint8_t y_offset,
                             uint8_t width) {
  oled_display.setCursor(x_offset, y_offset);
  uint8_t number_of_items = get_menu()->get_number_of_items();
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
                            max(0, oled_display.getCursorY() - 6), width, 7,
                            WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
    }
    draw_item(encoders[1]->cur - cur_row + n, n);
  }

  // draw_item.read(getRow());

  oled_display.setTextColor(WHITE, BLACK);
}

void MenuPageBase::display() {

  uint8_t number_of_items = get_menu()->get_number_of_items();
  uint8_t x_offset = 43;
  oled_display.clearDisplay();
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.println(get_menu()->get_name());
  mcl_gui.draw_vertical_dashline(x_offset - 6);

  draw_menu(x_offset, 8);

  if (number_of_items > visible_rows) {
    draw_scrollbar(120);
  }
}

bool MenuPageBase::enter() {
  DEBUG_PRINT_FN();
  void (*row_func)() = get_menu()->get_row_function(encoders[1]->cur);
  PageIndex page_callback = get_menu()->get_page_callback(encoders[1]->cur);
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
  void (*exit_func)() = get_menu()->get_exit_function();
  if (exit_func != NULL) {
    (*exit_func)();
  }
  mcl.popPage();
}

bool MenuPageBase::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t inc = 1;
      if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
        inc = 8;
      }
      switch (key) {
      case MDX_KEY_YES:
        trig_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES;
      case MDX_KEY_NO:
        trig_interface.ignoreNextEvent(MDX_KEY_NO);
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
  return false;
}
