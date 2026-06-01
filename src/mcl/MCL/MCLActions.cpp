//* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCLActions.h"
#include "Project.h"
#include "MCLGUI.h"
#include "../Drivers/MD/MD.h"
#include "GridPages.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
#include "MCLSeq.h"
#include "Elektron.h"
#include "EmptyTrack.h"
#include "MDTrack.h"
#include "GridTask.h"
#include "TrackLoadFadeTarget.h"
#include "../Drivers/Generic/Sequencer/TrackLoadFadeRunner.h"
#include "platform.h"

#define MD_KIT_LENGTH 0x4D0

namespace {

const char *shared_row_name(ElektronDevice **devs,
                            uint8_t save_dev_mask) {
  // Preserve the existing single-source row naming model: primary device wins.
  if ((save_dev_mask & (1 << 0)) && devs[0] != nullptr) {
    const char *name = devs[0]->getGridRowName();
    if (name != nullptr && name[0] != '\0') {
      return name;
    }
  }

  if ((save_dev_mask & (1 << 1)) && devs[1] != nullptr) {
    const char *name = devs[1]->getGridRowName();
    if (name != nullptr && name[0] != '\0') return name;
  }
  return nullptr;
}

void copy_row_name_text(GridRowHeader &row_header, const char *name) {
  if (name == nullptr) {
    row_header.name[0] = '\0';
    return;
  }
  strncpy(row_header.name, name, sizeof(row_header.name));
  row_header.name[sizeof(row_header.name) - 1] = '\0';
}

void copy_row_name(GridRowHeader &row_header, const char *name) {
  copy_row_name_text(row_header, name);
  row_header.active = true;
}

void inherit_grid_x_row_name(GridRowHeader row_headers[NUM_GRIDS]) {
  for (uint8_t n = 1; n < NUM_GRIDS; n++) {
    copy_row_name_text(row_headers[n], row_headers[0].name);
  }
}

} // namespace

DeviceTrack *MCLActions::load_and_prepare_track(GridSlot track_idx, GridRow row,
                                                uint8_t track_type, SeqTrack *seq_track,
                                                uint8_t seq_track_idx, bool &was_rebuilt,
                                                EmptyTrack &scratch,
                                                int8_t link_slot) {
  auto *loaded_track = scratch.load_from_grid_512(track_idx, row);
  if (loaded_track == nullptr) {
    return nullptr;
  }
  if (link_slot >= 0) {
    scratch.link.store_in_mem(link_slot, &(links[0]));
  }

  auto *device_track =
      loaded_track->materialize_as(track_type, seq_track_idx, seq_track);
  if (device_track == nullptr) {
    scratch.clear();
    if (loaded_track->active != EMPTY_TRACK_TYPE) {
      scratch.init();
    }
    device_track = scratch.init_track_type(track_type);
    if (seq_track != nullptr) {
      device_track->init(seq_track_idx, seq_track);
    }
    was_rebuilt = true;
    return device_track;
  }

  was_rebuilt = false;
  return device_track;
}

// No STL, no closure, no std::function, cannot make this generic...
// void __attribute__ ((noinline)) FOREACH_GRID_TRACK(void(*fn)(uint8_t,
// uint8_t, uint8_t, MidiDevice*, ElektronDevice*)) { uint8_t grid; uint8_t
// track_idx; MidiDevice *devs[2] = {
// device_manager.primary_device(),
// device_manager.secondary_device(),
//};
// ElektronDevice *elektron_devs[2] = {
// devs[0]->asElektronDevice(),
// devs[1]->asElektronDevice(),
//};
// for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
// if (i < GRID_WIDTH) {
// grid = 0;
// track_idx = i;
//} else {
// grid = 1;
// track_idx = i - GRID_WIDTH;
//}
// fn(i, grid, track_idx, devs[grid], elektron_devs[grid]);
//}
//}

void MCLActions::setup() {
  // DEBUG_PRINTLN(F("mcl actions setup"));
  clear_load_fades();
  init_chains();
  mcl_actions_callbacks.setup_callbacks();
  mcl_actions_midievents.setup_callbacks();
}

void MCLActions::init_chains() {
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    chains[n].init();
  }
}

void MCLActions::clear_load_fades() {
  TrackLoadFadeRunner::clear();
}

void MCLActions::start_load_fade_at(GridSlot slot,
                                    const TrackLoadFadeData *fade,
                                    uint32_t start_clock) {
  if (slot >= GRID_WIDTH) {
    return;
  }

  GridDeviceTrack *gdt = get_grid_dev_track(slot);
  TrackLoadFadeTarget target;
  const bool ok = resolve_track_load_fade_target(slot, gdt, fade, &target);
  // The runner always clears the slot on entry, so an unresolved target still
  // wipes any prior fade for this slot — matches the pre-refactor behavior.
  TrackLoadFadeRunner::start(slot, target, ok ? fade : nullptr, start_clock);
}

void MCLActions::kit_reload(uint8_t pattern) {
  // DEBUG_PRINT_FN();
  /*
    if (mcl_actions.do_kit_reload != 255) {
      if (mcl_actions.writepattern == pattern) {
        auto primary =
            device_manager.primary_device()->asElektronDevice();
        auto secondary =
            device_manager.secondary_device()->asElektronDevice();
        if (primary != nullptr) {
          primary->loadKit(mcl_actions.do_kit_reload);
        }
        if (secondary != nullptr) {
          secondary->loadKit(mcl_actions.do_kit_reload);
        }
      }
      mcl_actions.do_kit_reload = 255;
    }
  */
}


GridDeviceTrack *MCLActions::get_grid_dev_track(GridSlot slot_number) {
  if (slot_number >= GRID_WIDTH * 2) { return nullptr; }
  GridIndex grid_idx = 0;
  if (slot_number >= GRID_WIDTH) { slot_number -= GRID_WIDTH; grid_idx = 1; }

  // Find first device that is hosting this slot_number.

  GridDeviceTrack *gdt = &proj.grids[grid_idx].tracks[slot_number];
  if (!gdt->isActive()) { return nullptr; }
  return gdt;
}

void md_import() {

  if (!mcl_gui.wait_for_confirm("Import:", "Overwrite rows?")) {
    return;
  }

  uint8_t track_select_array[NUM_SLOTS] = {0};

  MidiDevice *devs[2];
  device_manager.get_devices(devs);

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(n);

    if (gdt == nullptr)
      continue;

    if (gdt->device_idx >= NUM_DEVS)
      continue;

    if (devs[gdt->device_idx] != nullptr &&
        devs[gdt->device_idx]->supports_capability(
            MidiDeviceCapability::MdPatternImport)) {
      track_select_array[n] = 1;
    }
  }

  for (uint8_t n = opt_import_src;
       n < opt_import_src + opt_import_count && n < 128; n++) {
    uint8_t count = n - opt_import_src;
    mcl_gui.draw_progress("IMPORTING", count, opt_import_count);
    mcl_actions.save_tracks(opt_import_dest + count, track_select_array,
                            SAVE_MD_PATTERN_IMPORT, n);
  }

  grid_page.row_scan = GRID_LENGTH;
  grid_page.reload_slot_models = false;
  mcl.setPage(GRID_PAGE);
}

void MCLActions::save_tracks(GridRow row, uint8_t *slot_select_array, uint8_t merge,
                             uint8_t readpattern) {
  // DEBUG_PRINT_FN();

  EmptyTrack empty_track;

  if (readpattern == 255) {
    readpattern = MD.currentPattern;
  }

  uint8_t save_dev_mask = 0;
  uint8_t saved_grid_mask = 0;
  MidiDevice *devs[NUM_DEVS];
  device_manager.get_devices(devs);
  ElektronDevice *elektron_devs[NUM_DEVS] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
  };

  uint8_t i = 0;


  for (i = 0; i < NUM_SLOTS; i++) {
    if (slot_select_array[i] > 0) {
      GridDeviceTrack *gdt = get_grid_dev_track(i);
      if (gdt != nullptr && gdt->device_idx < NUM_DEVS) {
        save_dev_mask |= (uint8_t)(1 << gdt->device_idx);
      }
    }
  }

  if (MidiClock.state == 2) {
    merge = 0;
  }

  uint8_t device_bit = 1;
  for (i = 0; i < NUM_DEVS; ++i, device_bit <<= 1) {
    if (elektron_devs[i] != nullptr) {
      if (save_dev_mask & device_bit) {
        if (merge > 0) {
          // DEBUG_PRINTLN(F("fetching pattern"));
          // DEBUG_PRINTLN(readpattern);
          if (!elektron_devs[i]->getBlockingPattern(readpattern)) {
            // DEBUG_PRINTLN(F("could not receive pattern"));
            save_dev_mask &= (uint8_t)~device_bit;
            continue;
          }
          ElektronPattern *p = (ElektronPattern*) elektron_devs[i]->getPattern();
          if (p->isEmpty()) {
            save_dev_mask &= (uint8_t)~device_bit;
            continue;
          }
          if (!elektron_devs[i]->getBlockingKit(p->getKit())) {
            // DEBUG_PRINTLN(F("could not receive kit"));
            continue;
          }
        } else {
          if (elektron_devs[i]->canReadWorkspaceKit()) {
            if (!elektron_devs[i]->getWorkSpaceKit()) {
              // DEBUG_PRINTLN(F("could not receive kit"));
              save_dev_mask &= (uint8_t)~device_bit;
              continue;
            }
          } else if (elektron_devs[i]->canReadKit()) {
            auto kit = elektron_devs[i]->getCurrentKit();
            elektron_devs[i]->saveCurrentKit(kit);
            if (!elektron_devs[i]->getBlockingKit(kit)) {
              // DEBUG_PRINTLN(F("could not receive kit"));
              save_dev_mask &= (uint8_t)~device_bit;
              continue;
            }
          }
        }
        if (MidiClock.state == 2) {
          elektron_devs[i]->updateKitParams();
        } else {
          elektron_devs[i]->undokit_sync();
        }
      }
    }
  }

  GridRowHeader row_headers[NUM_GRIDS];
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.read_grid_row_header(&row_headers[n], row, n);
  }

  for (i = 0; i < NUM_SLOTS; i++) {
    if (slot_select_array[i] > 0) {

      GridDeviceTrack *gdt = get_grid_dev_track(i);
      if (gdt == nullptr || gdt->device_idx >= NUM_DEVS) {
        continue;
      }
      GridIndex grid_idx = i >> 4;
      GridColumn track_idx = i & 0xF;
      uint8_t device_idx = gdt->device_idx;
      bool online = (devs[device_idx] != nullptr);
      // If save_dev_tracks[dev_idx] turns false, it means getBlockingKit
      // has failed, so we just skip this device.

      if (!(save_dev_mask & (uint8_t)(1 << device_idx))) {
        continue;
      }

      // Preserve existing link settings before save.
      GridLink saved_link;
      TrackLoadFadeData saved_load_fade;
      saved_load_fade.init();
      if (row_headers[grid_idx].track_type[track_idx] != EMPTY_TRACK_TYPE) {
        auto *loaded_existing = empty_track.load_from_grid_512(i, row);
        if (loaded_existing == nullptr) {
          continue;
        }
        saved_link = loaded_existing->link;
        const TrackLoadFadeData *existing_load_fade =
            loaded_existing->load_fade_data();
        if (existing_load_fade != nullptr) {
          saved_load_fade = *existing_load_fade;
        }
      } else {
        saved_link.init(row);
      }
      auto pdevice_track =
          ((DeviceTrack *)&empty_track)->init_track_type(gdt->track_type);
      pdevice_track->link = saved_link;
      TrackLoadFadeData *load_fade = pdevice_track->load_fade_data();
      if (load_fade != nullptr) {
        *load_fade = saved_load_fade;
      }
      if (pdevice_track->store_in_grid(i, row, gdt->seq_track, merge,
                                       online)) {
        row_headers[grid_idx].update_model(
            track_idx, pdevice_track->get_model(), gdt->track_type);
        saved_grid_mask |= (uint8_t)(1 << grid_idx);
      }
    }
  }

  const char *row_name = shared_row_name(elektron_devs, save_dev_mask);

  if (saved_grid_mask && row_headers[0].name[0] == '\0' &&
      row_name != nullptr) {
    copy_row_name(row_headers[0], row_name);
  }
  inherit_grid_x_row_name(row_headers);

  uint8_t grid_bit = 1;
  for (uint8_t n = 0; n < NUM_GRIDS; n++, grid_bit <<= 1) {
    if (saved_grid_mask & grid_bit) {
      row_headers[n].active = true;
    }
    proj.write_grid_row_header(&row_headers[n], row, n);
    proj.sync_grid(n);
  }
  grid_page.reload_slot_models = false;
}

void MCLActions::row_update(GridSlot last_slot) {
  if (last_slot != 255) {
    //grid_task.last_active_row = grid_task.last_active_row;
    grid_task.next_active_row = chains[last_slot].mode > 1 ? links[last_slot].row : grid_task.last_active_row;
    grid_task.chain_behaviour = chains[last_slot].mode > 1;
    grid_task.row_update();
  }
}

void MCLActions::load_tracks(uint8_t *slot_select_array,
                             GridRow *_row_array, uint8_t load_mode,
                             GridSlot load_offset) {
  // DEBUG_PRINT_FN();
  ElektronDevice *elektron_devs[2] = {
      device_manager.primary_device()->asElektronDevice(),
      device_manager.secondary_device()->asElektronDevice(),
  };
  if (load_mode == 255) {
    load_mode = mcl_cfg.load_mode;
  }
  if (load_mode != LOAD_MANUAL) {
    load_offset = 255;
  }
  GridRow row_array[NUM_SLOTS];
  uint8_t cache_track_array[NUM_SLOTS];
  if (load_mode == LOAD_QUEUE) {
    memset(cache_track_array, 0, sizeof(cache_track_array));
  }
  bool recache = false;
  GridSlot last_slot = 255;
  DEBUG_PRINTLN("load tracks");
  DEBUG_PRINTLN(load_mode);
  for (uint8_t n = 0; n < NUM_SLOTS; ++n) {
    if (slot_select_array[n] == 0) { continue; }

    GridDeviceTrack *gdt = get_grid_dev_track(n);

    if (gdt == nullptr) { continue; }

    row_array[n] = _row_array[n];

    if (load_mode == LOAD_QUEUE) {
      chains[n].add(row_array[n], get_chain_length());
      if (chains[n].num_of_links > 1) {
        slot_select_array[n] = 0;
        if (chains[n].num_of_links == 2) {
          cache_track_array[n] = 1;
          recache = true;
          last_slot = n;
        }
      }
    } else {
      chains[n].init();
    }
    chains[n].mode = load_mode;
  }

  if (MidiClock.state == 2) {
    manual_transition(slot_select_array, row_array, load_offset);
    return;
  }

  if (load_offset != 255) { recache = false; }

  for (uint8_t i = 0; i < NUM_DEVS; ++i) {
    if (elektron_devs[i] != nullptr &&
        elektron_devs[i]->canReadWorkspaceKit()) {
      elektron_devs[i]->getBlockingKit(0x7F);
    }
  }
  if (recache) {
    bool gui_update = false;
    cache_next_tracks(cache_track_array, gui_update);
    row_update(last_slot);
  } else {
    send_tracks_to_devices(slot_select_array, row_array, load_offset);
  }
}

void MCLActions::collect_tracks(uint8_t *slot_select_array,
                                GridRow *row_array, GridSlot load_offset) {

  GridSlot first_slot = 255;
  for (uint8_t n = 0; n < NUM_SLOTS; ++n) {

    if (slot_select_array[n] == 0) { continue; }
    if (first_slot == 255) { first_slot = n; }

    GridSlot dst = load_offset == 255 ? n : (n - first_slot) + load_offset;

    GridDeviceTrack *gdt = get_grid_dev_track(n);
    GridColumn track_idx_dst = dst & 0xF;

    GridDeviceTrack *gdt_dst = get_grid_dev_track(dst);

    if (gdt == nullptr || gdt_dst == nullptr || (gdt->track_type != gdt_dst->track_type)) {
      // Ignore slots that are not device supported.
      slot_select_array[n] = 0;
      continue;
    }
    GridRow row = row_array[n];
    EmptyTrack scratch;
    bool rebuilt = false;
    auto *device_track =
        load_and_prepare_track(n, row, gdt->track_type,
                               gdt_dst->seq_track, track_idx_dst, rebuilt,
                               scratch);

    if (device_track == nullptr) {
      send_machine[dst] = 0;
      continue;
    }

    const bool load_sound = !rebuilt && device_track->load_sound();
    if (!load_sound) {
      device_track->restore_sound_from_mem_if_type(gdt_dst->mem_slot_idx,
                                                   gdt_dst->track_type);
      send_machine[dst] = 0;
    } else {
      if (load_offset != 255) {
        device_track->on_copy(n & 0xF, track_idx_dst, false);
      }
      device_track->transition_cache(track_idx_dst, dst);
      send_machine[dst] = 1;
    }

    device_track->store_in_mem(gdt_dst->mem_slot_idx);
  }
}

void MCLActions::manual_transition(uint8_t *slot_select_array,
                                   GridRow *row_array, GridSlot load_offset) {
  // DEBUG_PRINT_FN();
  uint8_t q = get_quant();

  // DEBUG_CHECK_STACK();

  collect_tracks(slot_select_array, row_array, load_offset);

  uint16_t next_step = q == 255 ? (uint16_t) -1 : (MidiClock.div16th_counter / q) * q + q;

  bool increase_loops = 0;

  bool recalc_latency = true;
  uint8_t headroom = 0;
  //uint8_t headroom = ceil(MidiClock.get_tempo()* 0.133333333333f * 0.200f);
  ////DEBUG_PRINTLN("manual trans");
  GridSlot first_slot = 255;
again:
  bool overflow = next_step < MidiClock.div16th_counter;
  GridRow row = grid_task.next_active_row;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {

      if (slot_select_array[n] == 0) { continue; }

      if (first_slot == 255) { first_slot = n; }

      GridSlot dst = load_offset == 255 ? n : (n - first_slot) + load_offset;
      row = row_array[n];
      if (q == 255) {

        GridDeviceTrack *gdt_dst = get_grid_dev_track(dst);

        if (gdt_dst != nullptr) {
          transition_level[dst] = 0;
          if (increase_loops) {
            if (next_transitions[dst] == next_transition) {
              DEBUG_PRINTLN("increasing loops");
              links[dst].loops++;
            }
          }
          else {
            links[dst].loops = 1;
          }
          links[dst].set_speed(gdt_dst->seq_track->speed);
          links[dst].length = gdt_dst->seq_track->length;
          links[dst].row = row;

          LOCK();
          uint8_t cur_step_count = gdt_dst->seq_track->step_count;
          uint8_t cur_mod12_counter = gdt_dst->seq_track->mod12_counter;
          uint8_t cur_div16th_counter = MidiClock.div16th_counter;
          CLEAR_LOCK();

          next_transitions[dst] = cur_div16th_counter - ((uint16_t)cur_step_count *
                                gdt_dst->seq_track->get_speed_multiplier_int() / 12 + cur_mod12_counter / 12);

          bool ignore_chain_settings = true;
          bool ignore_overflow = true;
          calc_next_slot_transition(dst, ignore_chain_settings, ignore_overflow);

          if (MidiClock.clock_less_than(next_transitions[dst], next_step)) {
            next_step = next_transitions[dst];
          }
        }
      } else {
        // transition_level[n] = gridio_param3.getValue();
        transition_level[dst] = 0;
        next_transitions[dst] = next_step;
        transition_offsets[dst] = 0;
        links[dst].row = row;
        links[dst].loops = 1;
        // if (grid_page.active_slots[n] < 0) {
        DEBUG_PRINT("slot man trans ");
        DEBUG_PRINT(dst);
        DEBUG_PRINT(" ");
        DEBUG_PRINTLN(next_transitions[dst]);
        // }
      }
      grid_page.active_slots[dst] = load_offset == 255 ? SLOT_PENDING : SLOT_OFFSET_LOAD;
  }

  calc_next_transition();

  grid_task.next_active_row = row;
  grid_task.chain_behaviour = false;
  grid_task.row_update();

  if (recalc_latency) {
    calc_latency();
  }

  DEBUG_PRINTLN("NEXT STEP");
  DEBUG_PRINTLN(next_step);
  DEBUG_PRINTLN(next_transition);
  DEBUG_PRINTLN(MidiClock.div16th_counter);

  // int32_t pos = next_transition - (div192th_total_latency / 12) -
  // MidiClock.div16th_counter; next transition should always be at least 2
  // steps away.

  uint32_t next_16th = (uint32_t)next_transition;

  //only recalculate if next_transition is a manual transition.

  if (next_transition == next_step) {
    if (overflow) {
      next_16th += (uint16_t)-1;
    }
    uint32_t next_32th = next_16th * 2;

    uint32_t ticks_per_32th = MidiClock.div192th_ticks_per_16th() / 2u;
    if (ticks_per_32th == 0) {
      ticks_per_32th = 6;
    }
    const uint32_t latency_32th =
        ((uint32_t)div192th_total_latency + ticks_per_32th - 1u) /
        ticks_per_32th;
    const uint32_t transition_guard = latency_32th + headroom;
    if (next_32th <= (uint32_t)MidiClock.div32th_counter + transition_guard) {

      if (q == 255) {
        increase_loops = 1;
      } else {
        // DEBUG_PRINTLN("try again");
        next_step += q;
      }
      recalc_latency = false;
      goto again;
    }
  }
}

bool MCLActions::load_track_immediate(GridRow row, GridSlot i, GridSlot dst,
                                      GridDeviceTrack *gdt_dst,
                                      uint8_t *send_masks) {
  GridColumn track_idx_dst = dst & 0xF;
  EmptyTrack scratch;
  bool rebuilt = false;
  auto *ptrack = load_and_prepare_track(i, row, gdt_dst->track_type,
                                        gdt_dst->seq_track, track_idx_dst,
                                        rebuilt, scratch, dst);

  if (ptrack == nullptr) {
    // DEBUG_PRINTLN("bad read");
    return false;
  } // read failure

  const bool load_sound = !rebuilt && ptrack->load_sound();
  if (!load_sound) {
    ptrack->restore_sound_from_mem_if_type(gdt_dst->mem_slot_idx,
                                           gdt_dst->track_type);
    ptrack->load_immediate_cleared(track_idx_dst, gdt_dst->seq_track);
  } else {
    send_masks[dst] = 1;
    if (i != dst) {
      ptrack->on_copy(i & 0xF, track_idx_dst, false);
    }
    ptrack->load_immediate(track_idx_dst, gdt_dst->seq_track);
  }

  ptrack->store_in_mem(gdt_dst->mem_slot_idx);
  start_load_fade_at(dst, ptrack->load_fade_data(), MidiClock.div192th_counter);

  return true;
}

void MCLActions::restore_mute_states(uint8_t *mute_states) {
  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
    if (mute_states[i] == 255) { continue; }
    GridDeviceTrack *gdt_dst = get_grid_dev_track(i);
    if (gdt_dst != nullptr) {
      gdt_dst->seq_track->mute_state = mute_states[i];
    }
  }
}

void MCLActions::send_tracks_to_devices(uint8_t *slot_select_array,
                                        GridRow *row_array, GridSlot load_offset) {
  // DEBUG_PRINT_FN();
  DEBUG_PRINTLN("send tracks to devices");
  // Unsupported slots are cleared from slot_select_array before cache refresh.

  MidiDevice *devs[2];
  device_manager.get_devices(devs);

  uint8_t send_masks[NUM_SLOTS] = {0};
  uint8_t mute_states[NUM_SLOTS];
  memset(mute_states, 255, sizeof(mute_states));

  GridRow row = 0;

  // DEBUG_PRINTLN("send tracks 1");
  // DEBUG_PRINTLN((int)SP);
  // DEBUG_CHECK_STACK();

  GridSlot last_slot = 255;
  GridSlot first_slot = 255;
  GridRow current_row = grid_page.getRow();
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {

    if (slot_select_array[i] == 0) { continue; }

    if (first_slot == 255) { first_slot = i; }

    GridDeviceTrack *gdt = get_grid_dev_track(i);

    GridSlot dst = load_offset == 255 ? i : (i - first_slot) + load_offset;
    GridDeviceTrack *gdt_dst = get_grid_dev_track(dst);

    if (gdt == nullptr || gdt_dst == nullptr || (gdt->track_type != gdt_dst->track_type)) { slot_select_array[i] = 0; continue; }

    row = row_array ? row_array[i] : current_row;
    last_slot = dst;

    grid_page.active_slots[dst] = load_offset == 255 ? row : SLOT_OFFSET_LOAD;

    // DEBUG_DUMP("here");
    // DEBUG_DUMP(row);

    if (!load_track_immediate(row, i, dst, gdt_dst, send_masks)) {
      slot_select_array[i] = 0;
    } else {
      mute_states[dst] = gdt_dst->seq_track->mute_state;
      gdt_dst->seq_track->mute_state = SEQ_MUTE_ON;
    }
  }

  /*Send the encoded kit to the devices via sysex*/
  uint16_t myclock = read_clock_ms();
  uint16_t latency_ms = 0;

  GridRowHeader row_header;

  proj.read_grid_row_header(&row_header, row, 0);

  if (mcl_cfg.uart2_prg_out > 0) {
    mcl_seq.secondary_output->sendProgramChange(mcl_cfg.uart2_prg_out - 1,
                                                row);
  }

  for (uint8_t i = 0; i < NUM_DEVS; ++i) {
    auto elektron_dev = devs[i]->asElektronDevice();
    if (elektron_dev != nullptr) {

      char *dst = elektron_dev->getKitName();
      if (dst != nullptr) {
        if (row_header.active) {
          uint8_t len = elektron_dev->sysex_protocol.kitname_length;
          strncpy(dst, row_header.name, len);
          m_toupper(dst);
          dst[len - 1] = '\0';
        } else {
          strcpy_P(dst, mclstr_new_kit);
        }
        // DEBUG_PRINTLN("SEND NAME");
        // DEBUG_PRINTLN(dst);
      }
      latency_ms += elektron_dev->sendKitParams(send_masks + i * GRID_WIDTH);
    }
  }

  // note, do not re-enter grid_task -- stackoverflow

  while (clock_diff(myclock, read_clock_ms()) < latency_ms) {
    platform_wait_poll();
  }

  restore_mute_states(mute_states);
  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;

  // if ((mcl_cfg.load_mode == 0) || (mcl_cfg.load_mode == LOAD_MANUAL)) {
  //   next_transition = (uint16_t)-1;
  //   return;
  // }

  // Cache
  // DEBUG_CHECK_STACK();

  if (load_offset == 255) {
  bool gui_update = false;
  cache_next_tracks(slot_select_array, gui_update);

  // in_sysex = 0;

    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if ((slot_select_array[n] == 0) || (grid_page.active_slots[n] == SLOT_DISABLED)) { continue; }
        GridDeviceTrack *gdt = get_grid_dev_track(n);
        if (gdt != nullptr) {
          transition_level[n] = 0;
          next_transitions[n] = MidiClock.div16th_counter -
                                ((uint16_t)gdt->seq_track->step_count *
                                 gdt->seq_track->get_speed_multiplier_int() / 12);
          transition_offsets[n] = 0;
          calc_next_slot_transition(n);
        }
    }
  }

  grid_task.last_active_row = row;
  grid_task.next_active_row = row;
  grid_task.chain_behaviour = false;
  row_update(last_slot);
  calc_next_transition();
  calc_latency();
}

void MCLActions::update_chain_links(GridSlot n, GridDeviceTrack *gdt) {

   if (chains[n].is_mode_queue()) {
      uint8_t chain_len = chains[n].get_length();
      if (chain_len == QUANT_LEN) {
        if (links[n].loops == 0) {
          links[n].loops = 1;
        }
      } else {
        links[n].loops = 1;
        uint8_t length =
            (uint16_t)chain_len * 12 /
            gdt->seq_track->get_speed_multiplier_int();
        links[n].length = length ? length : 1;
       /* constexpr uint8_t min_steps_before_transition = 2;
        while (links[n].loops * links[n].length < min_steps_before_transition) {
          links[n].loops++;
        }*/
      }
      //if (links[n].length == 0) { links[n].length = 16; }
      chains[n].inc();
      links[n].row = chains[n].get_row();
      if (links[n].row == 255) {
        setLed2();
      }
    }
}

void MCLActions::cache_track(GridSlot n, GridDeviceTrack* gdt, GridColumn track_idx) {
  EmptyTrack scratch;
  send_machine[n] = 0;
  bool rebuilt = false;
  auto *ptrack =
      load_and_prepare_track(n, links[n].row, gdt->track_type,
                             gdt->seq_track, track_idx, rebuilt, scratch);
  if (ptrack == nullptr) {
    return;
  }

  const bool load_sound = !rebuilt && ptrack->load_sound();
  if (!load_sound) {
    ptrack->restore_sound_from_mem_if_type(gdt->mem_slot_idx,
                                           gdt->track_type);
  } else if (ptrack->get_sound_data_ptr() && ptrack->get_sound_data_size()) {
    DEBUG_PRINTLN("comparing sound");
    if (ptrack->memcmp_sound(gdt->mem_slot_idx) != 0) {
      DEBUG_PRINTLN("no match");
      ptrack->transition_cache(track_idx, n);
      send_machine[n] = 1;
    }
  }

  ptrack->store_in_mem(gdt->mem_slot_idx);
}


void MCLActions::cache_next_tracks(uint8_t *slot_select_array,
                                   bool gui_update) {

  uint16_t tempo_uint = (uint16_t)MidiClock.get_tempo();
  uint32_t gui_threshold =
      MidiClock.scale_legacy_div192_to_current(
          ((uint32_t)tempo_uint * 64 + 999) / 1000);

  uint8_t n = NUM_SLOTS;

  while (n--) {
    if (slot_select_array[n] == 0)
      continue;

    GridDeviceTrack *gdt = get_grid_dev_track(n);

    if (gdt == nullptr)
      continue;

    //Assume next transition is 2 steps away.
    uint32_t next = MidiClock.div16th_to_div192(
                        (uint32_t)next_transition + 2u) -
                    1u;

    while ((gdt->seq_track->count_down && !gdt->seq_track->cache_loaded && (MidiClock.state == 2))) {
      platform_poll();
      handleIncomingMidi();
      uint32_t counter = MidiClock.div192th_counter;
      uint32_t diff = MidiClock.clock_diff_div192(counter, next);
      if (diff == 0 || counter >= next) {
        break;
      }
      if (gui_update && diff > gui_threshold) {
         mcl.loop();
      }
    }

    update_chain_links(n, gdt);

    if (links[n].row >= GRID_LENGTH ||
        links[n].row == grid_page.active_slots[n] || links[n].loops == 0)
      continue;

    cache_track(n, gdt, n & 0xF);
  }
}

void MCLActions::calc_next_slot_transition(GridSlot n,
                                           bool ignore_chain_settings,
                                           bool ignore_overflow) {
 DEBUG_PRINTLN("calc_next_slot_transition");
   if (!ignore_chain_settings) {
    switch (chains[n].mode) {
    case LOAD_QUEUE: {
      break;
    }
    case LOAD_AUTO: {
      if (links[n].loops == 0) {
        next_transitions[n] = -1;
        return;
      }
      break;
    }
    case LOAD_MANUAL: {
      next_transitions[n] = -1;
      return;
    }
    }
  }

  // next transition[n] already valid, use this.
  if (next_transitions[n] == (uint16_t)-1 ||
      (next_transitions[n] > next_transition && !ignore_overflow)) {
    return;
  }

  GridDeviceTrack *gdt = get_grid_dev_track(n);
  if (gdt == nullptr) {
    DEBUG_PRINTLN("exit");
    return;
  }
  uint16_t next_transitions_old = next_transitions[n];

  // Q12: loops * length * speed_int (where speed_int = speed_multiplier * 12)
  uint32_t len_q12 = (uint32_t)links[n].loops * links[n].length * gdt->seq_track->get_speed_multiplier_int();
  if (len_q12 < 12) {
      len_q12 = 48; // len = 4 in Q12
      transition_offsets[n] = 0;
  }

  // Carry fractional steps as sub-step offset (0..11)
  transition_offsets[n] += len_q12 % 12;
  if (transition_offsets[n] >= 12) {
    transition_offsets[n] -= 12;
    len_q12 += 12;
  }

  // DEBUG_DUMP(len - (uint16_t)(len));
  // DEBUG_DUMP(transition_offsets[n]);
  uint16_t len_steps = len_q12 / 12;
  next_transitions[n] += len_steps;

  // check for overflow and make sure next nearest step is greater than
  // midiclock counter
  while ((next_transitions[n] >= next_transitions_old) &&
         (next_transitions[n] < MidiClock.div16th_counter)) {
    next_transitions[n] += len_steps;
  }

  DEBUG_PRINT("slot ");
  DEBUG_PRINT(n);
  DEBUG_PRINT(" ");
  DEBUG_PRINTLN(next_transitions[n]);
}

void MCLActions::calc_next_transition() {
  DEBUG_PRINTLN(F("calc_next_transition"));
  next_transition = (uint16_t)-1;
  // DEBUG_PRINT_FN();
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (grid_page.active_slots[n] != SLOT_DISABLED) {
      if ((links[n].loops > 0)) {
        // && (links[n].row != grid_page.active_slots[n])) || links[n].length) {
        if (MidiClock.clock_less_than(next_transitions[n], next_transition)) {
          next_transition = next_transitions[n];
        }
      }
    }
  }

  nearest_bar = next_transition / 16 + 1;
  nearest_bar = nearest_bar - (nearest_bar / 100) * 100;
  nearest_beat = next_transition % 4 + 1;
  // next_transition = next_transition % 16;

  DEBUG_PRINTLN(F("current_step"));
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(F("nearest step"));
  DEBUG_PRINTLN(next_transition);
}

void MCLActions::calc_latency() {
  EmptyTrack empty_track;

  MidiDevice *devs[2];
  device_manager.get_devices(devs);

#if defined(__AVR__)
  ElektronDevice *secondary_elektron =
      devs[1] != nullptr ? devs[1]->asElektronDevice() : nullptr;
#endif

  memset(dev_latency, 0, sizeof(dev_latency));

  bool send_dev[NUM_DEVS] = {0};

  uint16_t dev_load_penalty[NUM_DEVS] = {0};

#if !defined(__AVR__)
  constexpr uint32_t LOAD_DIVISOR = (10 * 1000);
#endif

  for (uint8_t i = 0; i < NUM_DEVS; i++) {
    if (devs[i] != nullptr && devs[i]->uart != nullptr) {
#if defined(__AVR__)
      dev_load_penalty[i] = devs[i]->uart->speed / 2500u;
#else
      dev_load_penalty[i] =
          (devs[i]->uart->speed * TRACK_MIN_LOAD_TIME) / LOAD_DIVISOR;
#endif
    }
  }

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (grid_page.active_slots[n] == SLOT_DISABLED)
      continue;
    if (next_transitions[n] == next_transition) {
      GridDeviceTrack *gdt = get_grid_dev_track(n);
      if (gdt == nullptr) {
        continue;
      }

      uint8_t device_idx = gdt->device_idx;
      if (device_idx >= NUM_DEVS) {
        continue;
      }

      if (send_machine[n] == 1) {
        GridColumn track_idx = n & 0xF;
        // Optimised, assume we dont need to read the entire object to calculate
        // latency.
        auto *ptrack = empty_track.load_from_mem(
            gdt->mem_slot_idx, gdt->track_type, GridTrack::STORAGE_HEADER_SIZE);
        //   uint16_t diff = clock_diff(old_clock, clock);
        if (ptrack == nullptr || !ptrack->is_active() ||
            gdt->track_type != ptrack->active) {
          continue;
        }
        dev_latency[device_idx].latency_bytes +=
            max(ptrack->calc_latency(track_idx), dev_load_penalty[device_idx]);
      }
      send_dev[device_idx] = true;
    }
  }

  #if defined(__AVR__)
  /* atmega2560 is not fast enough to pack elektron data at 8x turbo for Analog4 etc..
   * double the latency required */
  if (send_dev[1] && secondary_elektron != nullptr &&
      devs[1]->uart != nullptr && devs[1]->uart->speed >= 250000) {
    dev_latency[1].latency_bytes *= 2;
  }
  #endif

  // div192th_latency = ceil(tempo * 0.8 * latency_bytes * 10 / speed)
  //                  = ceil(tempo * 8 * latency_bytes / speed)
  uint16_t tempo_uint = (uint16_t)MidiClock.get_tempo();

  div192th_total_latency = 0;

  if (mcl_cfg.uart2_prg_out > 0) {
    send_dev[1] = true;
  }
  const uint32_t min_latency = MidiClock.scale_legacy_div192_to_current(6);
  for (uint8_t a = 0; a < NUM_DEVS; a++) {
    if (send_dev[a]) {
      if (devs[a] == nullptr || devs[a]->uart == nullptr ||
          devs[a]->uart->speed == 0) {
        continue;
      }
      uint32_t den = devs[a]->uart->speed;
      uint32_t num = (uint32_t)tempo_uint * 8 * dev_latency[a].latency_bytes;
      //Transimission Latency
      uint32_t latency = (num + den - 1) / den;
      latency = MidiClock.scale_legacy_div192_to_current(latency);

      //Load Latency
      //We need at least 6 sequencer ticks of latency to account for seq_track load_cache() functions
      //which are splayed over count_down duration
      //if (a == 0) {
      if (latency < min_latency) {
        latency = min_latency;
      }
      if (latency > 0xFFFFu) {
        latency = 0xFFFFu;
      }
      dev_latency[a].div192th_latency = (uint16_t)latency;

      uint32_t total_latency =
          (uint32_t)div192th_total_latency + dev_latency[a].div192th_latency;
      div192th_total_latency =
          total_latency > 0xFFFFu ? 0xFFFFu : (uint16_t)total_latency;
    }
  }
   DEBUG_PRINTLN("total latency");
   DEBUG_PRINTLN(div192th_total_latency);
}

MCLActions mcl_actions;
