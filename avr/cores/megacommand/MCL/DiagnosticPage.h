/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef DIAGNOSTICPAGE_H__
#define DIAGNOSTICPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"

class DiagnosticPage : public LightPage, MidiCallback {
public:
  DiagnosticPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }

  bool handleEvent(gui_event_t *event);

  bool page_mode;
  uint8_t page_id;
  uint16_t last_clock;
  uint16_t uart_tx_wr_old;
  uint16_t uart_tx_wr_rate;

  uint16_t uart_rx_wr_old;
  uint16_t uart_rx_wr_rate;


  void display();
  void setup();
  void init();
  void loop();
  void cleanup();

};

extern DiagnosticPage diag_page;

#endif /* DIAGNOSTICPAGE_H__ */
