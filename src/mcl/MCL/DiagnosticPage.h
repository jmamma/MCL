/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef DIAGNOSTICPAGE_H__
#define DIAGNOSTICPAGE_H__
#include "GUI.h"
#include "MCLEncoder.h"
#include "CommonTools/Stopwatch.h"

#ifdef ENABLE_DIAG_LOGGING

#define DIAGNOSTIC_NUM_COUNTER 4
#define DIAGNOSTIC_NUM_LOG 40
#define DIAG_MEASURE(i, x) diag_page.set_perfcounter(i, #x, x)
#define DIAG_PRINTLN(x) diag_page.println(x)
#define DIAG_DUMP(x) diag_page.println(#x, x)

#else

#define DIAGNOSTIC_NUM_COUNTER 0
#define DIAGNOSTIC_NUM_LOG 0
#define DIAG_MEASURE(i, x)
#define DIAG_PRINTLN(x)
#define DIAG_DUMP(x)

#endif

#ifdef ENABLE_DIAG_LOGGING
class DiagnosticPage : public LightPage, MidiCallback {
private:
  uint16_t last_clock;
  bool active;
  uint8_t mode;

#ifdef ENABLE_DIAG_LOGGING
  unsigned long perf_counters[DIAGNOSTIC_NUM_COUNTER];
  char perf_name[DIAGNOSTIC_NUM_COUNTER][9];
  char log_buf[DIAGNOSTIC_NUM_LOG][17];

  uint8_t log_head;
  uint8_t log_disp_frame;
  uint8_t log_disp_head;

  void advance_log_head() {
    ++log_head;
    mode = 1;
    active = true;
    if (log_head >= DIAGNOSTIC_NUM_LOG) {
      log_head = 0;
    }
  }
#endif
  void draw_perfcounter();
  void draw_log();

public:
  DiagnosticPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4), active(false), last_clock(0), mode(0) {
        memset(perf_counters, 0, sizeof(perf_counters));
        memset(perf_name, 0, sizeof(perf_name));
        memset(log_buf, 0, sizeof(log_buf));
        log_head = DIAGNOSTIC_NUM_LOG - 1;
        log_disp_frame = 0;
        log_disp_head = log_head;
  }

  void set_perfcounter(uint8_t idx, const char* name, uint32_t val) {
    strncpy(perf_name[idx], name, 8);
    perf_counters[idx] = val;
    active = true;
    mode = 0;
  }

  void println(const char* msg) {
    strncpy(log_buf[log_head], msg, 16);
    advance_log_head();
  }

  void println(const char* msg, uint16_t val) {
    char buf[17];
    strncpy(log_buf[log_head], msg, 16);
    snprintf(buf, 16, "%u", val);
    strncat(log_buf[log_head], buf, 16);
    advance_log_head();
  }

  // -------- Diagnostic interfaces ----------
  void deactivate() { if (mode == 0) {active = false; } }
  bool is_active() { return active; }

  // -------- Page interfaces -----------
  void draw();
  void display();
  bool handleEvent(gui_event_t*);
};

extern DiagnosticPage diag_page;
#endif

#endif /* DIAGNOSTICPAGE_H__ */
