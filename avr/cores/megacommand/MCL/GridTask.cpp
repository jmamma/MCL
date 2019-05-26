#include "EmptyTrack.h"
#include "GridTask.h"
#include "MCL.h"

#define DIV16_MARGIN 8
#define HANDLE_GROUPS 1

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

  int slots_changed[NUM_TRACKS];
  uint8_t slots_loaded[NUM_MD_TRACKS];

  for (uint8_t i = 0; i < NUM_MD_TRACKS; i++) {
    slots_loaded[i] = 0;
  }

  bool send_a4_sound = false;
  bool send_md_kit = false;

  // uint8_t div16th_margin = (((md_div32th_latency + a4_div32th_latency)) / 2)
  // + 4;
  uint8_t div32th_margin = 8;
  // DEBUG_PRINTLN(md_div32th_latency);
  // DEBUG_PRINTLN(a4_div32th_latency);

  uint32_t div32th_counter;
  if (mcl_cfg.chain_mode == 0) {
    return;
  }
//  DEBUG_PRINTLN(MidiClock.div16th_counter);
  uint8_t curkit;
  ElektronDataToSysexEncoder encoder(&MidiUart);

  //! MidiClock.clock_less_than(a,b) == !(a < b) == (a >= b)

  if (!MidiClock.clock_less_than(MidiClock.div32th_counter + div32th_margin,
                                 (uint32_t)mcl_actions.next_transition * 2)) {
    //   while ((MidiClock.div32th_counter != (mcl_actions.next_transition * 2 -
    //   div32th_latency)) && (MidiClock.state == 2));

    DEBUG_PRINTLN("Preparing for next transition:");
    DEBUG_PRINTLN(MidiClock.div16th_counter);
    DEBUG_PRINTLN(mcl_actions.next_transition);

    // MD.getCurrentKit();
    // MD.getBlockingKit(MD.currentKit);
    // mcl_actions.md_setsysex_recpos(4, MD.currentKit);
    div32th_counter = MidiClock.div32th_counter + div32th_margin;
  } else {
    return;
  }

  GUI.removeTask(&grid_task);

  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    slots_changed[n] = -1;
    if ((grid_page.active_slots[n] >= 0) && (mcl_actions.chains[n].loops > 0)) {
      // mark slot as changed in case next statement doesnt pass
      uint32_t next_transition = (uint32_t)mcl_actions.next_transitions[n] * 2;

      if (!MidiClock.clock_less_than(div32th_counter, next_transition)) {

        slots_changed[n] = mcl_actions.chains[n].row;
        if ((mcl_actions.chains[n].row != grid_page.active_slots[n]) ||
            (mcl_cfg.chain_mode == 2)) {

          if (n < NUM_MD_TRACKS) {

            md_track->load_from_mem(n);
            if (slots_loaded[n] == 0) {
              //        md_track->place_track_in_kit(n, n, &(MD.kit));
            }

            slots_changed[n] = mcl_actions.chains[n].row;
            memcpy(&mcl_actions.chains[n], &md_track->chain, sizeof(GridChain));
            if (mcl_cfg.chain_mode == 2) {
              mcl_actions.chains[n].loops = 0;
            } else if (mcl_cfg.chain_mode == 3) {
              mcl_actions.chains[n].loops = random(1, 8);
              mcl_actions.chains[n].row =
                  random(mcl_cfg.chain_rand_min, mcl_cfg.chain_rand_max);
            }

            send_md_kit = true;
          } else {

            slots_changed[n] = mcl_actions.chains[n].row;
            memcpy(&mcl_actions.chains[n], &ext_track->chain,
                   sizeof(GridChain));
            send_a4_sound = true;
          }
        }
      }
    }
  }
  if (send_a4_sound) {
    DEBUG_PRINTLN("waiting to send a4");
    DEBUG_PRINTLN(MidiClock.div192th_counter);
    DEBUG_PRINTLN(mcl_actions.a4_latency);
    DEBUG_PRINTLN(mcl_actions.a4_div192th_latency);
    DEBUG_PRINTLN(mcl_actions.next_transition * 12 -
                  mcl_actions.a4_div192th_latency);

    uint32_t go_step = mcl_actions.next_transition * 12 -
                       mcl_actions.md_div192th_latency -
                       mcl_actions.a4_div192th_latency;
    uint32_t diff;
    if (mcl_actions.a4_latency > 0) {
      while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter,
                                                  go_step)) != 0) &&
             (MidiClock.div192th_counter < go_step) && (MidiClock.state == 2)) {
        if (diff > 8) {

          handleIncomingMidi();
          GUI.loop();
        }
      }
      // in_sysex2 = 1;
    }
    for (uint8_t n = NUM_MD_TRACKS; n < NUM_TRACKS; n++) {
      if (slots_changed[n] >= 0) {
        a4_track->load_from_mem(n);
        DEBUG_PRINTLN(mcl_actions.a4_latency);

        if (a4_track->active == A4_TRACK_TYPE) {
          if ((mcl_actions.a4_latency > 0) &&
              (mcl_actions.send_machine[n] == 0)) {
            DEBUG_PRINTLN("here");
            if (a4_track->active == A4_TRACK_TYPE) {
              DEBUG_PRINTLN("send a4 sound");
              a4_track->sound.toSysex();
            }
          }
        }

        grid_page.active_slots[n] = slots_changed[n];
        if (a4_track->active != EMPTY_TRACK_TYPE) {
          mcl_seq.ext_tracks[n - NUM_MD_TRACKS].buffer_notesoff();

          mcl_seq.ext_tracks[n - NUM_MD_TRACKS].start_step =
              mcl_actions.next_transition;
          mcl_seq.ext_tracks[n - NUM_MD_TRACKS].mute_until_start = true;
          a4_track->load_seq_data(n - NUM_MD_TRACKS);
        } else {
          DEBUG_PRINTLN("clearing track");
          mcl_seq.ext_tracks[n - NUM_MD_TRACKS].clear_track();
        }
      }
    }
  }
  if (send_md_kit) {
    DEBUG_PRINTLN(MidiClock.div192th_counter);
    DEBUG_PRINTLN(mcl_actions.next_transition * 12 -
                  mcl_actions.md_div192th_latency);
    uint32_t go_step =
        mcl_actions.next_transition * 12 - mcl_actions.md_div192th_latency;
    uint32_t diff;
    while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter,
                                                go_step)) != 0) &&
           (MidiClock.div192th_counter < go_step) && (MidiClock.state == 2)) {
      if (diff > 8) {

        handleIncomingMidi();

        GUI.loop();
      }
    }
    DEBUG_PRINTLN("div16");
    DEBUG_PRINTLN(MidiClock.div16th_counter);
    // in_sysex = 1;
    uint32_t div192th_counter_old = MidiClock.div192th_counter;
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {

      if (slots_changed[n] >= 0) {
        md_track->load_from_mem(n);
        if (md_track->active == MD_TRACK_TYPE) {
          if (mcl_actions.send_machine[n] == 0) {
#ifdef HANDLE_GROUPS
            uint8_t trigGroup = md_track->machine.trigGroup;
            if ((trigGroup < 16) && (trigGroup != n) &&
                (slots_loaded[trigGroup] == 0)) {
              md_track->load_from_mem(trigGroup);
              if (md_track->active == MD_TRACK_TYPE) {

                bool set_level = false;
                if (mcl_actions.transition_level[n] == 1) {
                  set_level = true;
                  md_track->machine.level = 0;
                }

                mcl_actions.md_set_machine(trigGroup, &(md_track->machine),
                                           &(MD.kit), set_level);
                md_track->place_track_in_kit(trigGroup, trigGroup, &(MD.kit),
                                             set_level);
              }
              md_track->load_from_mem(n);
              slots_loaded[trigGroup] = 1;
            }
#endif
            if (slots_loaded[n] == 0) {
              bool set_level = false;
              if (mcl_actions.transition_level[n] == 1) {
                set_level = true;
                md_track->machine.level = 0;
              }
              mcl_actions.md_set_machine(n, &(md_track->machine), &(MD.kit),
                                         set_level);
              md_track->place_track_in_kit(n, n, &(MD.kit), set_level);
              slots_loaded[n] = 1;
            }
          }

            mcl_seq.md_tracks[n].start_step = mcl_actions.next_transition;
            mcl_seq.md_tracks[n].mute_until_start = true;

            md_track->load_seq_data(n);
        }

        else {
          //&& (mcl_cfg.chain_mode != 2)) {
          DEBUG_PRINTLN("clearing track");
          bool clear_locks = true;
          mcl_seq.md_tracks[n].clear_track(clear_locks);
        }

        grid_page.active_slots[n] = slots_changed[n];
      }
    }
  }
  //  if (send_md_kit || send_a4_sound) {
  uint8_t count = 0;
  uint8_t slots_cached[NUM_TRACKS] = {0};

  EmptyTrack empty_track2;
  MDTrack *md_temp_track = (MDTrack *)&empty_track2;
  A4Track *a4_temp_track = (A4Track *)&empty_track2;

  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    if (slots_changed[n] >= 0) {

      handleIncomingMidi();
      if (count % 8 == 0) {
        if (GUI.currentPage() != &grid_write_page) {
          GUI.loop();
        }
      }
      if ((slots_changed[n] != mcl_actions.chains[n].row)) {

        count++;
        if (n < NUM_MD_TRACKS) {
          //          DEBUG_PRINTLN("trying to cache MD track");
          //         DEBUG_PRINTLN(n);
          //       DEBUG_PRINTLN(mcl_actions.chains[n].row);
          int32_t len =
              sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);
          if (md_track->load_track_from_grid(n, mcl_actions.chains[n].row,
                                             len)) {
            //  DEBUG_PRINTLN("storing");
            md_temp_track->load_from_mem(n);

            if ((md_track->active != EMPTY_TRACK_TYPE) && (memcmp(&(md_temp_track->machine), &(md_track->machine),
                       sizeof(MDMachine)) != 0)) {
              mcl_actions.send_machine[n] = 0;
            } else {
              mcl_actions.send_machine[n] = 1;
              DEBUG_PRINTLN("machines match");
            }

            md_track->store_in_mem(n);
            slots_cached[n] = 1;

#ifdef HANDLE_GROUPS
            uint8_t trigGroup = md_track->machine.trigGroup;
            if ((trigGroup < 16) && (trigGroup != n) &&
                (slots_cached[trigGroup] == 0)) {
              if (md_track->load_track_from_grid(
                      trigGroup, mcl_actions.chains[n].row, len)) {
                md_track->store_in_mem(trigGroup);
                mcl_actions.send_machine[trigGroup] =
                    mcl_actions.send_machine[n];
                slots_cached[trigGroup] = 1;
              }
            }
#endif
          } else {
            DEBUG_PRINTLN("failed");
          }
        } else {
          DEBUG_PRINTLN("trying to load a4 track");
          DEBUG_PRINTLN(n);
          DEBUG_PRINTLN(mcl_actions.chains[n].row);
          if (a4_track->load_track_from_grid(n, mcl_actions.chains[n].row, 0)) {
            a4_temp_track->load_from_mem(n);
            if ((a4_track->active != EMPTY_TRACK_TYPE) && (memcmp(&(a4_temp_track), &(a4_track), sizeof(A4Track)) != 0)) {
              mcl_actions.send_machine[n] = 0;
            } else {
              mcl_actions.send_machine[n] = 1;
              DEBUG_PRINTLN("sounds match");
            }
            a4_track->store_in_mem(n);
          }
        }
      }

      mcl_actions.calc_next_slot_transition(n);
    }
  }
  mcl_actions.calc_next_transition();
  mcl_actions.calc_latency(&empty_track);
  //  }
  GUI.addTask(&grid_task);
}

GridTask grid_task(0);
