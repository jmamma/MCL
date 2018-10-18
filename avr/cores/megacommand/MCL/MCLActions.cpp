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
  if (column < 16) {

    if (md_track->load_track_from_grid(column, row)) {
      //      md_track->store_in_mem(column, BANK1_R2_START);
      memcpy(&(chains[column]), &(md_track->chain), sizeof(GridChain));

      grid_page.active_slots[column] = row;
      if (md_track->active != EMPTY_TRACK_TYPE) {
        md_track->place_track_in_sysex(curtrack, column);
        return true;
      }
    }
  } else {
    if (Analog4.connected) {

      if (a4_track->load_track_from_grid(column, row, 0)) {
        //   a4_track->sta4_ore_in_mem(column, BANK1_R2_START);
        memcpy(&(chains[column]), &(a4_track->chain), sizeof(GridChain));

        grid_page.active_slots[column] = row;
        if (a4_track->active != EMPTY_TRACK_TYPE) {
          return a4_track->place_track_in_sysex(curtrack, column,
                                                analogfour_sound);
        }
      }
    } else {
      if (ext_track->load_track_from_grid(column, row, 0)) {
        //    a4_track->store_in_mem(column, BANK1_R2_START);
        memcpy(&(chains[column]), &(a4_track->chain), sizeof(GridChain));
        grid_page.active_slots[column] = row;
        if (ext_track->active != EMPTY_TRACK_TYPE) {
          return ext_track->place_track_in_sysex(curtrack, column);
        }
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

void MCLActions::store_tracks_in_mem(int column, int row,
                                     int store_behaviour_) {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;

  empty_track.chain.row = row;
  empty_track.chain.loops = 1;

  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  int16_t tclock = slowclock;
  uint8_t readpattern = MD.currentPattern;
  if ((gridio_param1.getValue() * 16 + gridio_param2.getValue()) !=
      MD.currentPattern) {
    readpattern = (gridio_param1.getValue() * 16 + gridio_param2.getValue());
  }

  store_behaviour = store_behaviour_;
  setLed();
  patternswitch = PATTERN_STORE;

  bool save_md_tracks = false;
  bool save_a4_tracks = false;
  uint8_t i = 0;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 3) {
      save_md_tracks = true;
    }
  }
  for (i = 16; i < 20; i++) {
    if (note_interface.notes[i] == 3) {
      save_a4_tracks = true;
    }
  }
  if (save_md_tracks) {
    if (!MD.getBlockingPattern(readpattern)) {
      DEBUG_PRINTLN("could not receive pattern");
      return;
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
  }

  // A4Track analogfour_track;
  //  MDTrack md_track->;

  bool n;
  /*Send a quick sysex message to get the current selected track of the MD*/

  //       int curtrack = 0;

  uint8_t first_note = 254;

  int curtrack = 0;
  if (store_behaviour == STORE_AT_SPECIFIC) {
    curtrack = last_md_track;
    // MD.getCurrentTrack(CALLBACK_TIMEOUT);
  }
  uint8_t max_notes = 20;
  if (!Analog4.connected) {
    max_notes = 16;
  }
  for (i = 0; i < max_notes; i++) {
    if (note_interface.notes[i] == 3) {
      if (first_note == 254) {
        first_note = i;
      }

      if (store_behaviour == STORE_IN_PLACE) {
        if ((i >= 16) && (i < 20)) {
          if (Analog4.connected) {
            DEBUG_PRINTLN("a4 get sound");
            Analog4.getBlockingSoundX(i - 16);
            a4_track->sound.fromSysex(MidiSysex2.data + 8,
                                      MidiSysex2.recordLen - 8);
          }
          n = a4_track->store_track_in_grid(i, i, grid_page.getRow());
        } else {
          n = md_track->store_track_in_grid(i, grid_page.getRow(), i);
        }
      }

      if ((store_behaviour == STORE_AT_SPECIFIC) && (i < 16)) {
        n = md_track->store_track_in_grid(grid_page.getCol() + i,
                                          grid_page.getRow(), (i - first_note));
      }
      // CLEAR_BIT32(note_interface.notes, i);
    }
  }

  for (uint8_t c = 0; c < 17; c++) {
    grid_page.row_headers[grid_page.cur_row].name[c] = MD.kit.name[c];
  }

  grid_page.row_headers[grid_page.cur_row].active = true;
  grid_page.row_headers[grid_page.cur_row].write(grid_page.getRow());

  // Sync project file to SD Card
  proj.file.sync();

  clearLed();
  DEBUG_PRINTLN(slowclock - tclock);
}

void MCLActions::write_tracks_to_md(int column, int row, int b) {
  DEBUG_PRINT_FN();

  store_behaviour = b;
  writepattern = MD.currentPattern;
  if (((gridio_param1.getValue() * 16 + gridio_param2.getValue()) !=
       MD.currentPattern)) {
    writepattern = (gridio_param1.getValue() * 16 + gridio_param2.getValue());
  }

  // Get pattern first, hopefully with the original kit assigned.
  if (write_original != 1) {
    if (!MD.getBlockingPattern(MD.currentPattern)) {
      DEBUG_PRINTLN("could not get blocking pattern");
      return;
    }
    if (gridio_param3.getValue() != MD.currentKit) {
      MD.currentKit = gridio_param3.getValue();
    } else {
      MD.saveCurrentKit(MD.currentKit);
    }

    MD.getBlockingKit(MD.currentKit);
  }
  patternswitch = 1;

  send_pattern_kit_to_md();
  patternswitch = PATTERN_UDEF;

  //  }

  // clearLed();
}

void MCLActions::send_pattern_kit_to_md() {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;
  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  // md_track->load_track_from_grid(0, grid_page.getRow());
  // if (!Analog4.getBlockingKitX(0)) { return; }
  // if (!analog4_kit.fromSysex(MidiSysex2.data + 8, MidiSysex2.recordLen - 8))
  // { return; }

  /*Send a quick sysex message to get the current selected track of the MD*/
  int curtrack = last_md_track;
  // MD.getCurrentTrack(CALLBACK_TIMEOUT);
  uint8_t reload = 1;
  uint16_t quantize_mute = 0;
  uint8_t q_pattern_change = 0;
  if (writepattern != MD.currentPattern) {
    reload = 0;
  }
  if (gridio_param4.getValue() == 0) {
    quantize_mute = 0;
  } else if (gridio_param4.getValue() < 7) {
    quantize_mute = 1 << gridio_param4.getValue();
  }
  if (gridio_param4.getValue() == 7) {
    quantize_mute = 254;
  }
  if (gridio_param4.getValue() == 8) {
    quantize_mute = 254;
  }
  if (gridio_param4.getValue() >= 9) {
    quantize_mute = MD.pattern.patternLength;
    q_pattern_change = 1;
    reload = 0;
    if ((gridio_param4.getValue() == 9) &&
        (writepattern == MD.currentPattern)) {
      reload = 1;
    }
    if (gridio_param4.getValue() == 10) {
      if (writepattern == 127) {
        writepattern = 0;
      } else {
        writepattern = writepattern + 1;
      }
      gridio_param4.cur = 11;
    } else if (gridio_param4.getValue() == 11) {
      if (writepattern == 0) {
        writepattern = 127;
      } else {
        writepattern = writepattern - 1;
      }
      gridio_param4.cur = 10;
    }
  }

  /*Define sysex encoder objects for the Pattern and Kit*/
  ElektronDataToSysexEncoder encoder(&MidiUart);
  ElektronDataToSysexEncoder encoder2(&MidiUart);
  /*Write the selected trackinto a Pattern object by moving it from a Track
    object into a Pattern object The destination track is the currently selected
    track on the machinedrum.
  */

  uint8_t i = 0;
  int track = 0;
  uint8_t note_count = 0;
  uint8_t first_note = 254;

  // Used as a way of flaggin which A4 tracks are to be sent
  uint8_t a4_send[6] = {0, 0, 0, 0, 0, 0};
  A4Sound sound_array[4];
  KitExtra kit_extra;

  volatile uint8_t *ptr;

  while ((i < 20)) {

    if ((note_interface.notes[i] > 1)) {
      if (first_note == 254) {
        first_note = i;
      }
      //  if (grid_page.encoders[0]->cur > 0) {
      if (store_behaviour == STORE_IN_PLACE) {
        track = i;

        if (i < 16) {
          place_track_inpattern(track, i, grid_page.getRow(),
                                (A4Sound *)&sound_array[0], &empty_track);

          if (i == first_note) {
            // Use first track's original kit values for write orig
            if (md_track->active != EMPTY_TRACK_TYPE) {
            memcpy(&kit_extra, &(md_track->kitextra), sizeof(kit_extra));
            }
            else {
            write_original = 0;
            }
            if (write_original == 1) {
              MD.pattern.patternLength = kit_extra.patternLength;
            }
          }
        } else {
          track = track - 16;
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

      else if ((curtrack + (i - first_note) < 16) && (i < 16)) {
        track = curtrack + (i - first_note);
        place_track_inpattern(track, i, grid_page.getRow(), &sound_array[0],
                              &empty_track);
      }

      if (gridio_param4.getValue() == 8) {
        if (i < 16) {
          MD.kit.levels[track] = 0;
        } else if (Analog4.connected) {
          Analog4.setLevel(i - 16, 0);
        }
      }
      //   }

      note_count++;
      if ((quantize_mute > 0) && (gridio_param4.getValue() < 8)) {
        if (i < 16) {
          MD.muteTrack(track, true);
        } else {
          mcl_seq.ext_tracks[i - 16].mute = SEQ_MUTE_ON;
        }
      }
    }
    i++;
  }

  /*Set the pattern position on the MD the pattern is to be written to*/

  setLed();

  /*Send the encoded pattern to the MD via sysex*/

  // int temp = MD.getCurrentKit(CALLBACK_TIMEOUT);

  /*Tell the MD to receive the kit sysexdump in the current kit position*/

  /* Retrieve the position of the current kit loaded by the MD.
    Use this position to store the modi
  */
  // If write original, let's copy the master fx settings from the first track
  // in row Let's also set the kit receive position to be the original.

  if ((write_original == 1)) {
    DEBUG_PRINTLN("write original");
    //     MD.kit.origPosition = md_track->origPosition;
    for (uint8_t c = 0; c < 17; c++) {
      MD.kit.name[c] = grid_page.row_headers[grid_page.cur_row].name[c];
    }
    memcpy(&MD.kit.reverb[0], kit_extra.reverb, sizeof(kit_extra.reverb));
    memcpy(&MD.kit.delay[0], kit_extra.delay, sizeof(kit_extra.delay));
    memcpy(&MD.kit.eq[0], kit_extra.eq, sizeof(kit_extra.eq));
    memcpy(&MD.kit.dynamics[0], kit_extra.dynamics, sizeof(kit_extra.dynamics));
    MD.pattern.swingAmount = kit_extra.swingAmount;
    MD.pattern.accentAmount = kit_extra.accentAmount;
    MD.pattern.doubleTempo = kit_extra.doubleTempo;
    MD.pattern.scale = kit_extra.scale;

    MD.pattern.accentEditAll = kit_extra.accentEditAll;
    MD.pattern.slideEditAll = kit_extra.slideEditAll;
    MD.pattern.swingEditAll = kit_extra.swingEditAll;

    MD.pattern.accentPattern = kit_extra.accentPattern;
    MD.pattern.slidePattern = kit_extra.slidePattern;
    MD.pattern.swingPattern = kit_extra.swingPattern;
  }

  // MD.kit.origPosition = MD.currentKit;

  // Kit
  // If Kit is OG.
  if (gridio_param3.getValue() == 64) {

    MD.kit.origPosition = md_track->origPosition;
    MD.pattern.kit = md_track->origPosition;

  }

  else {

    MD.pattern.kit = MD.currentKit;
    MD.kit.origPosition = MD.currentKit;
    //       }
  }
  // If Pattern is OG
  if (gridio_param1.getValue() == 8) {
    MD.pattern.origPosition = md_track->patternOrigPosition;
    reload = 0;
  } else {
    MD.pattern.setPosition(writepattern);
  }

  // MidiUart.setActiveSenseTimer(0);
  //in_sysex = 1;

  md_setsysex_recpos(8, MD.pattern.origPosition);

  MD.pattern.toSysex(encoder);
 // in_sysex = 1;

  /*Send the encoded kit to the MD via sysex*/
  md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex(encoder2);
  /*Instruct the MD to reload the kit, as the kit changes won't update until the
   * kit is reloaded*/
  //
  //    MidiUart.setActiveSenseTimer(290);
  if (reload == 1) {
    MD.loadKit(MD.pattern.kit);
  } else if ((q_pattern_change == 1) || (writepattern != MD.currentPattern)) {
    do_kit_reload = MD.pattern.kit;
    if (q_pattern_change == 1) {
      MD.loadPattern(writepattern);
    }
  }
  /*kit_sendmode != 1 therefore we are going to send the Machine via Sysex and
   * Midi cc without sending the kit*/

  // I fthe sequencer is running then we will pause and wait for the next
  // divison
  //  for (int n=0; n < 16; n++) {
  //      MD.global.baseChannel = 10;
  //      for (i=0; i < 16; i++) {
  //       MD.muteTrack(i,true);
  //      }
  // }
  //          delay(100);
  // Midiclock start hack

  // Send Analog4
  if (Analog4.connected) {
    uint8_t a4_kit_send = 0;
    // if (write_original == 1) {
    //in_sysex2 = 1;
    for (i = 0; i < 4; i++) {
      if (a4_send[i] == 1) {
        sound_array[i].toSysex();
      }
    }
    //in_sysex2 = 0;
  }

  if (mcl_actions.start_clock32th > MidiClock.div32th_counter) {
    mcl_actions.start_clock32th = 0;
  }
  if (mcl_actions.start_clock96th > MidiClock.div96th_counter) {
    mcl_actions.start_clock96th = 0;
  }

  if (quantize_mute > 0) {
    if (MidiClock.state == 2) {
      if ((q_pattern_change != 1) && (quantize_mute <= 64)) {
        // (MidiClock.div32th_counter - mcl_actions.start_clock32th)
        //                   while (((MidiClock.div32th_counter + 3) %
        //                   (quantize_mute * 2))  != 0) {
        while (
            (((MidiClock.div32th_counter - mcl_actions.start_clock32th) + 3) %
             (quantize_mute * 2)) != 0) {
          GUI.display();
        }
      }

      if (q_pattern_change != 1) {
        for (i = 0; i < 20; i++) {
          // If we're in cue mode, send the track to cue before unmuting
          if ((note_interface.notes[i] > 1)) {
            if ((gridio_param4.getValue() == 7) && (i < 16)) {
              SET_BIT32(mcl_cfg.cues, i);
              MD.setTrackRouting(i, 5);
            }
            if (i < 16) {
              MD.muteTrack(i, false);
            } else {
              mcl_seq.ext_tracks[i - 16].mute = SEQ_MUTE_OFF;
            }
          }
        }
      }
    }
    if (gridio_param4.getValue() == 7) {
      md_exploit.send_globals();
    }
  }
  // Pre-cache next chain
  // uint32_t mdlen = sizeof(GridTrack) + sizeof(MDSeqTrackData) +
  // sizeof(MDMachine);
  //
  for (uint8_t n = 0; n < 20; n++) {
    if (note_interface.notes[n] > 0) {
      // if (chains[n].active > 0) {

      DEBUG_PRINTLN("about to load");
      DEBUG_PRINTLN(chains[n].row);
      DEBUG_PRINTLN(n);
      if ((n < 16)) {
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

  //in_sysex = 0;
  if (MidiClock.state != 2) {
    for (uint8_t n = 0; n < 20; n++) {
      nearest_steps[n] = 0;
    }
  } else {
    for (uint8_t n = 0; n < 20; n++) {
      if (note_interface.notes[n] > 0) {
        uint32_t len;
        if (n < 16) {
          len = chains[n].loops * mcl_seq.md_tracks[n].length;
        } else {
          len = chains[n].loops * mcl_seq.ext_tracks[n - 16].length;
        }
        if (len < 4) {
          len = 4;
        }
        nearest_steps[n] = (MidiClock.div16th_counter / len) * len;
      }
    }
  }
  for (uint8_t n = 0; n < 20; n++) {
    if (grid_page.active_slots[n] >= 0) {
      calc_nearest_slot_step(n);
    }
  }
  calc_nearest_step();
  calc_latency(&empty_track);

  clearLed();
  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;
}

void MCLActions::calc_nearest_slot_step(uint8_t n) {

//  DEBUG_PRINT_FN();
  uint32_t step;
//  DEBUG_PRINTLN(n);
//  DEBUG_PRINTLN(nearest_steps[n]);
  if (n < 16) {
    step = chains[n].loops * mcl_seq.md_tracks[n].length;
    if (step < 4) {
      step = 4;
    }
    nearest_steps[n] += step;
  } else {
    step = chains[n].loops * mcl_seq.ext_tracks[n - 16].length;
    if (step < 4) {
      step = 4;
    }
    nearest_steps[n] += step;
  }
  DEBUG_PRINTLN(nearest_steps[n]);
}

void MCLActions::calc_nearest_step() {
  nearest_step = -1;
 // DEBUG_PRINT_FN();
  for (uint8_t n = 0; n < 20; n++) {
 //   DEBUG_PRINTLN(grid_page.active_slots[n]);
    if (grid_page.active_slots[n] >= 0) {
      if ((chains[n].loops > 0) ||
          (chains[n].row != grid_page.active_slots[n])) {

        if (nearest_steps[n] < nearest_step) {
   //       DEBUG_PRINTLN("setting nearest");
     //     DEBUG_PRINTLN( nearest_steps[n]);
       //   DEBUG_PRINTLN(n);
           nearest_step = nearest_steps[n];
        }
      }
    }
  }
  DEBUG_PRINTLN("current_step");
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN("nearest step");
  DEBUG_PRINTLN(nearest_step);
}

void MCLActions::calc_latency(EmptyTrack *empty_track) {
  MDTrack *md_track = (MDTrack *)empty_track;

  md_latency = 0;
  a4_latency = 0;

  for (uint8_t n = 0; n < 16; n++) {
    if (grid_page.active_slots[n] > 0) {
      if (n < 16) {
        if (nearest_steps[n] == nearest_step) {
          md_track->load_from_mem(n);
          md_latency +=
              calc_md_set_machine_latency(n, &(md_track->machine), &(MD.kit));
        } else {
          if (nearest_steps[n] == nearest_step) {
            a4_latency += A4_SOUND_LENGTH;
          }
        }
      }
    }
  }
  grid_task.active = true;
}
int MCLActions::calc_md_set_machine_latency(uint8_t track, MDMachine *machine,
                                            MDKit *kit_) {
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
  for (uint8_t i = 0; i < 24; i++) {
    if ((kit_->params[track][i] != machine->params[i]) || ((i < 8) && (kit_->models[track] != machine->model))) {
      //       (mcl_seq.md_tracks[track].is_param(i)))) {
      bytes += 3;
    }
  }

  return bytes;
}
void MCLActions::md_set_machine(uint8_t track, MDMachine *machine,
                                MDKit *kit_) {
  if (kit_ == NULL) {
    MD.setMachine(track, machine);
  } else {
    if (kit_->models[track] != machine->model) {
      MD.assignMachine(track, machine->model,0);
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
        MD.setTrigGroup(track, track);
      } else {
        MD.setTrigGroup(track, machine->trigGroup);
      }
    }
    if ((kit_->muteGroups[track] != machine->muteGroup)) {
      if (machine->muteGroup == 255) {

        MD.setMuteGroup(track, track);
      } else {
        MD.setMuteGroup(track, machine->muteGroup);
      }
    }
    //  MidiUart.useRunningStatus = true;
    //  mcl_seq.md_tracks[track].trigGroup = machine->trigGroup;

    //  mcl_seq.md_tracks[track].send_params = true;
    for (uint8_t i = 0; i < 24; i++) {

      if (((kit_->params[track][i] != machine->params[i])) || ((i < 8) && (kit_->models[track] != machine->model)) ) {
        //   (mcl_seq.md_tracks[track].is_param(i)))) {
        // mcl_seq.md_tracks[track].params[i] = machine->params[i];
        MD.setTrackParam(track, i, machine->params[i]);
      }

      else {
        mcl_seq.md_tracks[track].params[i] = 255;
      }
    }
  }
}
