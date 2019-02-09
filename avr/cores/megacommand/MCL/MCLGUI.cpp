#include "MCL.h"

bool MCLGUI::wait_for_input(char *dst, char *title, uint8_t len) {
  text_input_page.init();
  text_input_page.init_text(dst, title, len);
  GUI.pushPage(&text_input_page);
  while (GUI.currentPage() == &text_input_page) {
     GUI.loop();
  }
  return text_input_page.return_state;
}

