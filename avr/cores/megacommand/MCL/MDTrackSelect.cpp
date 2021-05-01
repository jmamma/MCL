#include "MCL_impl.h"

void MDTrackSelect::start() {}

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
  if (sysex->getByte(0) != ids[0]) {
    return;
  }
  if (sysex->getByte(1) != ids[1]) {
    return;
  }

  if (sysex->recordLen == 8) {
    bool expand = true;
    reset_undo();
    uint8_t length = sysex->getByte(6);
    uint8_t new_speed = sysex->getByte(7);
    if (GUI.currentPage() == &seq_step_page) {
      if (seq_step_page.recording) {
        goto update_pattern;
      }
      uint8_t b = sysex->getByte(2);
      MD.currentTrack = b & 0xF;
      mcl_seq.md_tracks[MD.currentTrack].set_length(length, expand);
      mcl_seq.md_tracks[MD.currentTrack].set_speed(new_speed);
      seq_step_page.config_encoders();
    } else {
    update_pattern:
      uint8_t old_speeds[16];
      uint8_t old_mutes[16];
      for (uint8_t n = 0; n < 16; n++) {
        mcl_seq.md_tracks[n].set_length(length, expand);
        old_speeds[n] = mcl_seq.md_tracks[n].speed;
        if (old_speeds[n] == new_speed) {
          old_speeds[n] = 255;
          continue;
        }
        old_mutes[n] = mcl_seq.md_tracks[n].mute_state;
        mcl_seq.md_tracks[n].set_speed(new_speed, 255, false);
        mcl_seq.md_tracks[n].mute_state = SEQ_MUTE_ON;
      }
      for (uint8_t n = 0; n < 16; n++) {
        if (old_speeds[n] == 255)
          continue;
        mcl_seq.md_tracks[n].set_speed(new_speed, old_speeds[n], true);
        mcl_seq.md_tracks[n].mute_state = old_mutes[n];
      }
    }
  }
}

void MDTrackSelect::end_immediate() {
  if (!state) {
    return;
  }
  if (sysex->getByte(0) != ids[0]) {
    return;
  }
  if (sysex->getByte(1) != ids[1]) {
    return;
  }
  uint8_t b = sysex->getByte(2);
  if (sysex->recordLen != 8) {
    MD.currentTrack = b & 0xF;
  }
  MD.currentSynthPage = b >> 4;
  MD.global.extendedMode = sysex->getByte(3);
  b = sysex->getByte(4);
  MD.kit.models[MD.currentTrack] = sysex->getByte(5);
  if (b & 1) { MD.kit.models[MD.currentTrack] += 128; }
  if (b & 2) { MD.kit.models[MD.currentTrack] += 0x20000; }
  return;
}

MDTrackSelect md_track_select;
