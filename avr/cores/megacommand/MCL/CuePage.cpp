#include "CuePage.h"

bool CuePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    note_interface.draw_notes(0);
    if (note_interface.notes_all_off()) {
      if ((curpage == CUE_PAGE) && (trackinfo_param4.getValue() > 0) &&
          (note_noteinterface.notes_count_off() > 1)) {
        toggle_cues_batch();
        md_exploit.send_globals();
        md_exploit.off();
        GUI.setPage(&page);
        curpage = 0;
      }
    }
  return true;
  }

  return false;
}
