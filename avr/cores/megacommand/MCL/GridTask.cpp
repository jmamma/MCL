#include "MCL_impl.h"

#define DIV16_MARGIN 8

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::destroy() {}

void GridTask::run() {
  //  DEBUG_PRINTLN(MidiClock.div32th_counter / 2);
  //  A4Track *a4_track = (A4Track *)&temp_track;
  //  ExtTrack *ext_track = (ExtTrack *)&temp_track;

  if (stop_hard_callback) {
    mcl_actions_callbacks.StopHardCallback();
    stop_hard_callback = false;
    return;
  }

  GridTask::transition_handler();
}

void GridTask::transition_handler() {

  if (MidiClock.state != 2 || mcl_actions.next_transition == (uint16_t)-1) {
    return;
  }

  bool send_device[2] = {0};

  uint8_t slots_changed[NUM_SLOTS];
  uint8_t track_select_array[NUM_SLOTS] = {0};

  bool send_ext_slots = false;
  bool send_md_slots = false;

  uint8_t div32th_margin = 8;

  uint32_t div32th_counter;

  // Get within four 16th notes of the next transition.
  if (MidiClock.clock_less_than(MidiClock.div32th_counter + div32th_margin,
                                 (uint32_t)mcl_actions.next_transition * 2) <= 0) {

    DEBUG_PRINTLN(F("Preparing for next transition:"));
    DEBUG_DUMP(MidiClock.div16th_counter);
    DEBUG_DUMP(mcl_actions.next_transition);
    // Transition window
    div32th_counter = MidiClock.div32th_counter + div32th_margin;
  } else {
    return;
  }

  DEBUG_PRINTLN((int)SP);
  GUI.removeTask(&grid_task);
  int8_t last_active_row = -1;
  uint8_t track_idx, dev_idx;

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
    //        (mcl_actions.chains[n].mode == CHAIN_MANUAL)) {

    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(n, &track_idx, &dev_idx);

    if (gdt == nullptr) {
      continue;
    }

    if (link_load(n, track_idx, slots_changed, track_select_array, gdt)) {
      send_device[dev_idx] = true;
    }

    //  if (mcl_actions.chains[n].mode == CHAIN_MANUAL) {
    //    mcl_actions.links[n].loops = 0;
    //  }
  }

  DEBUG_PRINTLN(F("sending tracks"));
  bool wait;
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

        while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter,
                                                    go_step)) != 0) &&
               (MidiClock.div192th_counter < go_step) &&
               (MidiClock.state == 2)) {
          if (diff > 8) {
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
  //  if (mcl_cfg.chain_mode != CHAIN_MANUAL) {
  DEBUG_PRINTLN("gettin ready to cache");
  DEBUG_PRINTLN((int)SP);
  bool update_gui = true;
  mcl_actions.cache_next_tracks(track_select_array, update_gui);
  // Once tracks are cached, we can calculate their next transition
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (track_select_array[n] > 0) {
      mcl_actions.calc_next_slot_transition(n);
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

bool GridTask::link_load(uint8_t n, uint8_t track_idx, uint8_t *slots_changed, uint8_t *track_select_array, GridDeviceTrack *gdt) {
  EmptyTrack empty_track;

  auto *pmem_track =
      empty_track.load_from_mem(gdt->mem_slot_idx, gdt->track_type);
  if (pmem_track == nullptr) { return false; }
  slots_changed[n] = mcl_actions.links[n].row;
  track_select_array[n] = 1;
  memcpy(&mcl_actions.links[n], &pmem_track->link, sizeof(GridLink));
  if (pmem_track->active) {
    return true;
  }
  return false;
}
bool GridTask::transition_load(uint8_t n, uint8_t track_idx, uint8_t dev_idx, GridDeviceTrack *gdt) {
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
