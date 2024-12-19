#include "QuestionDialogPage.h"
#include "MCLGUI.h"

void QuestionDialogPage::init(const char* title_, const char* text_) {
  mcl_gui.draw_infobox(title_, text_, -1);
  oled_display.drawFastHLine(MCLGUI::dlg_info_x1 + 1, MCLGUI::dlg_info_y2, MCLGUI::dlg_info_w - 2, BLACK);

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);

  oled_display.setCursor(MCLGUI::dlg_info_x2 - 86, MCLGUI::dlg_info_y1 + 23);
  oled_display.print(F(" NO"));

  oled_display.setCursor(MCLGUI::dlg_info_x2 - 55, MCLGUI::dlg_info_y1 + 23);
  oled_display.print(F(" YES"));

  oled_display.drawRect(MCLGUI::dlg_info_x2 - 88, MCLGUI::dlg_info_y1 + 16, 18, 9, WHITE);
  oled_display.drawRect(MCLGUI::dlg_info_x2 - 57, MCLGUI::dlg_info_y1 + 16, 18, 9, WHITE);

}

void QuestionDialogPage::display() {
}

bool QuestionDialogPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return false;
  }

    if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES:
      //  trig_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES;
      case MDX_KEY_NO:
      //  trig_interface.ignoreNextEvent(MDX_KEY_NO);
        goto NO;
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_YES:
      //  trig_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES_released;
      case MDX_KEY_NO:
      //  trig_interface.ignoreNextEvent(MDX_KEY_NO);
        goto NO_released;
      }
    }

    }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    NO:
    oled_display.fillRect(MCLGUI::dlg_info_x2 - 87, MCLGUI::dlg_info_y1 + 17, 16, 7, INVERT);
    oled_display.display();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4) && (!BUTTON_DOWN(Buttons.BUTTON1))) {
    YES:
    oled_display.fillRect(MCLGUI::dlg_info_x2 - 56, MCLGUI::dlg_info_y1 + 17, 16, 7, INVERT);
    oled_display.display();
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    YES_released:
    return_state = true;
    mcl.popPage();
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    NO_released:
    return_state = false;
    mcl.popPage();
    return true;
  }

  //mcl.popPage();

  return false;
}

QuestionDialogPage questiondialog_page;
