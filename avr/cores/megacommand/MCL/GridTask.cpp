#include "MCL_impl.h"

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

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  bool send_device[2] = {0};

  int slots_changed[NUM_SLOTS];

  bool send_ext_slots = false;
  bool send_md_slots = false;

  uint8_t div32th_margin = 8;

  uint32_t div32th_counter;
  if ((mcl_cfg.chain_mode == 0) ||
      (mcl_actions.next_transition == (uint16_t)-1)) {
    return;
  }

  // Get within four 16th notes of the next transition.
  if (!MidiClock.clock_less_than(MidiClock.div32th_counter + div32th_margin,
                                 (uint32_t)mcl_actions.next_transition * 2)) {

    DEBUG_PRINTLN("Preparing for next transition:");
    DEBUG_DUMP(MidiClock.div16th_counter);
    DEBUG_DUMP(mcl_actions.next_transition);
    // Transition window
    div32th_counter = MidiClock.div32th_counter + div32th_margin;
  } else {
    return;
  }

  GUI.removeTask(&grid_task);

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    slots_changed[n] = -1;

    if ((mcl_actions.chains[n].loops == 0) || (grid_page.active_slots[n] < 0))
      continue;

    uint32_t next_transition = (uint32_t)mcl_actions.next_transitions[n] * 2;

    // If next_transition[n] is less than or equal to transition window, then
    // flag track for transition.
    if (MidiClock.clock_less_than(div32th_counter, next_transition))
      continue;

    slots_changed[n] = mcl_actions.chains[n].row;
    if ((mcl_actions.chains[n].row != grid_page.active_slots[n]) ||
        (mcl_cfg.chain_mode == 2)) {

      uint8_t grid_col = n;
      uint8_t grid = 0;
      // GRID 1
      if (n < GRID_WIDTH) {
        grid = 0;
      }
      // GRID 2
      else {
        grid = 1;
        grid_col -= GRID_WIDTH;
      }

      auto *pmem_track =
          empty_track.load_from_mem(grid_col, devs[grid]->track_type);
      if (pmem_track != nullptr) {
        slots_changed[n] = mcl_actions.chains[n].row;
        memcpy(&mcl_actions.chains[n], &pmem_track->chain, sizeof(GridChain));
        if (pmem_track->active) {
          send_device[grid] = true;
        }
      }
    }
    // Override chain data if in manual or random mode
    if (mcl_cfg.chain_mode == 2) {
      mcl_actions.chains[n].loops = 0;
    } else if (mcl_cfg.chain_mode == 3) {
      mcl_actions.chains[n].loops = random(1, 8);
      mcl_actions.chains[n].row =
          random(mcl_cfg.chain_rand_min, mcl_cfg.chain_rand_max);
    }
  }
  if (send_device[1]) {

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
    }

    for (uint8_t n = NUM_MD_TRACKS; n < NUM_SLOTS; n++) {
      if (slots_changed[n] >= 0) {
        auto ext_track = empty_track.load_from_mem<ExtTrack>(n - NUM_MD_TRACKS);
        DEBUG_DUMP(mcl_actions.a4_latency);

        if (ext_track->active != EMPTY_TRACK_TYPE) {
          ext_track->chain_load(n - NUM_MD_TRACKS);

        } else {

          DEBUG_PRINTLN("clearing ext track");
          mcl_seq.ext_tracks[n - NUM_MD_TRACKS].clear_track();
        }

        grid_page.active_slots[n] = slots_changed[n];
      }
    }
  }

  if (send_device[0]) {
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
    DEBUG_DUMP(MidiClock.div16th_counter);
    // in_sysex = 1;
    uint32_t div192th_counter_old = MidiClock.div192th_counter;
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {

      if (slots_changed[n] >= 0) {
        auto md_track = empty_track.load_from_mem<MDTrack>(n);
        if (md_track) {
          md_track->chain_load(n);
        } else {
          DEBUG_PRINTLN("clearing track");
          DEBUG_DUMP(n);
          bool clear_locks = true;
          bool reset_params = false;
          mcl_seq.md_tracks[n].clear_track(clear_locks, reset_params);
        }

        grid_page.active_slots[n] = slots_changed[n];
      }
    }
  }
  uint8_t count = 0;
  uint8_t slots_cached[NUM_SLOTS] = {0};

  EmptyTrack empty_track2;

  uint8_t old_grid = proj.get_grid();

  if (mcl_cfg.chain_mode != 2) {

    uint8_t track_select_array[NUM_SLOTS] = {0};

    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if ((slots_changed[n] >= 0) &&
          (slots_changed[n] != mcl_actions.chains[n].row)) {
        track_select_array[n] = 1;
      }
    }
    bool update_gui = true;
    mcl_actions.cache_next_tracks(track_select_array, &empty_track,
                                  &empty_track2, update_gui);

    // Once tracks are cached, we can calculate their next transition
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if (track_select_array[n] > 0) {
        mcl_actions.calc_next_slot_transition(n);
      }
    }

    mcl_actions.calc_next_transition();
    mcl_actions.calc_latency(&empty_track);
  } else {
    mcl_actions.next_transition = (uint16_t)-1;
  }
  GUI.addTask(&grid_task);
}

GridTask grid_task(0);
