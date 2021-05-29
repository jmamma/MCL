/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL_impl.h"

#define MD_KIT_LENGTH 0x4D0

// No STL, no closure, no std::function, cannot make this generic...
// void __attribute__ ((noinline)) FOREACH_GRID_TRACK(void(*fn)(uint8_t,
// uint8_t, uint8_t, MidiDevice*, ElektronDevice*)) { uint8_t grid; uint8_t
// track_idx; MidiDevice *devs[2] = {
// midi_active_peering.get_device(UART1_PORT),
// midi_active_peering.get_device(UART2_PORT),
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
  DEBUG_PRINTLN(F("mcl actions setup"));
  mcl_actions_callbacks.setup_callbacks();
  mcl_actions_midievents.setup_callbacks();
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    next_transitions[i] = 0;
    transition_offsets[i] = 0;
    send_machine[i] = 0;
    transition_level[i] = 0;
  }
  memset(dev_sync_slot, 255, NUM_DEVS);
}

void MCLActions::kit_reload(uint8_t pattern) {
  DEBUG_PRINT_FN();
  if (mcl_actions.do_kit_reload != 255) {
    if (mcl_actions.writepattern == pattern) {
      auto dev1 =
          midi_active_peering.get_device(UART1_PORT)->asElektronDevice();
      auto dev2 =
          midi_active_peering.get_device(UART2_PORT)->asElektronDevice();
      if (dev1 != nullptr) {
        dev1->loadKit(mcl_actions.do_kit_reload);
      }
      if (dev2 != nullptr) {
        dev2->loadKit(mcl_actions.do_kit_reload);
      }
    }
    mcl_actions.do_kit_reload = 255;
  }
}

uint8_t MCLActions::get_grid_idx(uint8_t slot_number) {
  return slot_number / GRID_WIDTH;
}

GridDeviceTrack *MCLActions::get_grid_dev_track(uint8_t slot_number,
                                                uint8_t *track_idx,
                                                uint8_t *dev_idx) {
  uint8_t grid_idx = get_grid_idx(slot_number);
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  // Find first device that is hosting this slot_number.
  for (uint8_t n = 0; n < 2; n++) {
    auto *p = &(devs[n]->grid_devices[grid_idx]);
    for (uint8_t i = 0; i < GRID_WIDTH; i++) {
      if (slot_number == p->tracks[i].get_slot_number()) {
        *track_idx = i;
        *dev_idx = n;
        return &(p->tracks[i]);
      }
    }
  }
  *track_idx = 255;
  *dev_idx = 255;
  return nullptr;
}

void MCLActions::store_tracks_in_mem(int column, int row,
                                     uint8_t *slot_select_array,
                                     uint8_t merge) {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;

  uint8_t readpattern = MD.currentPattern;

  patternswitch = PATTERN_STORE;

  uint8_t old_grid = proj.get_grid();

  bool save_dev_tracks[2] = {false, false};
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  ElektronDevice *elektron_devs[2] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
  };

  uint8_t i = 0;

  uint8_t track_idx, dev_idx;

  for (i = 0; i < NUM_SLOTS; i++) {
    if (slot_select_array[i] > 0) {
      GridDeviceTrack *gdt = get_grid_dev_track(i, &track_idx, &dev_idx);
      if (gdt != nullptr) {
        save_dev_tracks[dev_idx] = true;
      }
    }
  }

  if (MidiClock.state == 2) {
    merge = 0;
  }

  for (i = 0; i < NUM_DEVS; ++i) {
    if (save_dev_tracks[i] && elektron_devs[i] != nullptr) {
      if (merge > 0) {
        DEBUG_PRINTLN(F("fetching pattern"));
        if (!elektron_devs[i]->getBlockingPattern(readpattern)) {
          DEBUG_PRINTLN(F("could not receive pattern"));
          save_dev_tracks[i] = false;
          continue;
        }
      }

      if (elektron_devs[i]->canReadWorkspaceKit()) {
        if (!elektron_devs[i]->getBlockingKit(0x7F)) {
          DEBUG_PRINTLN(F("could not receive kit"));
          save_dev_tracks[i] = false;
          continue;
        }
      } else if (elektron_devs[i]->canReadKit()) {
        auto kit = elektron_devs[i]->getCurrentKit();
        elektron_devs[i]->saveCurrentKit(kit);
        if (!elektron_devs[i]->getBlockingKit(kit)) {
          DEBUG_PRINTLN(F("could not receive kit"));
          save_dev_tracks[i] = false;
          continue;
        }
      }

      if (MidiClock.state == 2) {
        elektron_devs[i]->updateKitParams();
      }
    }
  }

  GridRowHeader row_headers[NUM_GRIDS];
  GridTrack grid_track;

  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.select_grid(n);
    proj.read_grid_row_header(&row_headers[n], grid_page.getRow());
  }

  DEBUG_DUMP(Analog4.connected);
  bool online;

  for (i = 0; i < NUM_SLOTS; i++) {
    if (slot_select_array[i] > 0) {

      GridDeviceTrack *gdt = get_grid_dev_track(i, &track_idx, &dev_idx);
      uint8_t grid_idx = get_grid_idx(i);

      online = (devs[dev_idx] != nullptr);
      // If save_dev_tracks[dev_idx] turns false, it means getBlockingKit
      // has failed, so we just skip this device.

      if (!save_dev_tracks[dev_idx]) {
        continue;
      }

      if (gdt != nullptr) {
        proj.select_grid(grid_idx);

        // Preserve existing chain settings before save.
        if (row_headers[grid_idx].track_type[track_idx] != EMPTY_TRACK_TYPE) {
          grid_track.load_from_grid(track_idx, row);
          memcpy(&empty_track.chain, &grid_track.chain, sizeof(GridChain));
        } else {
          empty_track.chain.init(row);
        }
        auto pdevice_track =
            ((DeviceTrack *)&empty_track)->init_track_type(gdt->track_type);
        pdevice_track->store_in_grid(track_idx, grid_page.getRow(),
                                     gdt->seq_track, merge, online);
        row_headers[grid_idx].update_model(
            track_idx, pdevice_track->get_model(), gdt->track_type);
      }
    }
  }

  // Only update row name if, the current row is not active.
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    if (!row_headers[n].active) {
      for (uint8_t c = 0; c < 17; c++) {
        row_headers[n].name[c] = MD.kit.name[c];
      }
      row_headers[n].active = true;
    }
    proj.write_grid_row_header(&row_headers[n], grid_page.getRow(), n);
    proj.sync_grid(n);
  }

  proj.select_grid(old_grid);
}

void MCLActions::write_tracks(int column, int row, uint8_t *slot_select_array) {
  DEBUG_PRINT_FN();
  ElektronDevice *elektron_devs[2] = {
      midi_active_peering.get_device(UART1_PORT)->asElektronDevice(),
      midi_active_peering.get_device(UART2_PORT)->asElektronDevice(),
  };
  if ((mcl_cfg.chain_mode > 0) && (MidiClock.state == 2)) {
    for (uint8_t i = 0; i < NUM_DEVS; ++i) {
      if (elektron_devs[i] != nullptr &&
          elektron_devs[i]->canReadWorkspaceKit()) {
        auto kit = elektron_devs[i]->getKit();
        if (kit != nullptr &&
            elektron_devs[i]->currentKit != kit->getPosition()) {
          elektron_devs[i]->getBlockingKit(0x7F);
        }
      }
    }
    prepare_next_chain(row, slot_select_array);
    return;
  }
  for (uint8_t i = 0; i < NUM_DEVS; ++i) {
    if (elektron_devs[i] != nullptr &&
        elektron_devs[i]->canReadWorkspaceKit()) {
      elektron_devs[i]->getBlockingKit(0x7F);
    }
  }

  send_tracks_to_devices(slot_select_array);
}

void MCLActions::prepare_next_chain(int row, uint8_t *slot_select_array) {
  DEBUG_PRINT_FN();
  EmptyTrack empty_track;
  uint8_t q;
  uint8_t old_grid = proj.get_grid();

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  //  if (MidiClock.state != 2) {
  //  q = 0;
  //  } else {
  if (gridio_param4.cur == 0) {
    q = 4;
  } else {
    q = 1 << gridio_param4.cur;
  }
  if (q < 4) {
    q = 4;
  }

  memset(dev_sync_slot, 255, NUM_DEVS);

  uint8_t track_idx, dev_idx;
  for (uint8_t n = 0; n < NUM_SLOTS; ++n) {

    GridDeviceTrack *gdt = get_grid_dev_track(n, &track_idx, &dev_idx);
    uint8_t grid_idx = get_grid_idx(n);
    proj.select_grid(grid_idx);

    if ((slot_select_array[n] == 0) || (gdt == nullptr)) {
      // Ignore slots that are not device supported.
      slot_select_array[n] = 0;
      continue;
    }
    auto device_track = empty_track.load_from_grid(track_idx, row);
    if (device_track == nullptr || device_track->active != gdt->track_type) {
      empty_track.clear();
      device_track = device_track->init_track_type(gdt->track_type);
      device_track->init(track_idx, gdt->seq_track);
      send_machine[n] = 1;
    } else {
      send_machine[n] = 0;
      dev_sync_slot[dev_idx] = n;
    }
    device_track->store_in_mem(gdt->mem_slot_idx);
  }

  uint16_t next_step;
  if (q > 0) {
    next_step = (MidiClock.div16th_counter / q) * q + q;

    //    if (next_step < MidiClock.div16th_counter + 2) {
    //      next_step += q;
    //    }
  } else {
    next_step = MidiClock.div16th_counter + 2;
  }
  bool recalc_latency = true;
again:
  DEBUG_PRINTLN(F("q"));
  DEBUG_PRINTLN(q);
  DEBUG_PRINTLN(F("write step"));
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(next_step);
  DEBUG_PRINTLN(F("setting transition"));
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {

    if (slot_select_array[n] > 0) {
      // transition_level[n] = gridio_param3.getValue();
      transition_level[n] = 0;
      next_transitions[n] = next_step;
      chains[n].row = row;
      chains[n].loops = 1;
      // if (grid_page.active_slots[n] < 0) {
      grid_page.active_slots[n] = 0x7FFF;
      // }
    }
  }
  calc_next_transition();
  if (recalc_latency) {
    calc_latency(&empty_track);
  }
  if (next_step - (div192th_total_latency / 6) < MidiClock.div16th_counter) {
    next_step += q;
    recalc_latency = false;
    goto again;
  }
  proj.select_grid(old_grid);
}

void MCLActions::send_tracks_to_devices(uint8_t *slot_select_array) {
  DEBUG_PRINT_FN();

  uint8_t select_array[NUM_SLOTS];
  // Take a copy, because we call GUI.loop later.
  memcpy(select_array, slot_select_array, NUM_SLOTS);

  EmptyTrack empty_track;
  EmptyTrack empty_track2;

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  uint8_t mute_states[NUM_SLOTS];
  uint8_t send_masks[NUM_SLOTS] = {0};

  uint8_t old_grid = proj.get_grid();

  uint8_t track_idx, dev_idx;

  for (uint8_t i = 0; i < NUM_SLOTS; i++) {

    GridDeviceTrack *gdt = get_grid_dev_track(i, &track_idx, &dev_idx);
    uint8_t grid_idx = get_grid_idx(i);
    proj.select_grid(grid_idx);

    if ((select_array[i] == 0) || (gdt == nullptr)) {
      // Ignore slots that are not device supported.
      select_array[i] = 0;
      continue;
    }

    mute_states[i] = gdt->seq_track->mute_state;
    gdt->seq_track->mute_state = SEQ_MUTE_ON;

    grid_page.active_slots[i] = grid_page.getRow();

    DEBUG_DUMP("here");
    DEBUG_DUMP(grid_page.getRow());

    auto *ptrack = empty_track.load_from_grid(track_idx, grid_page.getRow());

    if (!ptrack) {
      continue;
    } // read failure

    ptrack->chain.store_in_mem(i, &(chains[0]));

    if (ptrack->active != gdt->track_type) {
      ptrack->init_track_type(gdt->track_type);
      ptrack->transition_clear(track_idx, gdt->seq_track);
    } else {
      ptrack->load_immediate(track_idx, gdt->seq_track);
      send_masks[i] = 1;
    }
  }

  if (write_original == 1) {
    DEBUG_PRINTLN(F("write original"));
    //     MD.kit.origPosition = md_track->origPosition;
    if (grid_page.row_headers[grid_page.cur_row].active) {
      for (uint8_t c = 0; c < 17; c++) {
      MD.kit.name[c] =
          toupper(grid_page.row_headers[grid_page.cur_row].name[c]);
      }
    }
    else {
      strcpy(MD.kit.name,"NEW_KIT");
    }
  }

  /*Send the encoded kit to the devices via sysex*/
  uint16_t myclock = slowclock;
  uint16_t latency_ms = 0;
  for (uint8_t i = 0; i < NUM_DEVS; ++i) {
#ifndef EXT_TRACKS
    if (i > 0) {
      break;
    }
#endif
    auto elektron_dev = devs[i]->asElektronDevice();
    if (elektron_dev != nullptr) {
      latency_ms += elektron_dev->sendKitParams(send_masks + i * GRID_WIDTH,
                                                &empty_track);
    }
  }

  // switch back to old grid before driving the GUI loop
  proj.select_grid(old_grid);
  // note, do not re-enter grid_task -- stackoverflow
  GUI.removeTask(&grid_task);
  while (clock_diff(myclock, slowclock) < latency_ms) {
    GUI.loop();
  }
  GUI.addTask(&grid_task);

  for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
    GridDeviceTrack *gdt = get_grid_dev_track(i, &track_idx, &dev_idx);
    if (gdt != nullptr) {
      gdt->seq_track->mute_state = mute_states[i];
    }
  }

  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;
  if ((mcl_cfg.chain_mode == 0) || (mcl_cfg.chain_mode == 2)) {
    next_transition = (uint16_t)-1;
    return;
  }

  // Cache

  cache_next_tracks(select_array, &empty_track, &empty_track2);

  // in_sysex = 0;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if ((select_array[n] > 0) && (grid_page.active_slots[n] >= 0)) {
      GridDeviceTrack *gdt = get_grid_dev_track(n, &track_idx, &dev_idx);
      if (gdt != nullptr) {
        transition_level[n] = 0;
        next_transitions[n] = MidiClock.div16th_counter -
                              (gdt->seq_track->step_count *
                               gdt->seq_track->get_speed_multiplier());
        calc_next_slot_transition(n);
      }
    }
  }
  calc_next_transition();
  calc_latency(&empty_track);
}

void MCLActions::cache_next_tracks(uint8_t *slot_select_array,
                                   EmptyTrack *empty_track,
                                   EmptyTrack *empty_track2, bool gui_update) {
  DEBUG_PRINT_FN();

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  uint8_t old_grid = proj.get_grid();

  uint8_t track_idx, dev_idx;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (slot_select_array[n] > 0) {

      if (gui_update) {
        if (n % 8 == 0) {
          if (GUI.currentPage() != &grid_write_page) {
            proj.select_grid(old_grid);
            GUI.loop();
          }
        }
      }
      GridDeviceTrack *gdt = get_grid_dev_track(n, &track_idx, &dev_idx);
      uint8_t grid_idx = get_grid_idx(n);

      if (gdt == nullptr) {
        continue;
      }

      proj.select_grid(grid_idx);

      if (chains[n].row >= GRID_LENGTH)
        continue;

      auto *ptrack = empty_track->load_from_grid(track_idx, chains[n].row);

      if (ptrack == nullptr || ptrack->active != gdt->track_type) {
        // EMPTY_TRACK_TYPE
        empty_track->clear();
        empty_track->init_track_type(gdt->track_type);
        send_machine[n] = 1;
      } else {
        auto *pmem_track =
            empty_track2->load_from_mem(gdt->mem_slot_idx, gdt->track_type);
        if (pmem_track != nullptr && pmem_track->active == ptrack->active) {
          // track type matched.
          auto *psound = ptrack->get_sound_data_ptr();
          auto *pmem_sound = pmem_track->get_sound_data_ptr();
          auto szsound = ptrack->get_sound_data_size();
          auto szmem_sound = pmem_track->get_sound_data_size();

          if (!psound || !pmem_sound || szsound != szmem_sound) {
            // something's wrong, don't send
          } else if (memcmp(psound, pmem_sound, szsound) != 0) {
            send_machine[n] = 0;
          } else {
            send_machine[n] = 1;
          }
          DEBUG_DUMP(send_machine[n]);
        }
      }
      ptrack->store_in_mem(gdt->mem_slot_idx);
    }
  }

  proj.select_grid(old_grid);
}

void MCLActions::calc_next_slot_transition(uint8_t n) {

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(n);
  //  DEBUG_PRINTLN(next_transitions[n]);
  if (chains[n].loops == 0) {
    next_transitions[n] = -1;
    return;
  }

  uint8_t track_idx, dev_idx;

  GridDeviceTrack *gdt = get_grid_dev_track(n, &track_idx, &dev_idx);
  if (gdt == nullptr) {
    return;
  }
  uint16_t next_transitions_old = next_transitions[n];
  float len;

  float l = chains[n].length;
  len = (float)chains[n].loops * l *
        (float)gdt->seq_track->get_speed_multiplier();
  while (len < 4) {
    if (len < 1) {
      len = 4;
      transition_offsets[n] = 0;
    } else {
      len = len * 2;
    }
  }

  // Last offset must be carried over to new offset.
  transition_offsets[n] += (float)(len - (uint16_t)(len)) * 12;
  if (transition_offsets[n] >= 12) {
    transition_offsets[n] = transition_offsets[n] - 12;
    len++;
  }

  DEBUG_DUMP(len - (uint16_t)(len));
  DEBUG_DUMP(transition_offsets[n]);
  next_transitions[n] += (uint16_t)len;

  // check for overflow and make sure next nearest step is greater than
  // midiclock counter
  while ((next_transitions[n] >= next_transitions_old) &&
         (next_transitions[n] < MidiClock.div16th_counter)) {
    next_transitions[n] += (uint16_t)len;
  }
  DEBUG_PRINTLN(next_transitions[n]);
}

void MCLActions::calc_next_transition() {
  next_transition = (uint16_t)-1;
  DEBUG_PRINT_FN();
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (grid_page.active_slots[n] >= 0) {
      if ((chains[n].loops > 0) &&
          (chains[n].row != grid_page.active_slots[n])) {
        if (MidiClock.clock_less_than(next_transitions[n], next_transition)) {
          DEBUG_PRINTLN(n);
          DEBUG_PRINTLN(grid_page.active_slots[n]);
          DEBUG_PRINTLN(chains[n].row);
          DEBUG_PRINTLN(next_transitions[n]);
          DEBUG_PRINTLN(F(" "));
          next_transition = next_transitions[n];
        }
      }
    }
  }
  nearest_bar = next_transition / 16 + 1;
  nearest_beat = next_transition % 4 + 1;
  // next_transition = next_transition % 16;

  DEBUG_PRINTLN(F("current_step"));
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(F("nearest step"));
  DEBUG_PRINTLN(next_transition);
}

void MCLActions::calc_latency(DeviceTrack *empty_track) {
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };

  for (uint8_t a = 0; a < NUM_DEVS; a++) {
    dev_latency[a].latency = 0;
    dev_latency[a].div32th_latency = 0;
    dev_latency[a].div192th_latency = 0;

    if (dev_sync_slot[a] != 255) {
    dev_latency[a].latency += 2 + 7;
    }
  //  dev_latency[a].load_latency = 0;
  }
  bool send_dev[NUM_DEVS] = {0};

  uint8_t track_idx, dev_idx;

  DEBUG_PRINTLN("calc latency");
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if ((grid_page.active_slots[n] < 0))
      continue;
    if (next_transitions[n] == next_transition) {
      GridDeviceTrack *gdt = get_grid_dev_track(n, &track_idx, &dev_idx);
      if (gdt == nullptr) {
        continue;
      }
      if (send_machine[n] == 0) {
      //   uint16_t old_clock = clock;
        auto *ptrack =
            empty_track->load_from_mem(gdt->mem_slot_idx, gdt->track_type);
      //   uint16_t diff = clock_diff(old_clock, clock);
        if (ptrack == nullptr || !ptrack->is_active() ||
            gdt->track_type != ptrack->active) {
          continue;
        }
        //dev_latency[dev_idx].load_latency += diff;
        dev_latency[dev_idx].latency += ptrack->calc_latency(n);
      }
      send_dev[dev_idx] = true;
    }
  }

  float tempo = MidiClock.get_tempo();
  //  div32th_per_second: tempo / 60.0f * 4.0f * 2.0f = tempo * 8 / 60
  float div32th_per_second = tempo * 0.133333333333f;
  //  div32th_per_second: tempo / 60.0f * 4.0f * 2.0f * 6.0f = tempo * 8 / 10
  float div192th_per_second = tempo * 0.8f;

  div32th_total_latency = 0;
  div192th_total_latency = 0;

  for (uint8_t a = 0; a < NUM_DEVS; a++) {
    if (send_dev[a]) {
      float bytes_per_second_uart1 = devs[a]->uart->speed / 10.0f;
      float latency_in_seconds =
           (float) dev_latency[a].latency / bytes_per_second_uart1;
      //latency_in_seconds += (float) dev_latency[a].load_latency * .0002;

      dev_latency[a].div32th_latency =
          round(div32th_per_second * latency_in_seconds) + 1;
      dev_latency[a].div192th_latency =
          round(div192th_per_second * latency_in_seconds) + 7;

      div32th_total_latency += dev_latency[a].div32th_latency;
      div192th_total_latency += dev_latency[a].div192th_latency;
      DEBUG_DUMP(dev_latency[a].div32th_latency);
      DEBUG_DUMP(dev_latency[a].div192th_latency);
    }
  }
}

MCLActions mcl_actions;
