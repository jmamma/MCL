#include "MCL_impl.h"

#if defined(ENABLE_DIAG_LOGGING)

#if defined(OLED_DISPLAY) 

void _draw_frame(uint8_t w) {
  oled_display.fillRect(60, 0, w + 2, 32, BLACK);
  oled_display.drawRect(61, 0, w, 32, WHITE);
}

void DiagnosticPage::draw_perfcounter() {
  _draw_frame(52);

  auto clock = read_slowclock();

  oled_display.setCursor(64, 7);
  oled_display.print("GUI loop");
  oled_display.setCursor(97, 7);
  oled_display.print(clock_diff(last_clock, clock));

  uint8_t y = 13;
  for(int i=0;i<DIAGNOSTIC_NUM_COUNTER;++i) {
    oled_display.setCursor(64, y);
    oled_display.print(perf_name[i]);
    oled_display.setCursor(97, y);
    oled_display.print(perf_counters[i]);
    y += 6;
  }

  last_clock = clock;
}

void DiagnosticPage::draw_log() {
  _draw_frame(66);

  uint8_t y = 7;
  int8_t log_idx = log_disp_head - 4;
  if(log_idx < 0) { log_idx += DIAGNOSTIC_NUM_LOG; }
  for(int i=0;i<5;++i) {
    if (log_idx >= DIAGNOSTIC_NUM_LOG) {
      log_idx = 0;
    }
    oled_display.setCursor(64, y);
    oled_display.print(log_buf[log_idx++]);
    y = y + 6;
  }

  if(++log_disp_frame > 3) {
    log_disp_frame = 0;
    if(log_disp_head != log_head && ++log_disp_head >= DIAGNOSTIC_NUM_LOG) {
      log_disp_head = 0;
    }
  }
}

void DiagnosticPage::draw() {

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  if (mode == 0) {
    draw_perfcounter();
  } else {
    draw_log();
  }

  oled_display.setFont(oldfont);
}
#else
void DiagnosticPage::draw() { }
void DiagnosticPage::draw_perfcounter() { }
void DiagnosticPage::draw_log() { }

#endif

void DiagnosticPage::display() {
  // if DiagnosticPage is pushed to the pages stack, we have to cancel
  // the active state so that draw() is not called in oled_display.display()
  draw();
  active = false;
  oled_display.display();
}

bool DiagnosticPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
   return true; 
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    return true;
  }

  return false;
}

DiagnosticPage diag_page;
#endif
