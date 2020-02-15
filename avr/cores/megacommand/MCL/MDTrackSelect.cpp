#include "MDTrackSelect.h"
#include "MD.h"
#include "MCL.h"

void MDTrackSelect::start() {

}

bool MDTrackSelect::on() {
  MD.activate_track_select();
  sysex->addSysexListener(this);
  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  state = true;
  return true;
}

bool MDTrackSelect::off() {
  MD.deactivate_track_select();
  sysex->removeSysexListener(this);
  if (!state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  state = false;
  return true;
}

void MDTrackSelect::end() { }


void MDTrackSelect::end_immediate() {
  if (!state) {
    return;
  }

 if (sysex->getByte(0) != ids[0]) { return false; }
 if (sysex->getByte(1) != ids[1]) { return false; }

 MD.currentTrack = sysex->getByte(2);
 return;
}

MDTrackSelect md_track_select;
