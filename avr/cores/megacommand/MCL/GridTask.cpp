
#include "MCL_impl.h"

#define DIV16_MARGIN 8

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::destroy() {}

void GridTask::gui_update() {
  auto &active_track = mcl_seq.md_tracks[last_md_track];
  if (GUI.currentPage() == &seq_step_page &&
      IS_BIT_SET(MDSeqTrack::sync_cursor, last_md_track)) {
    MD.sync_seqtrack(active_track.length, active_track.speed,
                     active_track.length - 1);
  }
  grid_page.set_active_row(last_active_row); // send led update
  MD.draw_pattern_idx(last_active_row, next_active_row, chain_behaviour);
}

void GridTask::run() {
  //  DEBUG_PRINTLN(MidiClock.div32th_counter / 2);
  //  A4Track *a4_track = (A4Track *)&temp_track;
  //   ExtTrack *ext_track = (ExtTrack *)&temp_track;
  clearLed2();
  if (load_row != 255) {
    grid_load_page.group_load(load_row);
    load_row = 255;
  }

  if ((midi_load) && (clock_diff(midi_event_clock, clock) > 60)) {
    uint8_t track_select[NUM_SLOTS] = {0};
    uint8_t r = 255;
    DEBUG_PRINTLN("process midi load");
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if (midi_track_select[n] < 128) {
        track_select[n] = 1;
      }
    }
    mcl_actions.write_original = 1;
    mcl_actions.load_tracks(r, track_select, midi_track_select);
    midi_load = false;
  }

  if (stop_hard_callback) {
    mcl_actions_callbacks.StopHardCallback();
    stop_hard_callback = false;
    return;
  }
  // MD GUI update.

  if (MDSeqTrack::sync_cursor) {
    if (MidiClock.state == 2) {
      if (last_active_row < GRID_LENGTH) {
        gui_update();
        MD.setKitName(kit_names[0]);
      }
    }
    MDSeqTrack::sync_cursor = 0;
  }

  GridTask::transition_handler();
}

void GridTask::transition_handler() {

  if (MidiClock.state != 2 || mcl_actions.next_transition == (uint16_t)-1) {
    return;
  }
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  ElektronDevice *elektron_devs[2] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
  };

  bool send_device[2] = {0};

  uint8_t slots_changed[NUM_SLOTS];
  uint8_t track_select_array[NUM_SLOTS] = {0};

  bool send_ext_slots = false;
  bool send_md_slots = false;

  uint8_t div32th_margin = 8;

  uint32_t div32th_counter;

  // Get within four 16th notes of the next transition.
  if (MidiClock.clock_less_than(MidiClock.div32th_counter + div32th_margin,
                                (uint32_t)mcl_actions.next_transition * 2) <=
      0) {

    DEBUG_PRINTLN(F("Preparing for next transition:"));
    DEBUG_PRINTLN(MidiClock.div16th_counter);
    DEBUG_PRINTLN(mcl_actions.next_transition);
    // Transition window
    div32th_counter = MidiClock.div32th_counter + div32th_margin;
  } else {
    return;
  }

  DEBUG_PRINTLN((int)SP);
  GUI.removeTask(&grid_task);
  uint8_t track_idx, dev_idx;

  uint8_t row = 255;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    slots_changed[n] = 255;

    if ((mcl_actions.links[n].loops == 0) ||
        (grid_page.active_slots[n] == SLOT_DISABLED))
      continue;

    uint32_t next_transition = (uint32_t)mcl_actions.next_transitions[n] * 2;

    // If next_transition[n] is less than or equal to transition window, then
    // flag track for transition.
    if (MidiClock.clock_less_than(div32th_counter, next_transition))
      continue;

    //    if ((mcl_actions.links[n].row != grid_page.active_slots[n]) ||
    //        (mcl_actions.chains[n].mode == LOAD_MANUAL)) {

    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(n, &track_idx, &dev_idx);

    if (gdt == nullptr) {
      continue;
    }

    if (link_load(n, track_idx, slots_changed, track_select_array, gdt)) {
      send_device[dev_idx] = true;
    }

    //  if (mcl_actions.chains[n].mode == LOAD_MANUAL) {
    if (row == 255) {
      row = slots_changed[n];
    }
    //    mcl_actions.links[n].loops = 0;
    //  }
  }

  DEBUG_PRINTLN(F("sending tracks"));
  bool wait;

  if (mcl_cfg.uart2_prg_out > 0 && row != 255) {
    MidiUart2.sendProgramChange(mcl_cfg.uart2_prg_out - 1, row);
  }

  for (int8_t c = NUM_DEVS - 1; c >= 0; c--) {
    wait = true;

    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if (slots_changed[n] == 255)
        continue;

      GridDeviceTrack *gdt =
          mcl_actions.get_grid_dev_track(n, &track_idx, &dev_idx);
      if ((gdt == nullptr) || (dev_idx != c))
        continue;

      // Wait on first track of each device;

      if (wait && send_device[c]) {

        uint32_t go_step = mcl_actions.next_transition * 12 -
                           mcl_actions.div192th_total_latency - 1;

        mcl_actions.div192th_total_latency -=
            mcl_actions.dev_latency[dev_idx].latency;

        uint32_t diff;

        float tempo = MidiClock.get_tempo();
        float div192th_per_second = tempo * 0.8f;

        while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter,
                                                    go_step)) != 0) &&
               (MidiClock.div192th_counter < go_step) &&
               (MidiClock.state == 2)) {
          if ((float)diff * div192th_per_second > 0.160) {
            handleIncomingMidi();
            if (GUI.currentPage() == &grid_load_page) {
              GUI.display();
            } else {
              GUI.loop();
            }
          }
        }
      }
      wait = false;
      if (transition_load(n, track_idx, dev_idx, gdt)) {
        grid_page.active_slots[n] = slots_changed[n];
      }
    }
  }
  DEBUG_PRINTLN(F("SP pre cache"));
  DEBUG_PRINTLN((int)SP);

  bool update_gui = true;
  mcl_actions.cache_next_tracks(track_select_array, update_gui);
 
  // Once tracks are cached, we can calculate their next transition
  uint8_t last_slot = 255;
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (track_select_array[n] > 0) {
      mcl_actions.calc_next_slot_transition(n);
      last_slot = n;
    }
  }

  if (last_slot != 255 && slots_changed[last_slot] < GRID_LENGTH) {
    // GridDeviceTrack *gdt =
    //    mcl_actions.get_grid_dev_track(last_slot, &track_idx, &dev_idx);
    last_active_row = slots_changed[last_slot];
    next_active_row = mcl_actions.links[last_slot].row;
    chain_behaviour = mcl_actions.chains[last_slot].mode > 1;

    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(last_slot, &track_idx, &dev_idx);

    GridRowHeader row_header;

    proj.read_grid_row_header(&row_header, last_active_row);
    dev_idx = 0;

    if (elektron_devs[dev_idx]) {
      uint8_t len = elektron_devs[dev_idx]->sysex_protocol.kitname_length;

      if (row_header.active) {
        memcpy(kit_names[dev_idx], row_header.name, len);
        kit_names[dev_idx][len - 1] = '\0';
      } else {
        strcpy(kit_names[dev_idx], "NEW_KIT");
      }
    }
  }

  mcl_actions.calc_next_transition();
  mcl_actions.calc_latency();
//  } else {
//    mcl_actions.next_transition = (uint16_t)-1;
//  }
end:
  GUI.addTask(&grid_task);
}

bool GridTask::link_load(uint8_t n, uint8_t track_idx, uint8_t *slots_changed,
                         uint8_t *track_select_array, GridDeviceTrack *gdt) {
  EmptyTrack empty_track;
  auto *pmem_track =
      empty_track.load_from_mem(gdt->mem_slot_idx, gdt->track_type);
  if (pmem_track == nullptr) {
    return false;
  }
  slots_changed[n] = mcl_actions.links[n].row;
  track_select_array[n] = 1;
  memcpy(&mcl_actions.links[n], &pmem_track->link, sizeof(GridLink));
  if (pmem_track->active) {
    return true;
  }
  return false;
}
bool GridTask::transition_load(uint8_t n, uint8_t track_idx, uint8_t dev_idx,
                               GridDeviceTrack *gdt) {
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  ElektronDevice *elektron_devs[2] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
  };

  EmptyTrack empty_track;

  auto *pmem_track =
      empty_track.load_from_mem(gdt->mem_slot_idx, gdt->track_type);

  if (pmem_track == nullptr)
    return false;

  gdt->seq_track->count_down = -1;
  if (mcl_actions.send_machine[n] == 0) {
    pmem_track->transition_send(track_idx, n);
    if (mcl_actions.dev_sync_slot[dev_idx] == n) {
      if (elektron_devs[dev_idx]) {
        elektron_devs[dev_idx]->undokit_sync();
      }
      mcl_actions.dev_sync_slot[dev_idx] = -1;
    }
  }

  pmem_track->transition_load(track_idx, gdt->seq_track, n);
  return true;
}

GridTask grid_task(0);
