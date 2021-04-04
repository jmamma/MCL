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

void MDTrackSelect::end() { }


void MDTrackSelect::end_immediate() {
  if (!state) {
    return;
  }

 if (sysex->getByte(0) != ids[0]) { return; }
 if (sysex->getByte(1) != ids[1]) { return; }

 MD.currentSynthPage = sysex->getByte(3);
 MD.global.extendedMode = sysex->getByte(4);
 if (sysex->recordLen == 7) {
   if (GUI.currentPage() == &seq_step_page) {
     MD.currentTrack = sysex->getByte(2);
     mcl_seq.md_tracks[MD.currentTrack].set_length(sysex->getByte(5));
     mcl_seq.md_tracks[MD.currentTrack].speed = sysex->getByte(6);
     seq_step_page.config_encoders();
     setLed2();
   }
   else if (GUI.currentPage() == &grid_page)  {
     for (uint8_t n = 0; n < 16; n++) {
       mcl_seq.md_tracks[n].set_length(sysex->getByte(5));
       mcl_seq.md_tracks[n].set_speed(sysex->getByte(6));
     }
   }
 }
 else {
   MD.currentTrack = sysex->getByte(2);
 }
 DEBUG_PRINTLN("extended");
 DEBUG_PRINTLN( MD.global.extendedMode);
 return;
}

MDTrackSelect md_track_select;
