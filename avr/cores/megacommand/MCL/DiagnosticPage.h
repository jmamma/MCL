/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef DIAGNOSTICPAGE_H__
#define DIAGNOSTICPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "CommonTools/Stopwatch.hh"

#define DIAGNOSTIC_NUM_COUNTER 4
#define DIAGNOSTIC_NUM_LOG 5

#define DIAG_DUMP(i, x) diag_page.set_perfcounter(i, #x, x)
#define DIAG_PRINTLN(x) diag_page.println(x)

class DiagnosticPage : public LightPage, MidiCallback {
private:
  uint16_t last_clock;
  bool active;

  unsigned long perf_counters[DIAGNOSTIC_NUM_COUNTER];
  char perf_name[DIAGNOSTIC_NUM_COUNTER][9];
  char log_buf[DIAGNOSTIC_NUM_LOG][17];

  uint8_t log_head;
  uint8_t cycle;

public:
  DiagnosticPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4), active(false), last_clock(0), cycle(0) {
        memset(perf_counters, 0, sizeof(perf_counters));
        memset(perf_name, 0, sizeof(perf_name));
        memset(log_buf, 0, sizeof(log_buf));
        log_head = DIAGNOSTIC_NUM_LOG - 1;
  }

  void set_perfcounter(uint8_t idx, const char* name, uint32_t val) {
    strncpy(perf_name[idx], name, 8);
    perf_counters[idx] = val;
    active = true;
  }

  void println(const char* msg) {
    strncpy(log_buf[log_head++], msg, 16);
    if (log_head >= DIAGNOSTIC_NUM_LOG) {
      log_head = 0;
    }
  }

  // -------- Diagnostic interfaces ----------
  void deactivate() { active = false; }
  bool is_active() { return active; }

  // -------- Page interfaces -----------
  void draw();
  void display();
  bool handleEvent(gui_event_t*);
};

extern DiagnosticPage diag_page;

#endif /* DIAGNOSTICPAGE_H__ */
