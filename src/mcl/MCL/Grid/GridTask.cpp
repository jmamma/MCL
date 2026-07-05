#include "Grid/GridTask.h"
#include "Devices/DeviceManager.h"
#include "EmptyTrack.h"
#include "GUI/Pages/Grid/GridPage.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "../Drivers/MD/MD.h"
#include "Grid/MCLActions.h"
#include "MDSeqTrack.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "MCLGUI.h"
#include "MCLPlatformFeatures.h"
#include "MCLSysConfig.h"
#include "Project.h"
#include "StackMonitor.h"
#include "Sequencer/MCLSeq.h"
#include "Sequencer/SeqTrackUtil.h"
#include "platform.h"
#if MCL_FEATURE_GRID_PRIVATE_LOADS || MCL_FEATURE_HOST_ARRANGER
#include "Arrangement/MCLArrangement.h"
#endif
#if MCL_FEATURE_HOST_ARRANGER
#include "Host/SpsHostArrBridge.h"  // SPS host arranger dirty notifications
#include "Host/SpsHostSeqBridge.h"  // SPS host step-grid dirty notifications
#endif

#define DIV16_MARGIN 8
#define GRIDTASK_PRE_CACHE_UI_MS 80

#if defined(__AVR__)
inline void GridTask::pre_cache_ui_yield() {
  static uint8_t last_pre_cache_ui_ms = 0;
  uint8_t pre_cache_ui_ms = (uint8_t)read_clock_ms();
  if ((uint8_t)(pre_cache_ui_ms - last_pre_cache_ui_ms) <
      GRIDTASK_PRE_CACHE_UI_MS) {
    return;
  }

  last_pre_cache_ui_ms = pre_cache_ui_ms;
  GUI.deferDisplayOnce();
  mcl.loop();
}
#endif

#if MCL_FEATURE_GRID_PRIVATE_LOADS
uint32_t GridTask::selected_track_mask(const uint8_t *track_select) {
  if (track_select == nullptr) {
    return 0;
  }
  uint32_t mask = 0;
  for (uint8_t n = 0; n < NUM_SLOTS && n < 32; ++n) {
    if (track_select[n] != 0) {
      mask |= (uint32_t)(1ul << n);
    }
  }
  return mask;
}

#endif

#if MCL_FEATURE_HOST_ARRANGER
static void ignore_current_step_for_arranger_preload(
    const uint8_t *track_select) {
  if (track_select == nullptr) {
    return;
  }
  for (uint8_t n = 0; n < NUM_SLOTS; ++n) {
    if (track_select[n] == 0) {
      continue;
    }
    uint8_t track = (uint8_t)(n & 0x0F);
    if (n < GRID_WIDTH) {
      if (track >= mcl_seq.num_md_tracks) {
        continue;
      }
#if !defined(__AVR__)
      if (mcl_seq.using_spsx_tracks) {
        mcl_seq.spsx_tracks[track].ignore_step =
            mcl_seq.spsx_tracks[track].step_count;
        continue;
      }
#endif
      mcl_seq.md_tracks[track].ignore_step =
          mcl_seq.md_tracks[track].step_count;
      continue;
    }
#ifdef EXT_TRACKS
#if !defined(__AVR__)
    if (SeqTrackUtil::use_midi_tracks_for_ext()) {
      if (track < mcl_seq.num_midi_tracks) {
        mcl_seq.midi_tracks[track].ignore_step =
            mcl_seq.midi_tracks[track].step_count;
      }
      continue;
    }
#endif
    if (track < mcl_seq.num_ext_tracks) {
      mcl_seq.ext_tracks[track].ignore_step =
          mcl_seq.ext_tracks[track].step_count;
    }
#endif
  }
}

bool GridTask::save_needs_md_current_pattern() {
#ifdef PLATFORM_TBD
  return MD.connected &&
         (device_manager.primary_device() == &MD ||
          device_manager.secondary_device() == &MD);
#else
  return true;
#endif
}

void GridTask::reset_host_playback_after_stop() {
  mcl_arrangement.flushRuntimePrivateSourceEdits();
  mcl_arrangement.resetPlayback(false);
}

void GridTask::tick_host_arranger() {
  mcl_arrangement.tick();
}

void GridTask::flush_host_automation_writes() {
  mcl_arrangement.flushAutomationWrites();
}

void GridTask::notify_host_seq_dirty_for_load(const uint8_t *track_select,
                                              const uint8_t *clear_select,
                                              GridSlot load_offset) {
  uint16_t mask = 0;
  GridSlot first_slot = 255;

  if (track_select != nullptr) {
    for (uint8_t n = 0; n < NUM_SLOTS; ++n) {
      if (track_select[n] == 0) {
        continue;
      }
      if (first_slot == 255) {
        first_slot = n;
      }
      GridSlot dst = n;
      if (load_offset < NUM_SLOTS) {
        int mapped = (int)n - (int)first_slot + (int)load_offset;
        if (mapped < 0 || mapped >= (int)NUM_SLOTS) {
          continue;
        }
        dst = (GridSlot)mapped;
      }
      if (dst < GRID_WIDTH) {
        mask |= (uint16_t)(1u << dst);
      }
    }
  }

  mask |= (uint16_t)(selected_track_mask(clear_select) & 0xFFFFul);
  sps_host_seq_bridge.notifyTracksDirty(
      mask, (uint8_t)(spsseq::DIRTY_SUMMARY | spsseq::DIRTY_DETAIL |
                      spsseq::DIRTY_LOCKS));
}

void GridActiveStateDirty::flush() {
  if (changed) {
    sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)spsarr::DIRTY_ACTIVE);
    changed = false;
  }
}
#endif

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::row_update() {
  if (last_active_row < GRID_LENGTH) {
    grid_page.set_active_row(last_active_row); // send led update
    MD.draw_pattern_idx(next_active_row, last_active_row, chain_behaviour);
  }
}

void GridTask::gui_update() {
  if (!update) { return; }
  row_update();
  update = 0;
}

#if MCL_FEATURE_GRID_SAVE_QUEUE
void GridTask::init_save_queue() {
  save_queue.init();
}

void GridTask::save_queue_handler() {
  if (save_queue.is_empty()) { return; }

  GridRow row = 255;
  uint8_t track_select[NUM_SLOTS];
  uint8_t merge = 0;
#if MCL_FEATURE_HOST_ARRANGER
  uint8_t ack_tag = 0;
  bool ack_valid = false;
  if (!save_queue.get(row, track_select, merge, &ack_tag, &ack_valid)) {
    return;
  }
#else
  if (!save_queue.get(row, track_select, merge)) {
    return;
  }
#endif

  if (row >= GRID_LENGTH) {
#if MCL_FEATURE_HOST_ARRANGER
    if (ack_valid) {
      sps_host_arr_bridge.ackSaveSlots(ack_tag, false);
    }
#endif
    return;
  }
  bool any = false;
  for (uint8_t n = 0; n < NUM_SLOTS; ++n) {
    if (track_select[n] != 0) {
      any = true;
      break;
    }
  }
  if (!any) {
#if MCL_FEATURE_HOST_ARRANGER
    if (ack_valid) {
      sps_host_arr_bridge.ackSaveSlots(ack_tag, false);
    }
#endif
    return;
  }

#if MCL_FEATURE_HOST_ARRANGER
  if (save_needs_md_current_pattern()) {
    MD.getCurrentPattern(500);
  }
  mcl_arrangement.flushRuntimePrivateSourceEdits();
#endif

  mcl_actions.save_tracks(row, track_select, merge);

#if MCL_FEATURE_HOST_ARRANGER
  if (ack_valid) {
    sps_host_arr_bridge.ackSaveSlots(ack_tag, true);
  }
  grid_page.row_scan = GRID_LENGTH;
  grid_page.reload_slot_models = false;
  sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)spsarr::DIRTY_CELLS);
#endif
}
#endif

#if defined(__AVR__)
void GridTask::load_queue_handler() {
  if (load_queue.is_empty()) { return; }

  uint8_t mode;
  GridSlot offset;
  GridRow *row_select_array;
  uint8_t track_select[NUM_SLOTS];

  row_select_array = load_queue.get(mode, offset, track_select);

  mcl_actions.write_original = 1;
  mcl_actions.load_tracks(track_select, row_select_array, mode, offset);
}
#else
void GridTask::load_queue_handler() {
  if (load_queue.is_empty()) { return; }

  uint8_t mode;
  GridSlot offset;
  GridRow row_select_array[NUM_SLOTS];
  uint8_t track_select[NUM_SLOTS];
  uint8_t clear_select[NUM_SLOTS];
  uint32_t private_source_ids[NUM_SLOTS];

  load_queue.get(mode, offset, row_select_array, track_select,
                 private_source_ids);
  bool immediate_load = (mode & LOAD_QUEUE_FLAG_IMMEDIATE) != 0;
  bool allow_prestart_fades = (mode & LOAD_QUEUE_FLAG_PRESTART_FADE) != 0;
  bool arranger_preload = (mode & LOAD_QUEUE_FLAG_ARRANGER_PRELOAD) != 0;
  mode &= (uint8_t)~(LOAD_QUEUE_FLAG_IMMEDIATE |
                     LOAD_QUEUE_FLAG_PRESTART_FADE |
                     LOAD_QUEUE_FLAG_ARRANGER_PRELOAD);
  memset(clear_select, 0, sizeof(clear_select));
  bool any_load = false;
  bool any_clear = false;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (track_select[n]) {
      any_load = true;
    }
    if (row_select_array[n] == LOAD_QUEUE_CLEAR_ROW) {
      clear_select[n] = 1;
      any_clear = true;
    }
  }
  if (any_load || any_clear) {
    mcl_arrangement.flushRuntimePrivateSourceEdits();
  }
  if (any_load) {
    mcl_actions.write_original = 1;
    mcl_arrangement.beginQueuedPrivateLoads(private_source_ids);
    mcl_actions.load_tracks(track_select, row_select_array, mode, offset,
                            immediate_load, allow_prestart_fades);
    mcl_arrangement.endQueuedPrivateLoads();
    if (arranger_preload) {
      ignore_current_step_for_arranger_preload(track_select);
    }
  }
  if (any_clear) {
    if (mode != LOAD_ARRANG) {
      if (mcl_arrangement.releasePlaybackTracks(
              selected_track_mask(clear_select))) {
        sps_host_arr_bridge.notifyDirty(0xFF,
                                        (uint8_t)spsarr::DIRTY_ACTIVE);
      }
    }
    mcl_actions.clear_tracks(clear_select);
  }
  // A load changes the active slot state, but not the source grid cell/link
  // data used by the arranger import. Avoid forcing arranger cell re-fetches
  // on every playback load boundary.
  if (any_load || any_clear) {
    sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)spsarr::DIRTY_ACTIVE);
    notify_host_seq_dirty_for_load(track_select, clear_select, offset);
  }
}
#endif

#if MCL_FEATURE_HOST_ARRANGER
void GridTask::service_host_arranger_load_before_edit() {
  if (!load_queue.is_empty()) {
    load_queue_handler();
  }
}
#else
void GridTask::service_host_arranger_load_before_edit() {}
#endif

void GridTask::run() {
  //  DEBUG_PRINTLN(MidiClock.div32th_counter / 2);
  //  A4Track *a4_track = (A4Track *)&temp_track;
  //   ExtTrack *ext_track = (ExtTrack *)&temp_track;
  // MD GUI update.

  GUI.removeTask(this);

  if (stop_hard_callback) {
    mcl_actions_callbacks.StopHardCallback();
    stop_hard_callback = false;
    load_queue.init();
    init_save_queue();
    reset_host_playback_after_stop();
  }
  else {
    gui_update();
    tick_host_arranger();
    save_queue_handler();
    load_queue_handler();
    flush_host_automation_writes();
    transition_handler();
  }
  GUI.addTask(this);
}

void GridTask::update_transition_details() {
  if (device_manager.primary_device() != &MD) {
    send_kit_name = false;
    return;
  }

  GridRowHeader row_header;
  proj.read_grid_row_header(&row_header, next_active_row, 0);
  uint8_t len = MD.sysex_protocol.kitname_length;
  if (len > sizeof(kit_names[0])) {
    len = sizeof(kit_names[0]);
  }

  if (row_header.active) {
    memcpy(kit_names[0], row_header.name, len);
    m_toupper(kit_names[0]);
    kit_names[0][len - 1] = '\0';
  } else {
    strcpy_P(kit_names[0], mclstr_new_kit_underscore);
  }
  send_kit_name = true;
}

void GridTask::wait_blocking(uint32_t go_step) {
  uint16_t tempo_uint = (uint16_t)MidiClock.get_tempo();
  uint32_t gui_threshold =
      MidiClock.scale_legacy_div192_to_current(
          ((uint32_t)tempo_uint * 64 + 999) / 1000);
  while (true) {
    // uint32_t counter = atomic_read(&MidiClock.div192th_counter);
    uint32_t counter = MidiClock.div192th_counter;

    uint32_t diff = MidiClock.clock_diff_div192(counter, go_step);

    if (diff == 0 || counter >= go_step || MidiClock.state != 2) {
      break;
    }

    handleIncomingMidi();
    platform_poll();

    if (diff > gui_threshold) {
      mcl.loop();
    }
  }
}

void GridTask::transition_handler() {
  GridRow slots_changed[NUM_SLOTS];
  uint8_t track_select_array[NUM_SLOTS];
  GridActiveStateDirty active_state_dirty;

  while (true) {
    if (MidiClock.state != 2 || mcl_actions.next_transition == (uint16_t)-1) {
      break;
    }
    uint16_t margin =
        ((uint16_t)MidiClock.get_tempo() * 32u + 999u) / 1000u;
    if (margin < 2) {
      margin = 2;
    }
    if (MidiClock.clock_less_than(MidiClock.div32th_counter + margin,
                                  (uint32_t)mcl_actions.next_transition * 2)) {
      break;
    }
    uint8_t send_device_mask = 0;
    memset(track_select_array, 0, sizeof(track_select_array));
    // 240ms headroom = 0.240 * (MidiClock.get_tempo()* 0.133333333333
    //                = 0.032 * MidiClock.get_tempo()
    //float div32th_per_second = MidiClock.get_tempo() * 0.133333333333f;
    //float div32th_time = 1.0f / div32th_per_second;

    load_queue_handler();

    DEBUG_PRINTLN(F("Preparing for next transition:"));
    DEBUG_PRINTLN(MidiClock.div16th_counter);
    DEBUG_PRINTLN(mcl_actions.next_transition);

    StackMonitor::print_stack_info();

    GridRow row = 255;
    GridSlot last_slot = 255;
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

      if (gdt == nullptr)
        continue;

      uint8_t device_idx = gdt->device_idx;
      if (device_idx >= NUM_DEVS) {
        continue;
      }

      if (link_load(n, slots_changed, track_select_array, gdt)) {
        send_device_mask |= (uint8_t)(1U << device_idx);
      }

      if (row == 255) {
        row = slots_changed[n];
      }
    }

    if (send_device_mask & 1U) {
      //Send kitName before tracks are cache-loaded in MD.
      //This allows the kitName to be stored in the undokit.
      update_transition_details();
    }

    DEBUG_PRINTLN(F("sending tracks"));
    bool wait;

    if (mcl_cfg.uart2_prg_out > 0 && row != 255) {
      mcl_seq.secondary_output->sendProgramChange(mcl_cfg.uart2_prg_out - 1,
                                                  row);
    }
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

        if (gdt == nullptr)
          continue;

        uint8_t device_idx = gdt->device_idx;

        if (device_idx >= NUM_DEVS || device_idx != c)
          continue;

        GridColumn track_idx = n & 0xF;
        // Wait on first track of each device;

        if (wait && (send_device_mask & (uint8_t)(1U << c))) {

          uint32_t go_step =
              MidiClock.div16th_to_div192(mcl_actions.next_transition);
          uint16_t latency = mcl_actions.div192th_total_latency;
          go_step = go_step > (uint32_t)latency + 1u
                        ? go_step - latency - 1u
                        : 0;

          if (mcl_actions.div192th_total_latency >
              mcl_actions.dev_latency[device_idx].div192th_latency) {
            mcl_actions.div192th_total_latency -=
                mcl_actions.dev_latency[device_idx].div192th_latency;
          } else {
            mcl_actions.div192th_total_latency = 0;
          }
          wait_blocking(go_step);
        }
        wait = false;
        if (transition_load(n, track_idx, gdt)) {
          if (grid_page.active_slots[n] != SLOT_OFFSET_LOAD) {
            if (grid_page.active_slots[n] != slots_changed[n]) {
              active_state_dirty.mark();
            }
            grid_page.active_slots[n] = slots_changed[n];
          }
        }
      }
    }
    DEBUG_PRINTLN(F("SP pre cache"));
    StackMonitor::print_stack_info();

    bool update_gui = true;

    DEBUG_PRINTLN("cache next");
//#if !defined(__AVR__)
//    uint32_t go_step = mcl_actions.next_transition * 12 - 1;
//    wait_blocking(go_step);
//#endif

    pre_cache_ui_yield();
    mcl_actions.cache_next_tracks(track_select_array, update_gui);
    gui_update();

    // Once tracks are cached, we can calculate their next transition
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(n);

      if (gdt == nullptr) {
        continue;
      }
      bool ignore_chain_settings = false;
      if (track_select_array[n]) {
        last_slot = n;
      }
      if (!track_select_array[n] &&
          !is_grid_chain_load_mode(mcl_actions.chains[n].mode)) {
        continue;
      }
      mcl_actions.calc_next_slot_transition(n, ignore_chain_settings);
    }

    if (last_slot != 255 && slots_changed[last_slot] < GRID_LENGTH) {
      bool last_slot_chain =
          is_grid_chain_load_mode(mcl_actions.chains[last_slot].mode);
      active_state_dirty.mark_row_change(last_active_row, slots_changed[last_slot],
                                         chain_behaviour, last_slot_chain);
      last_active_row = slots_changed[last_slot];
      chain_behaviour = last_slot_chain;
      next_active_row = chain_behaviour ? mcl_actions.links[last_slot].row : last_active_row;
      row_update();
    }

    active_state_dirty.flush();

    mcl_actions.calc_next_transition();
    mcl_actions.calc_latency();
  }
}

bool GridTask::link_load(GridSlot n, GridRow *slots_changed,
                         uint8_t *track_select_array, GridDeviceTrack *gdt) {
  EmptyTrack empty_track;
  auto *pmem_track = empty_track.load_from_mem(
      gdt->mem_slot_idx, gdt->track_type, GridTrack::STORAGE_HEADER_SIZE);
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

bool GridTask::transition_load(GridSlot n, GridColumn track_idx,
                               GridDeviceTrack *gdt) {
  EmptyTrack empty_track;

  auto *pmem_track =
      empty_track.load_from_mem(gdt->mem_slot_idx, gdt->track_type);

  if (pmem_track == nullptr) {
    return false;
  }

  gdt->seq_track->count_down = 0;
  gdt->seq_track->load_sound = mcl_actions.send_machine[n];
  if (mcl_actions.send_machine[n] == 1) {
    pmem_track->transition_send(track_idx, n);
  }

  pmem_track->transition_load(track_idx, gdt->seq_track, n);
  mcl_actions.start_load_fade_at(
      n, pmem_track->load_fade_data(),
      MidiClock.div16th_to_div192(mcl_actions.next_transition,
                                  mcl_actions.transition_offsets[n]));
  return true;
}

GridTask grid_task(0);
