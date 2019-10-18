#include "MCL.h"
#include "QuestionDialogPage.h"

void QuestionDialogPage::init(const char* title, const char* text) {
#ifdef OLED_DISPLAY
  mcl_gui.draw_infobox(title, text, -1);
  oled_display.drawFastHLine(MCLGUI::info_x1 + 1, MCLGUI::info_y2, MCLGUI::info_w - 2, BLACK);

  auto oldfont = oled_display.getFont();

  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE);

  oled_display.setCursor(MCLGUI::info_x2 - 86, MCLGUI::info_y1 + 24);
  oled_display.print("2 YES");

  oled_display.setCursor(MCLGUI::info_x2 - 55, MCLGUI::info_y1 + 24);
  oled_display.print("3  NO");

  oled_display.drawRect(MCLGUI::info_x2 - 88, MCLGUI::info_y1 + 17, 21, 8, WHITE);
  oled_display.drawRect(MCLGUI::info_x2 - 57, MCLGUI::info_y1 + 17, 19, 8, WHITE);

  oled_display.fillRect(MCLGUI::info_x2 - 87, MCLGUI::info_y1 + 18, 5, 6, INVERT);
  oled_display.fillRect(MCLGUI::info_x2 - 56, MCLGUI::info_y1 + 18, 5, 6, INVERT);

  oled_display.setFont(oldfont);
  oled_display.display();
#else
  // TODO
#endif
}

bool QuestionDialogPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return false;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    oled_display.fillRect(MCLGUI::info_x2 - 82, MCLGUI::info_y1 + 18, 12, 6, INVERT);
    oled_display.display();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    oled_display.fillRect(MCLGUI::info_x2 - 51, MCLGUI::info_y1 + 18, 12, 6, INVERT);
    oled_display.display();
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    return_state = true;
    GUI.popPage();
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    return_state = false;
    GUI.popPage();
    return true;
  }

  //GUI.popPage();

  return false;
}

QuestionDialogPage questiondialog_page;
