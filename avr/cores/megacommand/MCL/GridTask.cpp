#include "GridTask.h"
#include "MCL.h"

#define DIV16_MARGIN 8

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

  uint8_t slots_changed[20];
  uint8_t slots_loaded[16];

  for (uint8_t i = 0; i < 16; i++) {
    slots_loaded[i] = 0;
  }

  bool send_a4_sound = false;
  bool send_md_kit = false;

  float bytes_per_second_uart1 = (float)MidiUart.speed / (float)10;

  float bytes_per_second_uart2 = (float)MidiUart2.speed / (float)10;

  float md_latency_in_seconds =
      (float)mcl_actions.md_latency / bytes_per_second_uart1;
  float a4_latency_in_seconds =
      (float)mcl_actions.a4_latency / bytes_per_second_uart2;
  float div32th_per_second =
      ((float)MidiClock.tempo / (float)60) * (float)4 * (float)2;
  // DEBUG_PRINTLN(div32th_per_second * latency_in_seconds);
  float div192th_per_second =
      ((float)MidiClock.tempo / (float)60) * (float)4 * (float)12;
  // DEBUG_PRINTLN(div32th_per_second * latency_in_seconds);
  uint8_t md_div32th_latency =
      round(div32th_per_second * md_latency_in_seconds) + 1;
  uint8_t a4_div32th_latency =
      round(div32th_per_second * a4_latency_in_seconds) + 1;

  uint8_t md_div192th_latency =
      round(div192th_per_second * md_latency_in_seconds) + 3;
  uint8_t a4_div192th_latency =
      round(div192th_per_second * a4_latency_in_seconds) + 3;

  // uint8_t div16th_margin = (((md_div32th_latency + a4_div32th_latency)) / 2)
  // + 4;
  uint8_t div32th_margin = 8;
  // DEBUG_PRINTLN(md_div32th_latency);
  // DEBUG_PRINTLN(a4_div32th_latency);

  uint32_t div32th_counter;
  if (grid_page.chain_enabled == 0) {
    return;
  }
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  uint8_t curkit;
  ElektronDataToSysexEncoder encoder(&MidiUart);

  //! MidiClock.clock_less_than(a,b) == !(a < b) == (a >= b)

  if (!MidiClock.clock_less_than(MidiClock.div32th_counter + div32th_margin,
                                 (uint32_t)mcl_actions.nearest_step * 2)) {
    //   while ((MidiClock.div32th_counter != (mcl_actions.nearest_step * 2 -
    //   div32th_latency)) && (MidiClock.state == 2));
    //
    DEBUG_PRINTLN(div32th_per_second * md_latency_in_seconds);
    DEBUG_PRINTLN("timing");
    DEBUG_PRINTLN(MidiClock.div32th_counter);
    DEBUG_PRINTLN(MidiClock.div32th_counter + 8);
    DEBUG_PRINTLN(mcl_actions.nearest_step * 2);

    // MD.getCurrentKit();
    // MD.getBlockingKit(MD.currentKit);
    // mcl_actions.md_setsysex_recpos(4, MD.currentKit);
    div32th_counter = MidiClock.div32th_counter + div32th_margin;
  } else {
    return;
  }

  GUI.removeTask(&grid_task);

  for (uint8_t n = 0; n < 20; n++) {
    slots_changed[n] = 0;
    if ((grid_page.active_slots[n] >= 0) && (mcl_actions.chains[n].loops > 0)) {
      if (n < 16) {

        uint32_t next_transition = (uint32_t)mcl_actions.nearest_steps[n] * 2;
        if (!MidiClock.clock_less_than(div32th_counter, next_transition)) {

          md_track->load_from_mem(n);
          if (slots_loaded[n] == 0) {
            //        md_track->place_track_in_kit(n, n, &(MD.kit));
          }

          grid_page.active_slots[n] = mcl_actions.chains[n].row;
          memcpy(&mcl_actions.chains[n], &md_track->chain, sizeof(GridChain));

          send_md_kit = true;
          slots_changed[n] = 1;
        }
      } else {
        uint32_t next_transition = (uint32_t)mcl_actions.nearest_steps[n] * 2;

        if (!MidiClock.clock_less_than(div32th_counter, next_transition)) {

          grid_page.active_slots[n] = mcl_actions.chains[n].row;
          memcpy(&mcl_actions.chains[n], &ext_track->chain, sizeof(GridChain));
          slots_changed[n] = 1;
          send_a4_sound = true;
        }
      }
    }
  }
  if (send_a4_sound) {
    DEBUG_PRINTLN("waiting to send a4");
    DEBUG_PRINTLN(MidiClock.div192th_counter);
    DEBUG_PRINTLN(mcl_actions.a4_latency);
    DEBUG_PRINTLN(a4_div192th_latency);
    DEBUG_PRINTLN(mcl_actions.nearest_step * 12 - a4_div192th_latency);

    uint32_t go_step = mcl_actions.nearest_step * 12 - md_div192th_latency -
                       a4_div192th_latency;
    uint32_t diff;
    if (mcl_actions.a4_latency > 0) {
      while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter, go_step)) != 0) &&
             (MidiClock.div192th_counter < go_step) && (MidiClock.state == 2)) {
        if (diff > 8) {

          handleIncomingMidi();
          GUI.loop();
        }
      }
      // in_sysex2 = 1;
    }
    for (uint8_t n = 16; n < 20; n++) {
      if (slots_changed[n] > 0) {
        a4_track->load_from_mem(n);
        DEBUG_PRINTLN(mcl_actions.a4_latency);
        if (mcl_actions.a4_latency > 0) {
          DEBUG_PRINTLN("here");
          if (a4_track->active == A4_TRACK_TYPE) {
            DEBUG_PRINTLN("send a4 sound");
            a4_track->sound.toSysex();
          }
        }
        mcl_seq.ext_tracks[n - 16].buffer_notesoff();
        a4_track->load_seq_data(n - 16);
        mcl_seq.ext_tracks[n - 16].step_count = 0;
        mcl_seq.ext_tracks[n - 16].start_clock32th =
            mcl_actions.nearest_step * 2;
      }
    }
  }
  if (send_md_kit) {
    DEBUG_PRINTLN(MidiClock.div192th_counter);
    DEBUG_PRINTLN(md_div192th_latency);
    DEBUG_PRINTLN(mcl_actions.nearest_step * 12 - md_div192th_latency);
    uint32_t go_step = mcl_actions.nearest_step * 12 - md_div192th_latency;
    uint32_t diff;
    while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter, go_step)) != 0) &&
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

    //   MD.kit.toSysex(encoder);
    //    DEBUG_PRINTLN("what we have left");
    //  DEBUG_PRINTLN(MidiClock.div32th_counter);
    //   DEBUG_PRINTLN(MidiClock.div32th_counter);
    //   DEBUG_PRINTLN(MidiClock.mod12_counter);
    //   while ((MidiClock.mod12_counter != 11) && (MidiClock.state == 2)) {

    // }
    //  md_exploit.off();
    //   MD.loadKit(MD.currentKit);
    // md_exploit.on();
    //   DEBUG_PRINTLN("took this long");
    //  DEBUG_PRINTLN(clock_diff(div192th_counter_old,
    //  MidiClock.div192th_counter));

    // DEBUG_PRINTLN("loading md kit");
    for (uint8_t n = 0; n < 16; n++) {

      if (slots_changed[n] > 0) {
        md_track->load_from_mem(n);
        if (md_track->active == MD_TRACK_TYPE) {
          uint8_t trigGroup = md_track->machine.trigGroup;
          if ((trigGroup < 16) && (trigGroup != n) &&
              (slots_loaded[trigGroup] == 0)) {
            md_track->load_from_mem(trigGroup);
            if (md_track->active == MD_TRACK_TYPE) {

              mcl_actions.md_set_machine(trigGroup, &(md_track->machine),
                                         &(MD.kit));
              md_track->place_track_in_kit(trigGroup, trigGroup, &(MD.kit),
                                           false);
            }
            md_track->load_from_mem(n);
            slots_loaded[trigGroup] = 1;
          }
          if (slots_loaded[n] == 0) {
            mcl_actions.md_set_machine(n, &(md_track->machine), &(MD.kit));
            md_track->place_track_in_kit(n, n, &(MD.kit), false);
            slots_loaded[n] = 1;
          }

          mcl_seq.md_tracks[n].start_step = mcl_actions.nearest_step;
          mcl_seq.md_tracks[n].mute_until_start = true;

          md_track->load_seq_data(n);
          // DEBUG_PRINT("THIS ");

          //  DEBUG_PRINTLN(mcl_actions.nearest_step -
          //  MidiClock.div16th_counter);
          //   DEBUG_PRINT(n);
          // DEBUG_PRINT(" ");
          //  DEBUG_PRINTLN(MidiClock.div16th_counter);
          // DEBUG_PRINTLN(mcl_actions.nearest_step);
          // mcl_seq.md_tracks[n].step_count = 0;
        }
      }

      else {
        if (mcl_actions.chains[n].loops == 0) {
          DEBUG_PRINTLN("clearing track");
          bool clear_locks = true;
          mcl_seq.md_tracks[n].clear_track(clear_locks);
        }
      }
    }
    //  DEBUG_PRINTLN("step counts");
    //   for (uint8_t n = 0; n < 16; n++) {
    //   DEBUG_PRINT(mcl_seq.md_tracks[n].step_count); DEBUG_PRINT(" ");
    //  }
    //  DEBUG_PRINTLN(MidiClock.div16th_counter);
    // in_sysex = 0;
    // in_sysex2 = 0;
    // DEBUG_PRINTLN("step_counts");
    DEBUG_PRINTLN("took this long");
    DEBUG_PRINTLN(clock_diff(div192th_counter_old, MidiClock.div192th_counter));
  }
  if (send_md_kit || send_a4_sound) {
    uint8_t count = 0;
    for (uint8_t n = 0; n < 20; n++) {
      if (slots_changed[n] == 1) {

        if (count % 8 == 0) {
          handleIncomingMidi();
          GUI.loop();
        }
        count++;
        if (n < 16) {
          //          DEBUG_PRINTLN("trying to cache MD track");
          //         DEBUG_PRINTLN(n);
          //       DEBUG_PRINTLN(mcl_actions.chains[n].row);
          int32_t len =
              sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);
          if (md_track->load_track_from_grid(n, mcl_actions.chains[n].row,
                                             len)) {
            //  DEBUG_PRINTLN("storing");
            md_track->store_in_mem(n);
          } else {
            DEBUG_PRINTLN("failed");
          }
        } else {
          DEBUG_PRINTLN("trying to load a4 track");
          DEBUG_PRINTLN(n);
          DEBUG_PRINTLN(mcl_actions.chains[n].row);
          if (a4_track->load_track_from_grid(n, mcl_actions.chains[n].row, 0)) {
            a4_track->store_in_mem(n);
          }
        }

        mcl_actions.calc_nearest_slot_step(n);
      }
    }
    mcl_actions.calc_nearest_step();
    mcl_actions.calc_latency(&empty_track);
  }
  GUI.addTask(&grid_task);
}
/*
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


  uint8_t slots_changed[20];
  uint8_t slots_loaded[16];

  for (uint8_t i = 0; i < 16; i++) {
  slots_loaded[i] = 0;
  }

  // 1 / ((tempo / 60) * 4)
  // bytes per second = MidiUart.speed / 10
  // latency / bytes_per_second  = time.
  //
  //

  // 32150 bps. one byte is actually 10 bits, ( 8 data bits + 1 start +  1 stop
  // bit)
  //
  bool proceed = false;
  uint32_t speed = MidiUart.speed;

  float bytes_per_second = (float)MidiUart.speed / (float)10;
  float latency_in_seconds = (float)mcl_actions.latency / bytes_per_second;
  float div32th_per_second = ((float)MidiClock.tempo / (float)60) * (float)8;
 // DEBUG_PRINTLN(div32th_per_second * latency_in_seconds);
  uint8_t div32th_latency = div32th_per_second * latency_in_seconds + 1;
 // DEBUG_PRINTLN("latency in steps");
 // DEBUG_PRINTLN(div32th_latency);
 //
 // div32th_latency = 6;
  DEBUG_PRINTLN("latency");
  DEBUG_PRINTLN(div32th_latency);

  uint32_t div32th_counter;
  if (!active) { return; }
  if (MidiClock.div16th_counter + 4 >= mcl_actions.nearest_step) {
DEBUG_PRINTLN("check...");
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(mcl_actions.nearest_step);
          while ((MidiClock.div32th_counter != (mcl_actions.nearest_step * 2 -
div32th_latency)) && (MidiClock.state == 2));

  div32th_counter = MidiClock.div32th_counter;
  }
  else {
    return;
  }

  GUI.removeTask(&grid_task);
  for (uint8_t n = 0; n < 20; n++) {
    slots_changed[n] = 0;

    if (mcl_actions.chains[n].active > 0) {
      if (n < 16) {

        uint32_t len = mcl_actions.chains[n].loops * mcl_seq.md_tracks[n].length
* 2; uint32_t next_transition = mcl_actions.nearest_steps[n] * 2 -
div32th_latency; if (div32th_counter >= next_transition) {

          md_track->load_from_mem(n);
          if (md_track->active != MD_TRACK_TYPE) {
            DEBUG_PRINTLN("shit");
          }

            mcl_seq.disable();
          if (md_track->active != EMPTY_TRACK_TYPE) {
            uint8_t trigGroup = md_track->machine.trigGroup;
            if ((trigGroup < 16) && (trigGroup != n)) {
              md_track->load_from_mem(trigGroup);
               if (md_track->active == MD_TRACK_TYPE) {
              mcl_actions.md_set_machine(trigGroup, &(md_track->machine),
&(MD.kit)); md_track->place_track_in_kit(trigGroup, trigGroup, &(MD.kit));
            //mcl_seq.md_tracks[trigGroup].mute_until_zero = true;
               }
              md_track->load_from_mem(n);
              slots_loaded[trigGroup] = 1;
            }
            //  mcl_seq.md_tracks[n].mute();
            if (slots_loaded[n] == 0) {
            mcl_actions.md_set_machine(n, &(md_track->machine), &(MD.kit));
            md_track->place_track_in_kit(n, n, &(MD.kit));

            }
            md_track->load_seq_data(n);
            //mcl_seq.md_tracks[n].mute_until_zero = true;
          //  mcl_seq.md_tracks[n].update_params();
            mcl_seq.enable();
            //   mcl_seq.md_tracks[n].unmute();
            //    DEBUG_PRINTLN(MidiClock.div16th_counter);
          } else {
            bool clear_locks = true;
            mcl_seq.md_tracks[n].clear_track(clear_locks);
          }

          grid_page.active_slots[n] = mcl_actions.chains[n].row;
          memcpy(&mcl_actions.chains[n], &md_track->chain, sizeof(GridChain));
          proceed = true;
          slots_changed[n] = 1;
        }
      } else {
        if ((((div32th_counter + 3 - mcl_actions.start_clock16th)) %
             (mcl_actions.chains[n].loops * mcl_seq.ext_tracks[n - 16].length))
== 0) { DEBUG_PRINTLN("not sure what is happening here");
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

          grid_page.active_slots[n] = mcl_actions.chains[n].row;
          memcpy(&mcl_actions.chains[n], &ext_track->chain, sizeof(GridChain));
          slots_changed[n] = 1;
        }
      }
    }
  }
  if (proceed) {
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
        if (md_track->load_track_from_grid(n, mcl_actions.chains[n].row, len)) {
          md_track->store_in_mem(n);
        }
      } else {
        DEBUG_PRINTLN(mcl_actions.chains[n].row);
        if (a4_track->load_track_from_grid(n, mcl_actions.chains[n].row, 0)) {
          a4_track->store_in_mem(n);
        }
      }
    }
  }
    mcl_actions.calc_latency();
  }
  GUI.addTask(&grid_task);
}
/*
void GridTask::run() {
  if (MidiClock.state != 2) {
    return;
  }
  EmptyTrack empty_track;

  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  uint32_t div16th_counter = MidiClock.div16th_counter;
  uint32_t earliest_change = 255;
  uint8_t slots_changed[20];
  GUI.removeTask(&grid_task);
  bool kit_ready = false;
  for (uint8_t n = 0; n < 20; n++) {
    slots_changed[n] = 0;

    if (mcl_actions.chains[n].active > 0) {
      if (n < 16) {
        if ((((div16th_counter + 4 - mcl_actions.start_clock16th)) %
             (mcl_actions.chains[n].loops * mcl_seq.md_tracks[n].length)) == 0)
{ earliest_change = mcl_actions.chains[n].loops * mcl_seq.md_tracks[n].length;
          DEBUG_PRINT("loading ");
          DEBUG_PRINTLN(n);
          md_track->load_from_mem(n);
          if (md_track->active != MD_TRACK_TYPE) {
            DEBUG_PRINTLN("shit");
          }
          if (md_track->active != EMPTY_TRACK_TYPE) {

//            mcl_seq.disable();

            md_track->place_track_in_kit(n, n, &(MD.kit));
//            mcl_seq.enable();
            //   mcl_seq.md_tracks[n].unmute();
            //    DEBUG_PRINTLN(MidiClock.div16th_counter);
          } else {
            bool clear_locks = true;
            mcl_seq.md_tracks[n].clear_track(clear_locks);
          }

         kit_ready = true;
          slots_changed[n] = 1;
        }
      }
    }
  }

  if (kit_ready) {
    DEBUG_PRINTLN("kit ready");
    uint8_t curkit = MD.getCurrentKit();
    MD.getBlockingKit(curkit);

    ElektronDataToSysexEncoder encoder(&MidiUart);

    mcl_actions.md_setsysex_recpos(4, curkit);
    MD.kit.toSysex(encoder);

    while ((MidiClock.div16th_counter + 1 - mcl_actions.start_clock16th) %
earliest_change != 0) { handleIncomingMidi(); GUI.loop();
    }
    mcl_seq.disable();
    for (uint8_t n = 0; n < 16; n++) {
      if (mcl_actions.chains[n].active > 0) {
        md_track->load_from_mem(n);
        md_track->load_seq_data(n);
        mcl_seq.md_tracks[n].update_params();
        grid_page.active_slots[n] = mcl_actions.chains[n].row;
        memcpy(&mcl_actions.chains[n], &md_track->chain, sizeof(GridChain));
      }
    }
    MD.loadKit(curkit);
    mcl_seq.enable();

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
          if (md_track->load_track_from_grid(n, mcl_actions.chains[n].row, len))
{ md_track->store_in_mem(n);
          }
        } else {
          DEBUG_PRINTLN(mcl_actions.chains[n].row);
          if (a4_track->load_track_from_grid(n, mcl_actions.chains[n].row, 0)) {
            a4_track->store_in_mem(n);
          }
        }
      }
    }
  }
  GUI.addTask(&grid_task);
}
*/

GridTask grid_task(0);
