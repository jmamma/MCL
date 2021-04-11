#include "MCL_impl.h"

void QuestionDialogPage::init(const char* title_, const char* text_) {
#ifdef OLED_DISPLAY
  classic_display = false;
  mcl_gui.draw_infobox(title_, text_, -1);
  oled_display.drawFastHLine(MCLGUI::dlg_info_x1 + 1, MCLGUI::dlg_info_y2, MCLGUI::dlg_info_w - 2, BLACK);

  auto oldfont = oled_display.getFont();

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);

  oled_display.setCursor(MCLGUI::dlg_info_x2 - 86, MCLGUI::dlg_info_y1 + 24);
  oled_display.print("S NO");

  oled_display.setCursor(MCLGUI::dlg_info_x2 - 55, MCLGUI::dlg_info_y1 + 24);
  oled_display.print("W YES");

  oled_display.drawRect(MCLGUI::dlg_info_x2 - 88, MCLGUI::dlg_info_y1 + 16, 21, 9, WHITE);
  oled_display.drawRect(MCLGUI::dlg_info_x2 - 57, MCLGUI::dlg_info_y1 + 16, 21, 9, WHITE);

  oled_display.fillRect(MCLGUI::dlg_info_x2 - 87, MCLGUI::dlg_info_y1 + 17, 5, 7, INVERT);
  oled_display.fillRect(MCLGUI::dlg_info_x2 - 56, MCLGUI::dlg_info_y1 + 17, 5, 7, INVERT);

  oled_display.setFont(oldfont);
  oled_display.display();
#else
  title = title_;
  text = text_;
#endif
}

void QuestionDialogPage::display() {
#ifndef OLED_DISPLAY
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  uint8_t x;

  GUI.put_string_at(0, "[NO]");
  GUI.put_string_at(11, "[YES]");

  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, text);
  GUI.display_lcd();
#endif
}

bool QuestionDialogPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return false;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
#ifdef OLED_DISPLAY
    oled_display.fillRect(MCLGUI::dlg_info_x2 - 82, MCLGUI::dlg_info_y1 + 18, 12, 6, INVERT);
    oled_display.display();
#endif
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
#ifdef OLED_DISPLAY
    oled_display.fillRect(MCLGUI::dlg_info_x2 - 51, MCLGUI::dlg_info_y1 + 18, 12, 6, INVERT);
    oled_display.display();
#endif
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    return_state = true;
    GUI.popPage();
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    return_state = false;
    GUI.popPage();
    return true;
  }

  //GUI.popPage();

  return false;
}

QuestionDialogPage questiondialog_page;
