#include "MCL.h"
#include "RAMPage.h"

#define STATE_NOSTATE 0
#define STATE_QUEUE 1
#define STATE_RECORD 2
#define STATE_PLAY 3

void RAMPage::setup() { DEBUG_PRINT_FN(); }

void RAMPage::init() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  // classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  encoders[0]->cur = 100;
  md_exploit.off();
  setup_callbacks();
}

void RAMPage::cleanup() {
  //  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}
void RAMPage::setup_sequencer(uint8_t track) {

  USE_LOCK();
  SET_LOCK();
  mcl_seq.md_tracks[track].pattern_mask = 1;
  mcl_seq.md_tracks[track].length = encoders[3]->cur * 4;
  CLEAR_LOCK();
}

void RAMPage::setup_ram_rec(uint8_t track, uint8_t model, uint8_t mlev,
                            uint8_t len, uint8_t rate, uint8_t pan,
                            uint8_t linked_track) {
  MDTrackLight md_track;

  memset(&(md_track.seq_data), 0, sizeof(MDSeqTrackData));
  memset(&(md_track.machine.params), 255, 24);

  uint16_t steps = encoders[3]->cur * 4;
  rec_state = STATE_QUEUE;
  md_track.active = MD_TRACK_TYPE;
  md_track.machine.model = model;
  md_track.machine.params[RAM_R_MLEV] = mlev;
  md_track.machine.params[RAM_R_MBAL] = pan;
  md_track.machine.params[RAM_R_ILEV] = 0;
  md_track.machine.params[RAM_R_LEN] = encoders[3]->cur * 16;
  if (md_track.machine.params[RAM_R_LEN] > 127) { md_track.machine.params[RAM_R_LEN] = 127; } 
  md_track.machine.params[RAM_R_RATE] = 127;
  md_track.machine.params[MODEL_AMD] = 0;
  md_track.machine.params[MODEL_AMF] = 0;
  md_track.machine.params[MODEL_EQF] = 64;
  md_track.machine.params[MODEL_EQG] = 64;
  md_track.machine.params[MODEL_FLTF] = 0;
  md_track.machine.params[MODEL_FLTW] = 127;
  md_track.machine.params[MODEL_FLTQ] = 0;
  md_track.machine.params[MODEL_SRR] = 0;
  md_track.machine.params[MODEL_DIST] = 0;
  md_track.machine.params[MODEL_VOL] = 127;
  md_track.machine.params[MODEL_PAN] = pan;
  md_track.machine.params[MODEL_DEL] = 0;
  md_track.machine.params[MODEL_REV] = 0;
  md_track.machine.params[MODEL_LFOS] = 64;
  md_track.machine.params[MODEL_LFOD] = 0;
  md_track.machine.params[MODEL_LFOM] = 0;
  md_track.machine.lfo.destinationTrack = track;

  if (linked_track == 255) {
    md_track.machine.trigGroup = 255;
    md_track.seq_data.pattern_mask = 1;
    md_track.seq_data.conditional[0] = 14;
  } else if (track > linked_track) {
    md_track.machine.trigGroup = linked_track;
    md_track.seq_data.pattern_mask = 1;
    // oneshot
    md_track.seq_data.conditional[0] = 14;
  } else {
    md_track.machine.trigGroup = 255;
    md_track.seq_data.pattern_mask = 0;
  }

  md_track.machine.muteGroup = 127;
  md_track.seq_data.length = (uint8_t)steps;
  md_track.chain.loops = 0;
  md_track.chain.row = mcl_actions.chains[track].row;

  md_track.store_in_mem(track);

  grid_page.active_slots[track] = 0x7FFF;
  mcl_actions.chains[track].row = SLOT_RAM_RECORD;
  mcl_actions.chains[track].loops = 1;
  uint16_t next_step = (MidiClock.div16th_counter / steps) * steps + steps;
  /*
    if (MD.kit.models[track] == md_track.machine.model) {
    mcl_actions.send_machine[track] = 1; } else {
    mcl_actions.send_machine[track] = 0; }
  */
  // mcl_actions.calc_next_slot_transition(track);
  mcl_actions.send_machine[track] = 0;
  mcl_actions.next_transitions[track] = next_step;
  mcl_actions.transition_level[track] = TRANSITION_UNMUTE;

  mcl_actions.calc_next_transition();

  EmptyTrack empty_track;
  mcl_actions.calc_latency(&empty_track);
}

void RAMPage::reverse(uint8_t track) {
  uint8_t model = (MD.kit.models[track]);

  if (model != RAM_P1_MODEL && model != RAM_P2_MODEL && model != RAM_P3_MODEL &&
      model != RAM_P4_MODEL) {
    return;
  }
  if (magic == 0) {
    MD.setTrackParam(track, ROM_STRT, 127);
    MD.setTrackParam(track, ROM_END, 0);
  }
  if (magic == 1) {
    MD.setTrackParam(track, ROM_STRT, 0);
    MD.setTrackParam(track, ROM_END, 127);
  }
}

bool RAMPage::slice(uint8_t track, uint8_t linked_track) {
  uint8_t model = (MD.kit.models[track]);

  if (grid_page.active_slots[track] != SLOT_RAM_PLAY) {
    return false;
  }
  uint8_t slices = 1 << encoders[2]->cur;

  uint8_t sample_inc = 128 / slices;
  uint8_t track_length = encoders[3]->cur * 4;
  uint8_t step_inc = track_length / slices;
  bool clear_locks = true;
  bool send_params = false;
  mcl_seq.md_tracks[track].clear_track(clear_locks, send_params);

  mcl_seq.md_tracks[track].locks_params[0] = ROM_STRT + 1;
  mcl_seq.md_tracks[track].locks_params[1] = ROM_END + 1;
  uint8_t mode = encoders[1]->cur;

  for (uint8_t s = 0; s < slices; s++) {
    uint8_t n = s * step_inc;

    if ((linked_track < track) || (linked_track == 255)) {
      SET_BIT64(mcl_seq.md_tracks[track].pattern_mask, n);
    }
    SET_BIT64(mcl_seq.md_tracks[track].lock_mask, n);
    if (linked_track < track) {
      mcl_seq.md_tracks[track].locks[0][n] =
          mcl_seq.md_tracks[linked_track].locks[0][n];
      mcl_seq.md_tracks[track].locks[1][n] =
          mcl_seq.md_tracks[linked_track].locks[1][n];
    } else if (magic == 0) {
      mcl_seq.md_tracks[track].locks[0][n] = sample_inc * s + 1;
      mcl_seq.md_tracks[track].locks[1][n] = (sample_inc) * (s + 1) + 1;
      if (mcl_seq.md_tracks[track].locks[1][n] > 128) {
        mcl_seq.md_tracks[track].locks[1][n] = 128;
      }
    } else {
      switch (mode) {
      default:
        // Reverse
        mcl_seq.md_tracks[track].locks[1][n] = sample_inc * s + 1;
        mcl_seq.md_tracks[track].locks[0][n] = (sample_inc) * (s + 1) + 1;
        if (mcl_seq.md_tracks[track].locks[0][n] > 128) {
          mcl_seq.md_tracks[track].locks[0][n] = 128;
        }
        break;
      case 6:

        mcl_seq.md_tracks[track].locks[0][n] = sample_inc * (slices - s) + 1;
        mcl_seq.md_tracks[track].locks[1][n] =
            (sample_inc) * (slices - s + 1) + 1;
        if (mcl_seq.md_tracks[track].locks[1][n] > 128) {
          mcl_seq.md_tracks[track].locks[1][n] = 128;
        }

        break;
      case 7:

        uint8_t t;
        t = random(0, slices);
        mcl_seq.md_tracks[track].locks[0][n] = sample_inc * (t) + 1;
        mcl_seq.md_tracks[track].locks[1][n] = (sample_inc) * (t + 1) + 1;
        if (mcl_seq.md_tracks[track].locks[1][n] > 128) {
          mcl_seq.md_tracks[track].locks[1][n] = 128;
        }

        break;
      case 4:
      case 3:
      case 2:
      case 1:
      case 5:
        // Revers every 2nd.
        //
        uint8_t m = mode;

        // Random
        if (m == 4) {
          if (get_random_byte() > 64) {
            m = s;
          } else {
            m = s + 1;
          }
        } else if (m == 5) {
          if (IS_BIT_SET64(mcl_seq.md_tracks[0].pattern_mask, n)) {
            m = s;
          } else {
            m = m + 1;
          }
        }

        else {
          while (m > slices) {
            m--;
          }
        }
        if (s % m == 0) {
          mcl_seq.md_tracks[track].locks[1][n] = sample_inc * s + 1;
          mcl_seq.md_tracks[track].locks[0][n] = (sample_inc) * (s + 1) + 1;
          if (mcl_seq.md_tracks[track].locks[0][n] > 128) {
            mcl_seq.md_tracks[track].locks[0][n] = 128;
          }
        } else {
          mcl_seq.md_tracks[track].locks[0][n] = sample_inc * s + 1;
          mcl_seq.md_tracks[track].locks[1][n] = (sample_inc) * (s + 1) + 1;
          if (mcl_seq.md_tracks[track].locks[1][n] > 128) {
            mcl_seq.md_tracks[track].locks[1][n] = 128;
          }
        }
        break;
      }
    }
  }
  return true;
}

void RAMPage::setup_ram_play(uint8_t track, uint8_t model, uint8_t pan,
                             uint8_t linked_track) {
  MDTrackLight md_track;

  memset(&(md_track.seq_data), 0, sizeof(MDSeqTrackData));
  memset(&(md_track.machine.params), 255, 24);

  uint16_t steps = encoders[3]->cur * 4;

  rec_state = STATE_QUEUE;
  md_track.active = MD_TRACK_TYPE;
  md_track.machine.model = model;
  /*
  md_track.machine.params[ROM_PTCH] = 64;
  md_track.machine.params[ROM_DEC] = 64;
  md_track.machine.params[ROM_HOLD] = 127;
  md_track.machine.params[ROM_BRR] = 0;
  md_track.machine.params[ROM_STRT] = 0;
  md_track.machine.params[ROM_END] = 127;
  md_track.machine.params[ROM_RTRG] = 0;
  md_track.machine.params[ROM_RTIM] = 127;
  */
  md_track.machine.params[MODEL_AMD] = 0;
  md_track.machine.params[MODEL_AMF] = 0;
  md_track.machine.params[MODEL_EQF] = 64;
  md_track.machine.params[MODEL_EQG] = 64;
  md_track.machine.params[MODEL_FLTF] = 0;
  md_track.machine.params[MODEL_FLTW] = 127;
  md_track.machine.params[MODEL_FLTQ] = 0;
  md_track.machine.params[MODEL_SRR] = 0;
  md_track.machine.params[MODEL_DIST] = 0;
  md_track.machine.params[MODEL_VOL] = 127;
  md_track.machine.params[MODEL_PAN] = pan;
  md_track.machine.params[MODEL_DEL] = 0;
  md_track.machine.params[MODEL_REV] = 0;
  md_track.machine.params[MODEL_LFOS] = 64;
  md_track.machine.params[MODEL_LFOD] = 0;
  md_track.machine.params[MODEL_LFOM] = 0;

  if (linked_track == 255) {
    md_track.machine.trigGroup = 255;
    md_track.seq_data.pattern_mask = 1;
  } else if (track > linked_track) {
    md_track.machine.trigGroup = linked_track;
    md_track.seq_data.pattern_mask = 1;
  } else {
    md_track.machine.trigGroup = 255;
    md_track.seq_data.pattern_mask = 0;
  }
  md_track.machine.muteGroup = 127;

  md_track.seq_data.length = (uint8_t)steps;
  uint8_t magic = encoders[1]->cur;
  md_track.chain.loops = 0;
  md_track.chain.row = mcl_actions.chains[track].row;
  md_track.machine.params[MODEL_LFOD] = 0;
  md_track.machine.lfo.destinationTrack = track;

  md_track.store_in_mem(track);

  mcl_actions.chains[track].row = SLOT_RAM_PLAY;
  mcl_actions.chains[track].loops = 1;
  mcl_actions.send_machine[track] = 0;

  uint16_t next_step;
  uint8_t m = mcl_seq.md_tracks[track].length;

  next_step =
      MidiClock.div16th_counter + (m - mcl_seq.md_tracks[track].step_count);
  grid_page.active_slots[track] = 0x7FFF;
  //mcl_actions.transition_level[track] = TRANSITION_MUTE;
  mcl_actions.next_transitions[track] = next_step;

  mcl_actions.calc_next_transition();

  EmptyTrack empty_track;
  mcl_actions.calc_latency(&empty_track);
}

void RAMPage::setup_ram_play_mono(uint8_t track) {
  magic = 0;
  setup_ram_play(track, RAM_P1_MODEL, 64);
}
void RAMPage::setup_ram_play_stereo(uint8_t track) {
  if (track == 15) {
    return;
  }

  magic = 0;
  setup_ram_play(track, RAM_P1_MODEL, 0, track + 1);
  setup_ram_play(track + 1, RAM_P2_MODEL, 127, track);
}

void RAMPage::setup_ram_rec_mono(uint8_t track, uint8_t mlev, uint8_t len,
                                 uint8_t rate) {
  setup_ram_rec(track, RAM_R1_MODEL, mlev, len, rate, 63);
}

void RAMPage::setup_ram_rec_stereo(uint8_t track, uint8_t mlev, uint8_t len,
                                   uint8_t rate) {
  if (track == 15) {
    return;
  }
  setup_ram_rec(track, RAM_R1_MODEL, mlev, len, rate, 0, track + 1);
  setup_ram_rec(track + 1, RAM_R2_MODEL, mlev, len, rate, 127, track);
}

void RAMPage::display() {

  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  uint8_t x;
  // GUI.put_string_at(12,"RAM");
  GUI.put_string_at(0, "RAM");
  uint8_t break_loop = 0;
  for (uint8_t n = NUM_MD_TRACKS - 1; n > 0 && break_loop == 0; n--) {
    if ((grid_page.active_slots[n] == SLOT_RAM_RECORD) &&
        (mcl_seq.md_tracks[n].step_count == 0)) {
       if (rec_state == STATE_QUEUE) {
        rec_state = STATE_RECORD;
      } else if ((rec_state == STATE_RECORD) &&
        (mcl_seq.md_tracks[n].oneshot_mask != 0)) {
        rec_state = STATE_NOSTATE;
      }
      break_loop = 1;
    } else if ((grid_page.active_slots[n] == SLOT_RAM_PLAY) &&
               (mcl_seq.md_tracks[n].step_count == 0)) {
      rec_state = STATE_PLAY;
      break_loop = 1;
    }
    // in_sysex = 0;
  }
  switch (rec_state) {
  case STATE_QUEUE:
    GUI.put_string_at(5, "[Queue]");
    break;
  case STATE_RECORD:
    GUI.put_string_at(5, "[Recording]");
    break;
  case STATE_PLAY:
    GUI.put_string_at(5, "[Playback]");
    break;
  }

  GUI.setLine(GUI.LINE2);

  if (encoders[0]->cur == 0) {
    GUI.put_string_at(0, "MONO");
  } else {
    GUI.put_string_at(0, "STER");
  }

  GUI.put_value_at(5, encoders[1]->cur);
  GUI.put_value_at(9, 1 << encoders[2]->cur);
  GUI.put_value_at(13, encoders[3]->cur);
/*
GUI.put_value_at1(8,msb);
GUI.put_string_at(9,".");
GUI.put_value_at1(10,mantissa / 10);
GUI.put_value_at1(11,mantissa % 10);
*/
#ifdef OLED_DISPLAY
#endif
}
void RAMPage::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks
  uint8_t param_true = 0;

  MD.parseCC(channel, param, &track, &track_param);

  if (grid_page.active_slots[track] != SLOT_RAM_PLAY) {
    return;
  }

  for (uint8_t n = 0; n < 16; n++) {

    if ((grid_page.active_slots[n] == SLOT_RAM_PLAY) && (n != track)) {
      if (track_param == MODEL_PAN) {
        if (n < track) {
          MD.setTrackParam(n, track_param,127 - value);
          //Pan law.
         uint8_t lev = ((float)(value - 64) / (float)64) * (float)27 + 100;
          MD.setTrackParam(track, MODEL_VOL, lev);
          MD.setTrackParam(n, MODEL_VOL, lev);
        }
        else {
          MD.setTrackParam(n, track_param,63 + (64 - value));
          uint8_t lev = ((float)(64 - value) / (float)64) * (float)27 + 100;
          MD.setTrackParam(track, MODEL_VOL, lev);
          MD.setTrackParam(n, MODEL_VOL, lev);
        }
      }
      else {
        MD.setTrackParam(n, track_param, value);
      }
    }
    // in_sysex = 0;
  }
}

void RAMPage::setup_callbacks() {
  if (midi_state) {
    return;
  }
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&RAMPage::onControlChangeCallback_Midi);

  midi_state = true;
}

void RAMPage::remove_callbacks() {
  if (!midi_state) {
    return;
  }

  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&RAMPage::onControlChangeCallback_Midi);

  midi_state = false;
}

bool RAMPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    if (encoders[0]->cur == 0) {
      setup_ram_rec_mono(15, 64, 4 * encoders[3]->cur - 1, 128);
    } else {
      setup_ram_rec_stereo(14, 64, 4 * encoders[3]->cur - 1, 128);
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    magic = 1;
    if (encoders[0]->cur == 0) {
      slice(15, 255);
    } else {
      slice(14, 15);
      slice(15, 14);
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    magic = 0;
    if (encoders[0]->cur == 0) {
      if (!slice(15, 255)) {
        setup_ram_play_mono(15);
      }
    } else {
      slice(14, 15);
      if (!slice(15, 14)) {
        setup_ram_play_stereo(14);
      }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  return false;
}

MCLEncoder ram_param1(0, 1, 2);
MCLEncoder ram_param2(0, 255, 2);
MCLEncoder ram_param3(0, 5, 2);
MCLEncoder ram_param4(1, 8, 2);

RAMPage ram_page(&ram_param1, &ram_param2, &ram_param3, &ram_param4);
