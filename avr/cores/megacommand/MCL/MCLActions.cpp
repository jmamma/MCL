/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL_impl.h"

#define MD_KIT_LENGTH 0x4D0
#define A4_SOUND_LENGTH 0x19F

void MCLActions::setup() {
  DEBUG_PRINTLN("mcl actions setup");
  mcl_actions_callbacks.setup_callbacks();
  mcl_actions_midievents.setup_callbacks();
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    next_transitions[i] = 0;
    transition_offsets[i] = 0;
    send_machine[i] = 0;
    transition_level[i] = 0;
  }
}

void MCLActions::kit_reload(uint8_t pattern) {
  DEBUG_PRINT_FN();
  if (mcl_actions.do_kit_reload != 255) {
    if (mcl_actions.writepattern == pattern) {
      MD.loadKit(mcl_actions.do_kit_reload);
    }
    mcl_actions.do_kit_reload = 255;
  }
}

MCLActions mcl_actions;

void MCLActions::md_setsysex_recpos(uint8_t rec_type, uint8_t position) {
  DEBUG_PRINT_FN();

  uint8_t data[] = {0x6b, (uint8_t)(rec_type & 0x7F), position,
                    (uint8_t)1 & 0x7f};
  MD.sendSysex(data, countof(data));

  //  MD.sendRequest(0x6b,00000011);
}

void MCLActions::store_tracks_in_mem(int column, int row, uint8_t merge) {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;

  uint8_t readpattern = MD.currentPattern;

  patternswitch = PATTERN_STORE;

  uint8_t old_grid = proj.get_grid();

  bool save_md_tracks = false;
  bool save_grid2_tracks = false;
  uint8_t i = 0;

  for (i = 0; i < NUM_MD_TRACKS; i++) {
    if (note_interface.notes[i] == 3) {
      save_md_tracks = true;
    }
  }
#ifdef EXT_TRACKS
  for (i = GRID_WIDTH; i < NUM_SLOTS; i++) {
    if (note_interface.notes[i] == 3) {
      save_grid2_tracks = true;
    }
  }
#endif

  if (MidiClock.state == 2) {
    merge = 0;
  }
  if (save_md_tracks) {
    if (merge > 0) {
      DEBUG_PRINTLN("fetching pattern");
      if (!MD.getBlockingPattern(readpattern)) {
        DEBUG_PRINTLN("could not receive pattern");
        return;
      }
    }

    if (!MD.getBlockingKit(0x7F)) {
      DEBUG_PRINTLN("could not receive kit");
      return;
    }
    if (MidiClock.state == 2) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].update_kit_params();
      }
    }
  }

  uint8_t first_note = 255;

  GridRowHeader row_headers[NUM_GRIDS];
  GridTrack grid_track;

  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.read_grid_row_header(&row_headers[n], grid_page.getRow());
  }

  uint8_t grid_num = 0;

  DEBUG_DUMP(Analog4.connected);
  uint8_t track_type, track_num;
  bool online;

  for (i = 0; i < NI_MAX_NOTES; i++) {
    if (note_interface.notes[i] == 3) {
      if (first_note == 255) {
        first_note = i;
      }
      track_type = 255;
      online = false;

      if (i < GRID_WIDTH) {
        grid_num = 0;
        track_type = MD_TRACK_TYPE;
        track_num = i;
        if (MD.connected) {
          online = true;
        }
      } else {
        grid_num = 1;
        track_num = i - GRID_WIDTH;
        if (track_num < NUM_EXT_TRACKS) {
          track_type = EXT_TRACK_TYPE;
          if (Analog4.connected) {
            online = true;
            track_type = A4_TRACK_TYPE;
          }
        }
      }
      if (track_type != 255) {
        proj.select_grid(grid_num);

        // Preserve existing chain settings before save.
        if (row_headers[grid_num].track_type[track_num] != EMPTY_TRACK_TYPE) {
          grid_track.load_from_grid(track_num, row);
          empty_track.chain.loops = grid_track.chain.loops;
          empty_track.chain.row = grid_track.chain.row;
        } else {
          empty_track.chain.row = row;
          empty_track.chain.loops = 0;
        }
        DEBUG_DUMP(track_type);
        auto pdevice_track = empty_track.init_track_type(track_type);
        pdevice_track->store_in_grid(track_num, grid_page.getRow(), merge,
                                     online);
        row_headers[grid_num].update_model(
            track_num, pdevice_track->get_model(), track_type);
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

void MCLActions::write_tracks(int column, int row) {
  DEBUG_PRINT_FN();
  if ((mcl_cfg.chain_mode > 0) && (MidiClock.state == 2)) {
    if (MD.currentKit != MD.kit.origPosition) {
      MD.getBlockingKit(0x7F);
    }
    prepare_next_chain(row);
    return;
  }
  MD.getBlockingKit(0x7F);

  send_tracks_to_devices();
}

void MCLActions::prepare_next_chain(int row) {
  DEBUG_PRINT_FN();
  EmptyTrack empty_track;
  uint8_t q;
  uint8_t old_grid = proj.get_grid();

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
  //  }
  uint8_t slots_cached[NUM_SLOTS] = {0};

  proj.select_grid(0);

  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {

    if (note_interface.notes[n] > 0) {
      auto md_track = empty_track.load_from_grid<MDTrack>(n, row);
      if (md_track) {
        md_track->store_in_mem(n);
        slots_cached[n] = 1;
        send_machine[n] = 0;
      } else {
        send_machine[n] = 1;
      }
    }
  }
#ifdef EXT_TRACKS
  proj.select_grid(1);
  for (uint8_t n = NUM_MD_TRACKS; n < NUM_TRACKS; n++) {
    if (note_interface.notes[n] > 0) {
      auto a4_track = empty_track.load_from_grid<A4Track>(n - NUM_MD_TRACKS, row);
      if (a4_track) {
        a4_track->store_in_mem(n - NUM_MD_TRACKS);
        send_machine[n] = 0;
      } else {
        send_machine[n] = 1;
      }
    }
  }
#endif
  uint16_t next_step;
  if (q > 0) {
    next_step = (MidiClock.div16th_counter / q) * q + q;
  } else {
    next_step = MidiClock.div16th_counter + 1;
  }
  DEBUG_PRINTLN("q");
  DEBUG_PRINTLN(q);
  DEBUG_PRINTLN("write step");
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(next_step);
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {

    if (note_interface.notes[n] > 0) {
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
  calc_latency(&empty_track);

  proj.select_grid(old_grid);
}

void MCLActions::send_tracks_to_devices() {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;
#ifdef EXT_TRACKS
  // Used as a way of flaggin which A4 tracks are to be sent
  uint8_t a4_send[NUM_A4_SOUND_TRACKS] = {0};
#endif

  MDTrack md_temp_track;

  uint8_t first_note = 255;

  uint8_t mute_states[16];

  uint8_t old_grid = proj.get_grid();

  for (uint8_t i = 0; i < NUM_SLOTS; i++) {

    uint8_t grid_col = i;

    if (i < NUM_MD_TRACKS) {
      proj.select_grid(0);
      mute_states[i] = mcl_seq.md_tracks[i].mute_state;
      mcl_seq.md_tracks[i].mute_state = SEQ_MUTE_ON;
    } else {
      proj.select_grid(1);
      grid_col -= NUM_MD_TRACKS;
    }

    if (note_interface.notes[i] > 1) {

      grid_page.active_slots[grid_col] = grid_page.getRow();

      if (first_note == 255) {
        first_note = i;
      }

      auto *ptrack = empty_track.load_from_grid(grid_col, grid_page.getRow());
      if (ptrack->is_active()) {
        ptrack->chain.store_in_mem(i, &(chains[0]));
        ptrack->load_immediate(grid_col);
        if (Analog4.connected && ptrack->is<A4Track>() && grid_col < NUM_A4_SOUND_TRACKS) {
          a4_send[grid_col] = 1;
        }
      }
    }
  }

  if (write_original == 1) {
    DEBUG_PRINTLN("write original");
    //     MD.kit.origPosition = md_track->origPosition;
    for (uint8_t c = 0; c < 17; c++) {
      MD.kit.name[c] =
          toupper(grid_page.row_headers[grid_page.cur_row].name[c]);
    }
  }

  MD.kit.origPosition = 0x7F;

  /*Send the encoded kit to the MD via sysex*/
  uint16_t myclock = slowclock;

  // md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex();

  //  mcl_seq.disable();
  // md_set_kit(&MD.kit);

  // Send Analog4
#ifdef EXT_TRACKS
  if (Analog4.connected) {
    for (uint8_t i = 0; i < NUM_A4_SOUND_TRACKS; i++) {
      if (a4_send[i] == 1) {
        auto a4_track = empty_track.load_from_mem<A4Track>(i);
        if (a4_track) {
          a4_track->sound.soundpool = true;
          a4_track->sound.toSysex();
        }
      }
    }
  }
#endif
  uint16_t md_latency_ms =
      10000.0 * ((float)sizeof(MDKit) / (float)MidiUart.speed);
  md_latency_ms += 10;
  DEBUG_PRINTLN("latency");
  DEBUG_PRINTLN(md_latency_ms);

  while (clock_diff(myclock, slowclock) < md_latency_ms) {
    GUI.loop();
  };

  for (uint8_t i = 0; i < NUM_MD_TRACKS; i++) {
    mcl_seq.md_tracks[i].mute_state = mute_states[i];
  }

  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;
  if ((mcl_cfg.chain_mode == 0) || (mcl_cfg.chain_mode == 2)) {
    next_transition = (uint16_t)-1;
    return;
  }

  //Cache

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (note_interface.notes[n] > 0) {
      // if (chains[n].active > 0) {
      DEBUG_PRINTLN("about to load");
      DEBUG_PRINTLN(chains[n].row);
      DEBUG_PRINTLN(n);

      uint8_t grid_col = n;
      if ((n < NUM_MD_TRACKS)) {
        proj.select_grid(0);
        MDTrack *md_track, *mem_track;
        if ((md_track = empty_track.load_from_grid<MDTrack>(grid_col, chains[n].row))
            &&
            (mem_track = md_temp_track.load_from_mem<MDTrack>(grid_col))) {
          if (memcmp(&md_temp_track.machine, &md_track->machine, sizeof(MDMachine)) != 0) {
            send_machine[n] = 0;
          } else {
            send_machine[n] = 1;
            DEBUG_PRINTLN("machines match");
          }
          md_track->store_in_mem(grid_col);
        }
      }
#ifdef EXT_TRACKS
      else {
        proj.select_grid(1);
        grid_col -= NUM_MD_TRACKS;
        A4Track *a4_track;
        a4_track = empty_track.load_from_grid<A4Track>(grid_col, chains[n].row);
        if (a4_track) {
          a4_track->store_in_mem(grid_col);
          send_machine[n] = 0;
        } else {
          send_machine[n] = 1;
        }
      }
#endif
      //  }
    }
  }

  // in_sysex = 0;
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if ((note_interface.notes[n] > 0) && (grid_page.active_slots[n] >= 0)) {
      transition_level[n] = 0;
      next_transitions[n] = MidiClock.div16th_counter -
                            (mcl_seq.seq_tracks[n]->step_count *
                             mcl_seq.seq_tracks[n]->get_speed_multiplier());
      calc_next_slot_transition(n);
    }
  }
  calc_next_transition();
  calc_latency(&empty_track);

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

  uint16_t next_transitions_old = next_transitions[n];
  float len;

  float l = chains[n].length;
  len = (float)chains[n].loops * l *
        (float)mcl_seq.seq_tracks[n]->get_speed_multiplier();
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
          DEBUG_PRINTLN(" ");
          next_transition = next_transitions[n];
        }
      }
    }
  }
  nearest_bar = next_transition / 16 + 1;
  nearest_beat = next_transition % 4 + 1;
  // next_transition = next_transition % 16;

  DEBUG_PRINTLN("current_step");
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN("nearest step");
  DEBUG_PRINTLN(next_transition);
}

void MCLActions::calc_latency(DeviceTrack *empty_track) {
  md_latency = 0;
#ifdef EXT_TRACKS
  a4_latency = 0;
#endif

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if ((grid_page.active_slots[n] >= 0) && (send_machine[n] == 0)) {
      if (n < NUM_MD_TRACKS) {
        if (next_transitions[n] == next_transition) {
          auto md_track = empty_track->load_from_mem<MDTrack>(n);
          if (md_track) {
            md_latency +=
                calc_md_set_machine_latency(n, &(md_track->machine), &(MD.kit));
          }
          if (transition_level[n] == TRANSITION_MUTE ||
              transition_level[n] == TRANSITION_UNMUTE) {
            md_latency += 3;
          }
        }
      }
#ifdef EXT_TRACKS
      else {
        if (next_transitions[n] == next_transition) {
          a4_latency += A4_SOUND_LENGTH;
        }
      }
#endif
    }
  }
  grid_task.active = true;
  float tempo = MidiClock.get_tempo();
  //  div32th_per_second: tempo / 60.0f * 4.0f * 2.0f = tempo * 8 / 60
  float div32th_per_second = tempo * 0.133333333333f;
  //  div32th_per_second: tempo / 60.0f * 4.0f * 2.0f * 6.0f = tempo * 8 / 10
  float div192th_per_second = tempo * 0.8f;

  float bytes_per_second_uart1 = MidiUart.speed / 10.0f;
  float md_latency_in_seconds = mcl_actions.md_latency / bytes_per_second_uart1;
  md_div32th_latency = round(div32th_per_second * md_latency_in_seconds) + 1;
  md_div192th_latency = round(div192th_per_second * md_latency_in_seconds) + 3;

#ifdef EXT_TRACKS
  float bytes_per_second_uart2 = MidiUart2.speed / 10.0f;
  float a4_latency_in_seconds = mcl_actions.a4_latency / bytes_per_second_uart2;
  a4_div32th_latency = round(div32th_per_second * a4_latency_in_seconds) + 1;
  a4_div192th_latency = round(div192th_per_second * a4_latency_in_seconds) + 3;
#endif
}

int MCLActions::calc_md_set_machine_latency(uint8_t track, MDMachine *machine,
                                            MDKit *kit_, bool set_level) {
  // int sysex_headers_bytes = 7;
  int bytes = 0;

  if (kit_->models[track] != machine->model) {
    bytes += 5 + 7;
  }

  MDLFO *lfo = &(machine->lfo);
  if ((kit_->lfos[track].destinationTrack != lfo->destinationTrack)) {
    bytes += 3 + 7;
  }

  if ((kit_->lfos[track].destinationParam != lfo->destinationParam)) {
    bytes += 3 + 7;
  }

  if ((kit_->lfos[track].shape1 != lfo->shape1)) {
    bytes += 3 + 7;
  }

  if ((kit_->lfos[track].shape2 != lfo->shape2)) {
    bytes += 3 + 7;
  }

  if ((kit_->lfos[track].type != lfo->type)) {
    bytes += 3 + 7;
  }

  if ((kit_->trigGroups[track] != machine->trigGroup)) {
    bytes += 3 + 7;
  }
  if ((kit_->muteGroups[track] != machine->muteGroup)) {
    bytes += 3 + 7;
  }
  if ((set_level) && (kit_->levels[track] != machine->level)) {
    bytes += 3;
  }

  for (uint8_t i = 0; i < 24; i++) {
    if ((kit_->params[track][i] != machine->params[i]) ||
        ((i < 8) && (kit_->models[track] != machine->model))) {
      //       (mcl_seq.md_tracks[track].is_param(i)))) {
      if (machine->params[i] != 255) {
        bytes += 3;
      }
    }
  }

  return bytes;
}

void MCLActions::md_set_kit(MDKit *kit_) {
  MDTrack temp_track;

  MD.setKitName((kit_->name));
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    temp_track.get_machine_from_kit(n);
    md_set_machine(n, &(temp_track.machine), NULL, true);
    MD.setTrackParam(n, 33, temp_track.machine.level);
  }
  md_set_fxs(kit_);

  if (mcl_cfg.auto_save == 1) {
    MD.saveCurrentKit(MD.currentKit);
    MD.loadKit(MD.currentKit);
  }
}

void MCLActions::md_set_fxs(MDKit *kit_) {
  for (uint8_t n = 0; n < 8; n++) {
    MD.setCompressorParam(n, kit_->dynamics[n]);
    MD.setEQParam(n, kit_->eq[n]);
    MD.setReverbParam(n, kit_->reverb[n]);
    MD.setEchoParam(n, kit_->delay[n]);
  }
}
void MCLActions::md_set_machine(uint8_t track, MDMachine *machine, MDKit *kit_,
                                bool set_level) {
  if (kit_ == NULL) {
    MD.setMachine(track, machine);
  } else {
    if (kit_->models[track] != machine->model) {
      MD.assignMachine(track, machine->model, 0);
    }
    MDLFO *lfo = &(machine->lfo);
    if ((kit_->lfos[track].destinationTrack != lfo->destinationTrack)) {
      MD.setLFOParam(track, 0, lfo->destinationTrack);
    }

    if ((kit_->lfos[track].destinationParam != lfo->destinationParam)) {
      MD.setLFOParam(track, 1, lfo->destinationParam);
    }

    if ((kit_->lfos[track].shape1 != lfo->shape1)) {
      MD.setLFOParam(track, 2, lfo->shape1);
    }

    if ((kit_->lfos[track].shape2 != lfo->shape2)) {
      MD.setLFOParam(track, 3, lfo->shape2);
    }

    if ((kit_->lfos[track].type != lfo->type)) {
      MD.setLFOParam(track, 4, lfo->type);
    }

    if ((kit_->trigGroups[track] != machine->trigGroup)) {
      if ((machine->trigGroup > 15) || (kit_->trigGroups[track] == track)) {
        MD.setTrigGroup(track, 127);
      } else {
        MD.setTrigGroup(track, machine->trigGroup);
      }
    }
    if ((kit_->muteGroups[track] != machine->muteGroup)) {
      if ((machine->muteGroup > 15) || (kit_->muteGroups[track] == track)) {
        MD.setMuteGroup(track, 127);
      } else {
        MD.setMuteGroup(track, machine->muteGroup);
      }
    }

    if ((set_level) && (kit_->levels[track] != machine->level)) {
      MD.setTrackParam(track, 33, machine->level);
    }
    //  MidiUart.useRunningStatus = true;
    //  mcl_seq.md_tracks[track].trigGroup = machine->trigGroup;

    //  mcl_seq.md_tracks[track].send_params = true;
    for (uint8_t i = 0; i < 24; i++) {

      if (((kit_->params[track][i] != machine->params[i])) ||
          ((i < 8) && (kit_->models[track] != machine->model))) {
        //   (mcl_seq.md_tracks[track].is_param(i)))) {
        // mcl_seq.md_tracks[track].params[i] = machine->params[i];
        if (machine->params[i] != 255) {
          MD.setTrackParam(track, i, machine->params[i]);
        }
      }
    }
  }
}
