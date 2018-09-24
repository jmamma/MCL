#include "GridTask.h"
#include "MCL.h"

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::destroy() {}

void GridTask::run() {
  //  DEBUG_PRINTLN(MidiClock.div32th_counter / 2);
  //  A4Track *a4_track = (A4Track *)&temp_track;
  //  ExtTrack *ext_track = (ExtTrack *)&temp_track;
  if (MidiClock.state != 2) {
    return;
  }

  EmptyTrack empty_track;
  MDTrack *md_track = (MDTrack *)&empty_track;
  uint32_t div16th_counter = MidiClock.div16th_counter;
  uint32_t myclock = slowclock;

  bool send = false;
  uint8_t slots_changed[16];
  for (uint8_t n = 0; n < 16; n++) {
    //   if (n < 9) {
    //     chains[n].active = 1;
    //   } else {
    //     chains[n].active = false;
    //  }
    slots_changed[n] = 0;
    if (chains[n].active > 0) {
      if ((((div16th_counter + 3 - mcl_actions.start_clock16th)) %
           (chains[n].loops * mcl_seq.md_tracks[n].length)) == 0) {

        send = false;
        md_track->load_from_mem(n);

        if (md_track->active != EMPTY_TRACK_TYPE) {
          md_track->place_track_in_kit(n, chains[n].row, &(MD.kit));
          mcl_seq.disable();
          //  mcl_seq.md_tracks[n].mute();
          MD.setMachine(n, &(md_track->machine));
          md_track->load_seq_data(n);
          mcl_seq.enable();
          //   mcl_seq.md_tracks[n].unmute();
          //    DEBUG_PRINTLN(MidiClock.div16th_counter);
        } else {
          bool clear_locks = true;
          mcl_seq.md_tracks[n].clear_track(clear_locks);
        }
        memcpy(&chains[n], &md_track->chain, sizeof(GridChain));
        slots_changed[n] = 1;
      }
    }
  }
  for (uint8_t n = 0; n < 16; n++) {
    if (slots_changed[n] == 1) {
      int32_t len =
          sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);
      if (md_track->load_track_from_grid(n, chains[n].row, len)) {
        md_track->store_in_mem(n);

        grid_page.active_slots[n] = md_track->chain.row;
      }
    }
  }
  if (send) {
    // while ((((div16th_counter + 1 - mcl_actions.start_clock16th)) % (16)) !=
    // 0)
    //   ;
    /*
          ElektronDataToSysexEncoder encoder(&MidiUart);
    mcl_actions.md_setsysex_recpos(4, MD.kit.origPosition);

    mcl_seq.disable();
    MD.kit.toSysex(encoder);
    uint32_t pos = BANK1_R1_START;
    ptr = reinterpret_cast<uint8_t *>(pos);

    for (uint8_t n = 0; n < 16; n++) {
      if (chains[n].active > 0) {
        switch_ram_bank(1);
        memcpy(&md_track, ptr, len);
        switch_ram_bank(0);
        md_track.load_seq_data(n);
      }
      pos += len;
      ptr = reinterpret_cast<uint8_t *>(pos);
    }
    MD.loadKit(MD.pattern.kit);
    mcl_seq.enable();
 */
  }
}
GridTask grid_task(0);
