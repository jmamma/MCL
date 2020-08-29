#include "MCL_impl.h"

void DiagnosticPage::setup() { DEBUG_PRINT_FN(); }

void DiagnosticPage::init() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
}
void DiagnosticPage::cleanup() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void DiagnosticPage::loop() {
  if (clock_diff(last_clock,slowclock) >= 1000) {
  USE_LOCK();
  SET_LOCK();
  if (uart_tx_wr_old > MidiUart.txRb.wr) { uart_tx_wr_old = 0xFFFF - uart_tx_wr_old; }
  uart_tx_wr_rate =  (MidiUart.txRb.wr - uart_tx_wr_old);
  uart_tx_wr_old = MidiUart.txRb.wr;

  if (uart_rx_wr_old > MidiUart.rxRb.wr) { uart_rx_wr_old = 0xFFFF - uart_rx_wr_old; }
  uart_rx_wr_rate =  (MidiUart.rxRb.wr - uart_rx_wr_old);
  uart_rx_wr_old = MidiUart.rxRb.wr;

  last_clock = slowclock;
  CLEAR_LOCK();
  }

}

void DiagnosticPage::display() {

  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
#ifndef OLED_DISPLAY
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  uint8_t x;

  GUI.put_string_at(0, "Diagnostic");
  GUI.put_value_at1(4, page_mode ? 1 : 0);
  GUI.setLine(GUI.LINE2);
  /*
    if (mcl_cfg.ram_page_mode == 0) {
      GUI.put_string_at(0, "MON");
    } else {
      GUI.put_string_at(0, "LNK");
    }
  */

#endif
#ifdef OLED_DISPLAY
  oled_display.setFont();
  oled_display.setCursor(0, 0);

  oled_display.print("DIAG ");
  // oled_display.print(page_mode ? 1 : 0);
  oled_display.print(" ");
  oled_display.print(uart_tx_wr_rate);
  oled_display.setCursor(70, 0);
  oled_display.print(MidiUart.txRb.size());
  
  oled_display.setCursor(0, 15);
  oled_display.print(uart_rx_wr_rate);
  oled_display.setCursor(70, 15);
  oled_display.print(MidiUart.rxRb.size());
  oled_display.display();

#endif
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
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    page_mode = !(page_mode);
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
