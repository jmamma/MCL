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
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  uint32_t div16th_counter = MidiClock.div16th_counter;

  uint8_t slots_changed[20];
  GUI.removeTask(&grid_task);
  for (uint8_t n = 0; n < 20; n++) {
    slots_changed[n] = 0;

    if (chains[n].active > 0) {
      if (n < 16) {
        if ((((div16th_counter + 3 - mcl_actions.start_clock16th)) %
             (chains[n].loops * mcl_seq.md_tracks[n].length)) == 0) {
          DEBUG_PRINT("loading ");
          DEBUG_PRINTLN(n);
          md_track->load_from_mem(n);
          if (md_track->active != MD_TRACK_TYPE) {
            DEBUG_PRINTLN("shit");
          }
          if (md_track->active != EMPTY_TRACK_TYPE) {

            mcl_seq.disable();
            //  mcl_seq.md_tracks[n].mute();
            mcl_actions.md_set_machine(n, &(md_track->machine), &(MD.kit));
            md_track->place_track_in_kit(n, n, &(MD.kit));
            md_track->load_seq_data(n);
            mcl_seq.md_tracks[n].update_params();
            mcl_seq.enable();
            //   mcl_seq.md_tracks[n].unmute();
            //    DEBUG_PRINTLN(MidiClock.div16th_counter);
          } else {
            bool clear_locks = true;
            mcl_seq.md_tracks[n].clear_track(clear_locks);
          }

          grid_page.active_slots[n] = chains[n].row;
          memcpy(&chains[n], &md_track->chain, sizeof(GridChain));

          slots_changed[n] = 1;
        }
      } else {
        if ((((div16th_counter + 3 - mcl_actions.start_clock16th)) %
             (chains[n].loops * mcl_seq.ext_tracks[n - 16].length)) == 0) {
          a4_track->load_from_mem(n);

          if (a4_track->active != EMPTY_TRACK_TYPE) {
            mcl_seq.disable();
            mcl_seq.ext_tracks[n - 16].buffer_notesoff();
            //  mcl_seq.md_tracks[n].mute();
            if (a4_track->active == A4_TRACK_TYPE) {
              a4_track->sound.toSysex();
            }

            a4_track->load_seq_data(n - 16);
            mcl_seq.enable();
            //   mcl_seq.md_tracks[n].unmute();
            //    DEBUG_PRINTLN(MidiClock.div16th_counter);
          } else {
            mcl_seq.ext_tracks[n - 16].clear_track();
          }

          grid_page.active_slots[n] = chains[n].row;
          memcpy(&chains[n], &ext_track->chain, sizeof(GridChain));
          slots_changed[n] = 1;
        }
      }
    }
  }
  MD.saveCurrentKit(MD.currentKit);
  uint8_t count = 0;
  for (uint8_t n = 0; n < 20; n++) {
    if (slots_changed[n] == 1) {
      if (count % 8 == 0) {
        handleIncomingMidi();
        GUI.loop();
      }
      count++;
      if (n < 16) {

        int32_t len =
            sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);
        if (md_track->load_track_from_grid(n, chains[n].row, len)) {
          md_track->store_in_mem(n);
        }
      } else {
        DEBUG_PRINTLN(chains[n].row);
        if (a4_track->load_track_from_grid(n, chains[n].row, 0)) {
          a4_track->store_in_mem(n);
        }
      }
    }
  }
  GUI.addTask(&grid_task);
}
GridTask grid_task(0);
