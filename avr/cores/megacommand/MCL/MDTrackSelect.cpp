#include "MCL_impl.h"

void MDTrackSelect::start() {

}

bool MDTrackSelect::on() {
  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
//  MD.activate_track_select();
  sysex->addSysexListener(this);

  state = true;
  return true;
}

bool MDTrackSelect::off() {
  sysex->removeSysexListener(this);
  if (!state) {
    return false;
   }
  state = false;
  if (!MD.connected) {
    return false;
  }
//  MD.deactivate_track_select();
  return true;
}

void MDTrackSelect::end() {
  if (!state) {
    return;
  }
 if (sysex->getByte(0) != ids[0]) { return; }
 if (sysex->getByte(1) != ids[1]) { return; }

 if (sysex->recordLen == 7) {
  bool expand = true;
  seq_step_page.reset_undo();
  if (GUI.currentPage() == &seq_step_page) {
     if (seq_step_page.recording) { goto update_pattern; }
     MD.currentTrack = sysex->getByte(2);
     mcl_seq.md_tracks[MD.currentTrack].set_length(sysex->getByte(5), expand);
     mcl_seq.md_tracks[MD.currentTrack].set_speed(sysex->getByte(6));
     seq_step_page.config_encoders();
   }
   else if (GUI.currentPage() == &grid_page)  {
     update_pattern:
     for (uint8_t n = 0; n < 16; n++) {
       mcl_seq.md_tracks[n].set_length(sysex->getByte(5), expand);
       mcl_seq.md_tracks[n].set_speed(sysex->getByte(6));
     }
   }
 }

}


void MDTrackSelect::end_immediate() {
  if (!state) {
    return;
  }
 if (sysex->getByte(0) != ids[0]) { return; }
 if (sysex->getByte(1) != ids[1]) { return; }

 if (sysex->recordLen != 7) { MD.currentTrack = sysex->getByte(2); }
 MD.currentSynthPage = sysex->getByte(3);
 MD.global.extendedMode = sysex->getByte(4);
 return;
}

MDTrackSelect md_track_select;
