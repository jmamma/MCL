#include "MDTrackSelect.h"
#include "../MD.h"
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

void MDTrackSelect::handle_track_select_legacy(const SysexView &view,
                                               uint8_t len) {
  if (len == 8) {
    bool is_md_device =
        SeqPage::active_device_is_md() &&
        (mcl.currentPage() != SEQ_EXTSTEP_PAGE);
    bool expand = true;
    bool is_seq_page = mcl.isSeqPage();
    reset_undo();
    uint8_t length = view.getByte(6);
    uint8_t new_speed = view.getByte(7);
    if (is_seq_page) {
      if (SeqPage::recording) {
        goto update_pattern;
      }
      uint8_t b = view.getByte(3);
      MD.currentTrack = b & 0xF;
      uint8_t n = is_md_device ? MD.currentTrack : last_ext_track;

#if !defined(__AVR__)
      if (is_md_device && mcl_seq.using_spsx_tracks) {
        mcl_seq.spsx_tracks[n].set_length(length, expand);
        mcl_seq.spsx_tracks[n].set_speed(new_speed);
      } else
#endif
      {
        auto &track = SeqTrackUtil::get_track(is_md_device, n);
        track.set_length(length, expand);
        track.request_speed_change(new_speed);
      }
      if (mcl.currentPage() == SEQ_EXTSTEP_PAGE) { seq_extparam4.cur = length; }
      SeqPage *seq_page = (SeqPage*) GUI.currentPage();
      seq_page->config_encoders();
    } else {
    update_pattern:
      uint8_t track_len = SeqTrackUtil::track_count(is_md_device);
#if !defined(__AVR__)
      if (is_md_device && mcl_seq.using_spsx_tracks) {
        for (uint8_t n = 0; n < track_len; n++) {
          mcl_seq.spsx_tracks[n].set_length(length, expand);
          mcl_seq.spsx_tracks[n].set_speed(new_speed);
        }
      } else
#endif
      {
        for (uint8_t n = 0; n < track_len; n++) {
          auto &track = SeqTrackUtil::get_track(is_md_device, n);
          track.set_length(length, expand);
        }
        for (uint8_t n = 0; n < track_len; n++) {
          SeqTrackUtil::get_track(is_md_device, n).request_speed_change(new_speed);
        }
      }
      if (!is_md_device && last_ext_track < track_len) {
        seq_extparam4.cur = length;
      }
      if (is_seq_page) {
        SeqPage *seq_page = (SeqPage*) GUI.currentPage();
        seq_page->config_encoders();
      }
    }
  } else {

    uint8_t b = view.getByte(2);
    MD.global.extendedMode = b >> 4;
    MD.global.baseChannel = b & 0xF;

    b = view.getByte(3);
    if (len != 8) {
      MD.currentTrack = b & 0xF;
    }
    MD.currentSynthPage = (b >> 4) & 3;
    MD.currentBank = (b & 64) > 0;

    b = view.getByte(4);
    MD.kit.models[MD.currentTrack] = view.getByte(5);
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
        255,
        seq_ptc_page.primary_channel_event(seq_ptc_page.dev_note_channels[0]) !=
            POLY_EVENT);
  }
}

void MDTrackSelect::end() {
  if (!state) {
    return;
  }
  SysexView view(sysex);
  if (view.getByte(0) != ids[0]) {
    return;
  }
  if (view.getByte(1) != ids[1]) {
    return;
  }

  DEBUG_PRINTLN("track select end");
  uint8_t len = view.get_recordLen();
  DEBUG_PRINTLN(len);
  DEBUG_PRINTLN(msg_rd);

  // Handle legacy track select format only
  // Machine updates (0x63) and kit loaded (0x54) now use Elektron sysex
  handle_track_select_legacy(view, len);
}

MDTrackSelect md_track_select;
