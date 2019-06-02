/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL.h"
#include "MCLActions.h"

#define MD_KIT_LENGTH 0x4D0
#define A4_SOUND_LENGTH 0x18E

void MCLActions::setup() {
  DEBUG_PRINTLN("mcl actions setup");
  mcl_actions_callbacks.setup_callbacks();
  mcl_actions_midievents.setup_callbacks();
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

bool MCLActions::place_track_inpattern(int curtrack, int column, int row,
                                       A4Sound *analogfour_sound,
                                       EmptyTrack *empty_track) {

  //       if (Grids[encodervaluer] != NULL) {
  MDTrack *md_track = (MDTrack *)empty_track;
  A4Track *a4_track = (A4Track *)empty_track;
  ExtTrack *ext_track = (ExtTrack *)empty_track;

  DEBUG_PRINT_FN();
  if (column < NUM_MD_TRACKS) {
    if (md_track->load_track_from_grid(column, row)) {
      memcpy(&(chains[column]), &(md_track->chain), sizeof(GridChain));
      grid_page.active_slots[column] = row;
      md_track->place_track_in_kit(curtrack, column, &(MD.kit));
      md_track->load_seq_data(curtrack);
    }
  } else {
    if (Analog4.connected) {

      if (a4_track->load_track_from_grid(column, row, 0)) {
        memcpy(&(chains[column]), &(a4_track->chain), sizeof(GridChain));

        grid_page.active_slots[column] = row;
        return a4_track->place_track_in_sysex(curtrack, column,
                                              analogfour_sound);
      }
    } else {
      if (ext_track->load_track_from_grid(column, row, 0)) {
        memcpy(&(chains[column]), &(a4_track->chain), sizeof(GridChain));
        grid_page.active_slots[column] = row;
        return ext_track->place_track_in_sysex(curtrack, column);
      }
    }
  }
}

void MCLActions::md_setsysex_recpos(uint8_t rec_type, uint8_t position) {
  DEBUG_PRINT_FN();

  uint8_t data[] = {0x6b, (uint8_t)rec_type & 0x7F, position,
                    (uint8_t)1 & 0x7f};
  MD.sendSysex(data, countof(data));

  //  MD.sendRequest(0x6b,00000011);
}

void MCLActions::store_tracks_in_mem(int column, int row, bool merge) {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;

  empty_track.chain.row = row;
  empty_track.chain.loops = 0;

  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  uint8_t readpattern = MD.currentPattern;

  patternswitch = PATTERN_STORE;

  bool save_md_tracks = false;
  bool save_a4_tracks = false;
  uint8_t i = 0;
  for (i = 0; i < NUM_MD_TRACKS; i++) {
    if (note_interface.notes[i] == 3) {
      save_md_tracks = true;
    }
  }
  for (i = NUM_MD_TRACKS; i < NUM_TRACKS; i++) {
    if (note_interface.notes[i] == 3) {
      save_a4_tracks = true;
    }
  }
  bool storepattern = false;

  if (MidiClock.state == 2) { merge = false; }
  if (save_md_tracks) {
    if ((merge)) {
      DEBUG_PRINTLN("fetching pattern");
      if (!MD.getBlockingPattern(readpattern)) {
        DEBUG_PRINTLN("could not receive pattern");
        return;
      }
    }
    int curkit;
    if (readpattern != MD.currentPattern) {
      curkit = MD.pattern.kit;
    } else {

      curkit = MD.getCurrentKit(CALLBACK_TIMEOUT);
      if ((mcl_cfg.auto_save == 1)) {
        MD.saveCurrentKit(MD.currentKit);
      }
    }

    if (!MD.getBlockingKit(curkit)) {
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

  uint8_t max_notes = NUM_TRACKS;
  if (!Analog4.connected) {
    max_notes = NUM_MD_TRACKS;
  }

  for (i = 0; i < max_notes; i++) {
    if (note_interface.notes[i] == 3) {
      if (first_note == 255) {
        first_note = i;
      }

      if (i >= NUM_MD_TRACKS) {
        if (Analog4.connected) {
          DEBUG_PRINTLN("a4 get sound");
          Analog4.getBlockingSoundX(i - NUM_MD_TRACKS);
          a4_track->sound.fromSysex(Analog4.midi);
        }
        a4_track->store_track_in_grid(i, grid_page.getRow(), i);
      } else {
        md_track->store_track_in_grid(i, grid_page.getRow(), i, false,
                                          merge);
      }
    }
  }

  // Only update row name if, the current name is empty
  if (strlen(grid_page.row_headers[grid_page.cur_row].name) == 0) {
    for (uint8_t c = 0; c < 17; c++) {
      grid_page.row_headers[grid_page.cur_row].name[c] = MD.kit.name[c];
    }
  }

  grid_page.row_headers[grid_page.cur_row].active = true;
  grid_page.row_headers[grid_page.cur_row].write(grid_page.getRow());

  // Sync project file to SD Card
  proj.file.sync();
}

void MCLActions::write_tracks(int column, int row) {
  DEBUG_PRINT_FN();
  if ((mcl_cfg.chain_mode > 0) && (MidiClock.state == 2)) {
    if (MD.currentKit != MD.kit.origPosition) {
      MD.getBlockingKit(MD.currentKit);
    }
    // MD.saveCurrentKit(MD.currentKit);
    // MD.getBlockingKit(MD.currentKit);
    prepare_next_chain(row);
    //  grid_task.run();
    return;
  }
  MD.saveCurrentKit(MD.currentKit);
  MD.getBlockingKit(MD.currentKit);

  send_tracks_to_devices();

}

void MCLActions::prepare_next_chain(int row) {
  DEBUG_PRINT_FN();
  EmptyTrack empty_track;
  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  // MD.saveCurrentKit(MD.currentKit);
  // MD.getBlockingKit(MD.currentKit);
  uint8_t q;

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
  uint8_t slots_cached[NUM_TRACKS] = {0};
  int32_t len = sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);

  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {

    if (note_interface.notes[n] > 0) {
      if (md_track->load_track_from_grid(n, row, len)) {
        md_track->store_in_mem(n);
        slots_cached[n] = 1;
      }
      if (md_track->active != EMPTY_TRACK_TYPE) {
        send_machine[n] = 0;
      } else {
        send_machine[n] = 1;
      }

      uint8_t trigGroup = md_track->machine.trigGroup;

      if ((trigGroup < 16) && (trigGroup != n) &&
          (slots_cached[trigGroup] == 0)) {
        DEBUG_PRINTLN("caching trig group");
        DEBUG_PRINTLN(n);
        DEBUG_PRINTLN(trigGroup);
        if (md_track->load_track_from_grid(trigGroup, row, len)) {
          md_track->store_in_mem(trigGroup);
          slots_cached[n] = 1;
        }
      }
    }
  }
  for (uint8_t n = NUM_MD_TRACKS; n < NUM_TRACKS; n++) {
    if (note_interface.notes[n] > 0) {
      if (a4_track->load_track_from_grid(n, row, 0)) {
        a4_track->store_in_mem(n);
      }
      if (a4_track->active != EMPTY_TRACK_TYPE) {
        send_machine[n] = 0;
      } else {
        send_machine[n] = 1;
      }
    }
  }
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
  for (uint8_t n = 0; n < NUM_TRACKS; n++) {

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
}

void MCLActions::send_tracks_to_devices() {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;
  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  int curtrack = last_md_track;

  uint8_t i = 0;
  int track = 0;
  uint8_t note_count = 0;
  uint8_t first_note = 254;

  // Used as a way of flaggin which A4 tracks are to be sent
  uint8_t a4_send[6] = {0, 0, 0, 0, 0, 0};
  A4Sound sound_array[4];

  KitExtra kit_extra;
  volatile uint8_t *ptr;

  for (i = 0; i < NUM_TRACKS; i++) {

    if ((note_interface.notes[i] > 1)) {
      if (first_note == 254) {
        first_note = i;
      }
      //  if (grid_page.encoders[0]->cur > 0) {
      track = i;

      if (i < NUM_MD_TRACKS) {
        place_track_inpattern(track, i, grid_page.getRow(),
                              (A4Sound *)&sound_array[0], &empty_track);

        if (i == first_note) {
          // Use first track's original kit values for write orig
          if (md_track->active != EMPTY_TRACK_TYPE) {
            memcpy(&kit_extra, &(md_track->kitextra), sizeof(kit_extra));
          } else {
            write_original = 0;
          }
        }
      } else {
        track = track - NUM_MD_TRACKS;
        mcl_seq.ext_tracks[track].buffer_notesoff();
        if (place_track_inpattern(track, i, grid_page.getRow(),
                                  (A4Sound *)&sound_array[track],
                                  &empty_track)) {
          if (Analog4.connected) {
            sound_array[track].workSpace = true;
            a4_send[track] = 1;
          }
        }
      }
    }
  }
  if ((write_original == 1)) {
    DEBUG_PRINTLN("write original");
    //     MD.kit.origPosition = md_track->origPosition;
    for (uint8_t c = 0; c < 17; c++) {
      MD.kit.name[c] =
          toupper(grid_page.row_headers[grid_page.cur_row].name[c]);
    }
    memcpy(&MD.kit.reverb[0], kit_extra.reverb, sizeof(kit_extra.reverb));
    memcpy(&MD.kit.delay[0], kit_extra.delay, sizeof(kit_extra.delay));
    memcpy(&MD.kit.eq[0], kit_extra.eq, sizeof(kit_extra.eq));
    memcpy(&MD.kit.dynamics[0], kit_extra.dynamics, sizeof(kit_extra.dynamics));
  }

  MD.kit.origPosition = MD.currentKit;

  /*Send the encoded kit to the MD via sysex*/
  md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex();
  /*Instruct the MD to reload the kit, as the kit changes won't update until
   * the kit is reloaded*/
  // if (reload == 1) {
  MD.loadKit(MD.pattern.kit);
  // Send Analog4
  if (Analog4.connected) {
    uint8_t a4_kit_send = 0;
    for (i = 0; i < 4; i++) {
      if (a4_send[i] == 1) {
        sound_array[i].toSysex();
      }
    }
  }
  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;
  if (mcl_cfg.chain_mode == 0) {
    return;
  }

  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    if (note_interface.notes[n] > 0) {
      // if (chains[n].active > 0) {

      DEBUG_PRINTLN("about to load");
      DEBUG_PRINTLN(chains[n].row);
      DEBUG_PRINTLN(n);
      if ((n < NUM_MD_TRACKS)) {
        if (md_track->load_track_from_grid(n, chains[n].row,
                                           sizeof(GridTrack) +
                                               sizeof(MDSeqTrackData) +
                                               sizeof(MDMachine))) {

          md_track->store_in_mem(n);
        }
      } else {
        if (a4_track->load_track_from_grid(n, chains[n].row, 0)) {
          a4_track->store_in_mem(n);
        }
      }
      //  }
    }
  }

  // in_sysex = 0;
  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    if ((note_interface.notes[n] > 0) && (grid_page.active_slots[n] >= 0)) {
      uint32_t len;
      /*  if (n < 16) {
          len = chains[n].loops * mcl_seq.md_tracks[n].length;
        } else {
          len = chains[n].loops * mcl_seq.ext_tracks[n - 16].length;
        }
        if (len < 4) {
          len = 4;
        } */
      //  next_transitions[n] = next_transitions_old[n];
      if (n < NUM_MD_TRACKS) {
        next_transitions[n] =
            MidiClock.div16th_counter - mcl_seq.md_tracks[n].step_count;
      } else {
        next_transitions[n] = MidiClock.div16th_counter -
                              mcl_seq.ext_tracks[n - NUM_MD_TRACKS].step_count;
      }
      calc_next_slot_transition(n);
    }
  }

  calc_next_transition();
  calc_latency(&empty_track);
}

void MCLActions::calc_next_slot_transition(uint8_t n) {

  DEBUG_PRINT_FN();
  uint16_t len;
  DEBUG_PRINTLN(n);
  //  DEBUG_PRINTLN(next_transitions[n]);
  next_transitions_old[n] = next_transitions[n];

  if (n < NUM_MD_TRACKS) {
    len = chains[n].loops * mcl_seq.md_tracks[n].length;
  } else {
    len = chains[n].loops * mcl_seq.ext_tracks[n - NUM_MD_TRACKS].length;
  }
  if (len < 4) {
    len = 4;
  }
  next_transitions[n] += len;

  // check for overflow and make sure next nearest step is greater than
  // midiclock counter
  while ((next_transitions[n] >= next_transitions_old[n]) &&
         (next_transitions[n] < MidiClock.div16th_counter)) {
    next_transitions[n] += len;
  }
  DEBUG_PRINTLN(next_transitions[n]);
}

void MCLActions::calc_next_transition() {
  next_transition = -1;
  bool first_step = false;
  DEBUG_PRINT_FN();
  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    DEBUG_PRINTLN(n);
    DEBUG_PRINTLN(grid_page.active_slots[n]);
    DEBUG_PRINTLN(chains[n].row);
    DEBUG_PRINTLN(next_transitions[n]);
    DEBUG_PRINTLN(" ");
    if (grid_page.active_slots[n] >= 0) {
      if ((chains[n].loops > 0) &&
          (chains[n].row != grid_page.active_slots[n])) {
        if (MidiClock.clock_less_than(next_transitions[n], next_transition)) {
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

void MCLActions::calc_latency(EmptyTrack *empty_track) {
  MDTrack *md_track = (MDTrack *)empty_track;
  A4Track *a4_track = (A4Track *)empty_track;
  md_latency = 0;
  a4_latency = 0;

  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    if ((grid_page.active_slots[n] >= 0) && (send_machine[n] == 0)) {
      if (n < NUM_MD_TRACKS) {
        if (next_transitions[n] == next_transition) {
          md_track->load_from_mem(n);
          md_latency +=
              calc_md_set_machine_latency(n, &(md_track->machine), &(MD.kit));
        }
      } else {
        if (next_transitions[n] == next_transition) {
          a4_latency += A4_SOUND_LENGTH;
        }
      }
    }
  }
  grid_task.active = true;

  float bytes_per_second_uart1 = (float)MidiUart.speed / (float)10;

  float bytes_per_second_uart2 = (float)MidiUart2.speed / (float)10;

  float md_latency_in_seconds =
      (float)mcl_actions.md_latency / bytes_per_second_uart1;
  float a4_latency_in_seconds =
      (float)mcl_actions.a4_latency / bytes_per_second_uart2;

  float div32th_per_second =
      ((float)MidiClock.get_tempo() / (float)60) * (float)4 * (float)2;
  // DEBUG_PRINTLN(div32th_per_second * latency_in_seconds);
  float div192th_per_second = div32th_per_second * 6;
  //  ((float)MidiClock.get_tempo() / (float)60) * (float)4 * (float)12;
  // DEBUG_PRINTLN(div32th_per_second * latency_in_seconds);
  md_div32th_latency = round(div32th_per_second * md_latency_in_seconds) + 1;
  a4_div32th_latency = round(div32th_per_second * a4_latency_in_seconds) + 1;

  md_div192th_latency = round(div192th_per_second * md_latency_in_seconds) + 3;
  a4_div192th_latency = round(div192th_per_second * a4_latency_in_seconds) + 3;
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
      bytes += 3;
    }
  }

  return bytes;
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
      if (machine->trigGroup == 255) {
        MD.setTrigGroup(track, 127);
      } else {
        MD.setTrigGroup(track, machine->trigGroup);
      }
    }
    if ((kit_->muteGroups[track] != machine->muteGroup)) {
      if (machine->muteGroup == 255) {

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
        MD.setTrackParam(track, i, machine->params[i]);
      }
    }
  }
}
