#include "MDTrackSelect.h"
#include "MD.h"
#include "MCL.h"

void MDTrackSelect::start() {

}

bool MDTrackSelect::on() {
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
  uint8_t last_md_track = sysex->getByte(2);
  MD.currentTrack = last_md_track;
  return;
}

MDTrackSelect md_track_select;
