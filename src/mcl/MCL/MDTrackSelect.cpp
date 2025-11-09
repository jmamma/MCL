#include "MDTrackSelect.h"
#include "MD.h"
#include "MidiClock.h"
#include "SeqPtcPage.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

void MDTrackSelect::setup(MidiClass *_midi) {
  sysex = _midi->midiSysex;
}

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

  DEBUG_PRINTLN("track select end");
  DEBUG_PRINTLN(sysex->get_recordLen());
  DEBUG_PRINTLN(msg_rd);
  if (sysex->get_recordLen() == 8) {
    bool is_md_device =
        SeqTrackUtil::is_md_device(SeqPage::midi_device) &&
        (mcl.currentPage() != SEQ_EXTSTEP_PAGE);
    bool expand = true;
    bool is_seq_page = mcl.isSeqPage();
    reset_undo();
    uint8_t length = sysex->getByte(6);
    uint8_t new_speed = sysex->getByte(7);
    if (is_seq_page) {
      if (SeqPage::recording) {
        goto update_pattern;
      }
      uint8_t b = sysex->getByte(3);
      MD.currentTrack = b & 0xF;
      uint8_t n = is_md_device ? MD.currentTrack : last_ext_track;

      auto &track = SeqTrackUtil::get_track(is_md_device, n);
      track.set_length(length, expand);
      SeqTrackUtil::sync_ext_length_encoder(
          is_md_device, n, length, mcl.currentPage() == SEQ_EXTSTEP_PAGE);
      track.request_speed_change(new_speed);
      SeqPage *seq_page = (SeqPage*) GUI.currentPage();
      seq_page->config_encoders();
    } else {
    update_pattern:
      uint8_t len = SeqTrackUtil::track_count(is_md_device);

      bool wait_for_tick = MidiClock.isStarted();
      uint8_t mod12_snapshot = MidiClock.mod12_counter;

      for (uint8_t n = 0; n < len;) {
        if (wait_for_tick) {
          if (!MidiClock.isStarted() ||
              mod12_snapshot != MidiClock.mod12_counter) {
            wait_for_tick = false;
          } else {
            continue;
          }
        }
        SeqTrackUtil::get_track(is_md_device, n)
            .request_speed_change(new_speed);
        n++;
      }

      for (uint8_t n = 0; n < len; n++) {
        auto &track = SeqTrackUtil::get_track(is_md_device, n);
        track.set_length(length, expand);
        SeqTrackUtil::sync_ext_length_encoder(is_md_device, n, length,
                                              last_ext_track == n);
      }
      if (is_seq_page) {
        SeqPage *seq_page = (SeqPage*) GUI.currentPage();
        seq_page->config_encoders();
      }
    }
  } else {

    uint8_t b = sysex->getByte(2);
    MD.global.extendedMode = b >> 4;
    MD.global.baseChannel = b & 0xF;

    b = sysex->getByte(3);
    if (sysex->get_recordLen() != 8) {
      MD.currentTrack = b & 0xF;
    }
    MD.currentSynthPage = (b >> 4) & 3;
    MD.currentBank = (b & 64) > 0;

    b = sysex->getByte(4);
    MD.kit.models[MD.currentTrack] = sysex->getByte(5);
    if (b & 1) {
      MD.kit.models[MD.currentTrack] += 128;
    }
    if (b & 2) {
      MD.kit.models[MD.currentTrack] += 0x20000;
    }
  }
  bool ret = seq_step_page.md_track_change_check();
  if (ret) {
    arp_page.track_update(
        255, seq_ptc_page.is_md_midi(seq_ptc_page.dev_note_channels[0]) !=
                 POLY_EVENT);
  }
}

MDTrackSelect md_track_select;
