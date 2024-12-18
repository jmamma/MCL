#include "GridTask.h"
#include "GridPage.h"
#include "MD.h"
#include "MCLActions.h"
#include "MDSeqTrack.h"

#define DIV16_MARGIN 8

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::destroy() {}

void GridTask::row_update() {
  grid_page.set_active_row(last_active_row); // send led update
  MD.draw_pattern_idx(last_active_row, next_active_row, chain_behaviour);
}

void GridTask::gui_update() {
  if (MDSeqTrack::gui_update) {
    if (MidiClock.state == 2) {
      if (mcl.currentPage() == SEQ_STEP_PAGE && IS_BIT_SET16(MDSeqTrack::gui_update,last_md_track)) {
        auto active_track = mcl_seq.md_tracks[last_md_track];
        MD.sync_seqtrack(active_track.length, active_track.speed, active_track.step_count);
      }
      if (last_active_row < GRID_LENGTH) {
        row_update();
      }
    }
    MDSeqTrack::gui_update = 0;
  }
}
void GridTask::load_queue_handler() {
  if (!load_queue.is_empty()) {
    uint8_t mode;
    uint8_t offset;
    uint8_t row_select_array[NUM_SLOTS];
    uint8_t track_select[NUM_SLOTS] = {0};
    load_queue.get(mode, offset, row_select_array);
    DEBUG_PRINTLN("load queue get");
    DEBUG_PRINTLN(mode);
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if (row_select_array[n] < 128) {
        track_select[n] = 1;
      }
      DEBUG_PRINT(n);
      DEBUG_PRINT(" ");
      DEBUG_PRINT(track_select[n]);
      DEBUG_PRINT(" ");
      DEBUG_PRINTLN(row_select_array[n]);
    }
    mcl_actions.write_original = 1;
    mcl_actions.load_tracks(255, track_select, row_select_array, mode, offset);
  }
}

void GridTask::run() {
  //  DEBUG_PRINTLN(MidiClock.div32th_counter / 2);
  //  A4Track *a4_track = (A4Track *)&temp_track;
  //   ExtTrack *ext_track = (ExtTrack *)&temp_track;
  // MD GUI update.

  perf_page.encoder_check();
  trig_interface.check_key_throttle();

  if (stop_hard_callback) {
    mcl_actions_callbacks.StopHardCallback();
    stop_hard_callback = false;
    load_queue.init();
    return;
  }

  gui_update();
  load_queue_handler();
  GridTask::transition_handler();
}

void GridTask::update_transition_details() {
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  ElektronDevice *elektron_devs[2] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
  };

  GridRowHeader row_header;
  proj.read_grid_row_header(&row_header, next_active_row);
  uint8_t dev_idx = 0;

  uint8_t len = elektron_devs[0]->sysex_protocol.kitname_length;

  if (row_header.active) {
    memcpy(kit_names[dev_idx], row_header.name, len);
    m_toupper(kit_names[dev_idx]);
    kit_names[dev_idx][len - 1] = '\0';
  } else {
    strcpy(kit_names[dev_idx], "NEW_KIT");
  }
}

void GridTask::transition_handler() {

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  bool send_device[2] = {0};

  uint8_t slots_changed[NUM_SLOTS];
  uint8_t track_select_array[NUM_SLOTS] = {0};

  bool send_ext_slots = false;
  bool send_md_slots = false;

  uint8_t div32th_margin = 6;

  GUI.removeTask(&grid_task);

  // 240ms headroom = 0.240 * (MidiClock.get_tempo()* 0.133333333333
  //                = 0.032 * MidiClock.get_tempo()
  //
  while (MidiClock.clock_less_than(
             MidiClock.div32th_counter + 0.032 * max(60.0,MidiClock.get_tempo()),
             (uint32_t)mcl_actions.next_transition * 2) <= 0) {

    float div32th_per_second = MidiClock.get_tempo() * 0.133333333333f;
    float div32th_time = 1.0 / div32th_per_second;

    if (MidiClock.state != 2 || mcl_actions.next_transition == (uint16_t)-1) {
      break;
    }
    DEBUG_PRINTLN(F("Preparing for next transition:"));
    DEBUG_PRINTLN(MidiClock.div16th_counter);
    DEBUG_PRINTLN(mcl_actions.next_transition);

    DEBUG_PRINTLN((int)SP);

    uint8_t row = 255;
    uint8_t last_slot = 255;
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      slots_changed[n] = 255;

      DEBUG_PRINTLN(n);
      DEBUG_PRINT(mcl_actions.next_transition);
      DEBUG_PRINT(" ");
      DEBUG_PRINTLN(mcl_actions.next_transitions[n]);

      if ((mcl_actions.links[n].loops == 0) ||
          (grid_page.active_slots[n] == SLOT_DISABLED) ||
          (mcl_actions.next_transition != mcl_actions.next_transitions[n]))
        continue;

      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(n);
      uint8_t track_idx = mcl_actions.get_track_idx(n);
      uint8_t device_idx = gdt->device_idx;

      if (gdt == nullptr) {
        continue;
      }

      if (link_load(n, track_idx, slots_changed, track_select_array, gdt)) {
        send_device[device_idx] = true;
      }

      if (row == 255) {
        row = slots_changed[n];
      }
    }

    if (send_device[0]) {
      //Send kitName before tracks are cache-loaded in MD.
      //This allows the kitName to be stored in the undokit.
      update_transition_details();
    }

    DEBUG_PRINTLN(F("sending tracks"));
    bool wait;

    if (mcl_cfg.uart2_prg_out > 0 && row != 255) {
      MidiUart2.sendProgramChange(mcl_cfg.uart2_prg_out - 1, row);
    }
    float tempo = MidiClock.get_tempo();
    // float div192th_per_second = tempo * 0.8f;
    // float div192th_time = 1.0 / div192th_per_second;
    // float div192th_time = 1.0 / (tempo * 0.8f);
    // diff * div19th_time > 0.08 equivalent to diff > (0.8 * 0.08) * tempo
    for (int8_t c = NUM_DEVS - 1; c >= 0; c--) {
      wait = true;

      for (uint8_t n = 0; n < NUM_SLOTS; n++) {
        if (slots_changed[n] == 255)
          continue;
        GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(n);
        uint8_t track_idx = mcl_actions.get_track_idx(n);
        uint8_t device_idx = gdt->device_idx;

        if (gdt == nullptr || (device_idx != c))
          continue;

        // Wait on first track of each device;

        if (wait && send_device[c]) {

          uint32_t go_step = mcl_actions.next_transition * 12 -
                             mcl_actions.div192th_total_latency - 1;

          mcl_actions.div192th_total_latency -=
              mcl_actions.dev_latency[device_idx].latency;

          uint32_t diff;

          while (((diff = MidiClock.clock_diff_div192(
                       MidiClock.div192th_counter, go_step)) != 0) &&
                 (MidiClock.div192th_counter < go_step) &&
                 (MidiClock.state == 2)) {
                MidiUartParent::handle_midi_lock = 1;
                handleIncomingMidi();
                MidiUartParent::handle_midi_lock = 0;
                if ((float)diff > tempo * 0.064f) { //0.8 * 0.08 = 0.128f
                   GUI.loop();
               }
          }
        }
        wait = false;
        if (transition_load(n, track_idx, gdt)) {
          if (grid_page.active_slots[n] != SLOT_OFFSET_LOAD) {
            grid_page.active_slots[n] = slots_changed[n];
          }
        }
      }
    }
    DEBUG_PRINTLN(F("SP pre cache"));
    DEBUG_PRINTLN((int)SP);

    bool update_gui = true;

    DEBUG_PRINTLN("cache next");

    volatile uint32_t clk = slowclock;
    mcl_actions.cache_next_tracks(track_select_array, update_gui);

    uint32_t t = clock_diff(clk, slowclock);
    DEBUG_PRINTLN("time");
    DEBUG_PRINTLN(t);


    // Once tracks are cached, we can calculate their next transition
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(n);

      if (gdt == nullptr) {
        continue;
      }

      bool ignore_chain_settings = true;
      bool auto_check = true;
      if (track_select_array[n] > 0) {
        last_slot = n;
        ignore_chain_settings = false;
        auto_check = false;
      } else if (mcl_actions.chains[n].mode == LOAD_AUTO &&
                 mcl_actions.links[n].loops == 0) {
        mcl_actions.next_transitions[n] = -1;
        continue;
      }
      mcl_actions.calc_next_slot_transition(n, ignore_chain_settings);
    }
    if (last_slot != 255 && slots_changed[last_slot] < GRID_LENGTH) {
      last_active_row = slots_changed[last_slot];
      next_active_row = mcl_actions.links[last_slot].row;
      chain_behaviour = mcl_actions.chains[last_slot].mode > 1;
    }

    gui_update();
    mcl_actions.calc_next_transition();
    mcl_actions.calc_latency();
  }
  GUI.addTask(&grid_task);
}

bool GridTask::link_load(uint8_t n, uint8_t track_idx, uint8_t *slots_changed,
                         uint8_t *track_select_array, GridDeviceTrack *gdt) {
  EmptyTrack empty_track;
  auto *pmem_track = empty_track.load_from_mem(
      gdt->mem_slot_idx, gdt->track_type, sizeof(GridTrack));
  if (pmem_track == nullptr) {
    return false;
  }
  slots_changed[n] = mcl_actions.links[n].row;
  track_select_array[n] = 1;

  pmem_track->link.store_in_mem(n, &(mcl_actions.links[0]));
  if (pmem_track->active) {
    return true;
  }
  return false;
}

bool GridTask::transition_load(uint8_t n, uint8_t track_idx,
                               GridDeviceTrack *gdt) {
  EmptyTrack empty_track;

  auto *pmem_track =
      empty_track.load_from_mem(gdt->mem_slot_idx, gdt->track_type);

  if (pmem_track == nullptr) {
    return false;
  }

  gdt->seq_track->count_down = -1;
  gdt->seq_track->load_sound = mcl_actions.send_machine[n];
  if (mcl_actions.send_machine[n] == 1) {
    pmem_track->transition_send(track_idx, n);
  }

  pmem_track->transition_load(track_idx, gdt->seq_track, n);
  return true;
}

GridTask grid_task(0);
