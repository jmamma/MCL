#include "MCL_impl.h"
#include "ResourceManager.h"

#define STATE_NOSTATE 0
#define STATE_QUEUE 1
#define STATE_RECORD 2
#define STATE_PLAY 3
#define MONO 0
#define LINK 1

#define SOURCE_MAIN 0
#define SOURCE_INPA 1
#define SOURCE_INPB 2

#define SOURCE_INP 1

uint8_t RAMPage::rec_states[NUM_RAM_PAGES];
uint8_t RAMPage::slice_modes[NUM_RAM_PAGES];
bool RAMPage::cc_link_enable;

void RAMPage::setup() {
  DEBUG_PRINT_FN();
  encoders[3]->cur = 4;
}

void RAMPage::init() {
  DEBUG_PRINT_FN();
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
  trig_interface.off();
  cc_link_enable = true;
  if (mcl_cfg.ram_page_mode == MONO) {
    ((MCLEncoder *)encoders[0])->max = 2;
  } else {
    ((MCLEncoder *)encoders[0])->max = 1;
  }
  if (page_id == 0) {
    setup_callbacks();
  }
  if (mcl_cfg.ram_page_mode == LINK) {
    for (uint8_t n = 0; n < 4; n++) {
      if (page_id == 0) {
        encoders[n]->cur = ram_page_b.encoders[n]->cur;
      } else {
        encoders[n]->cur = ram_page_a.encoders[n]->cur;
      }
    }
  }
}

void RAMPage::cleanup() {
  oled_display.clearDisplay();
}
void RAMPage::setup_sequencer(uint8_t track) {

  USE_LOCK();
  SET_LOCK();
  mcl_seq.md_tracks[track].clear_track();
  mcl_seq.md_tracks[track].steps[0].trig = true;
  mcl_seq.md_tracks[track].length = encoders[3]->cur * 4;
  CLEAR_LOCK();
}

void RAMPage::prepare_link(uint8_t track, uint8_t steps, uint8_t row, uint8_t transition) {

  mcl_actions.links[track].row = row;
  mcl_actions.links[track].loops = 1;

  mcl_actions.send_machine[track] = 0;
  uint16_t next_step = (MidiClock.div16th_counter / steps) * steps + steps;
  grid_page.active_slots[track] = SLOT_PENDING;
  mcl_actions.transition_level[track] = transition;
  mcl_actions.next_transitions[track] = next_step;
  mcl_actions.transition_offsets[track] = 0;
  transition_step = next_step;
  record_len = (uint8_t)steps;
  mcl_actions.calc_next_transition();
  mcl_actions.calc_latency();
}


void RAMPage::setup_ram_rec(uint8_t track, uint8_t model, uint8_t lev,
                            uint8_t source, uint8_t len, uint8_t rate,
                            uint8_t pan, uint8_t linked_track) {
  MDTrack md_track;
  MDSeqTrack md_seq_track;
  bool clear_locks = true;
  bool send_params = false;

  md_seq_track.clear_track(clear_locks, send_params);

  md_track.machine.init();

  uint16_t steps = encoders[3]->cur * 4;
  RAMPage::rec_states[page_id] = STATE_QUEUE;
  if (mcl_cfg.ram_page_mode == LINK) {
    RAMPage::rec_states[0] = RAMPage::rec_states[1] = STATE_QUEUE;
  }
  md_track.active = MD_TRACK_TYPE;
  md_track.machine.model = model;

  if (source == SOURCE_MAIN) {
    md_track.machine.params[RAM_R_MLEV] = lev;
    md_track.machine.params[RAM_R_MBAL] = pan;
    md_track.machine.params[RAM_R_ILEV] = 0;
  }

  if (source >= SOURCE_INPA) {
    md_track.machine.params[RAM_R_MLEV] = 0;
    md_track.machine.params[RAM_R_IBAL] = pan;
    md_track.machine.params[RAM_R_ILEV] = lev;
  }

  md_track.machine.params[RAM_R_LEN] = encoders[3]->cur * 16;
  if (md_track.machine.params[RAM_R_LEN] > 127) {
    md_track.machine.params[RAM_R_LEN] = 127;
  }
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

  uint8_t timing_mid = md_seq_track.get_timing_mid();
  if (linked_track == 255) {
    md_track.machine.trigGroup = 255;
    md_seq_track.set_track_step(0,timing_mid,0);
    // md_track.seq_data.conditional[0] = 14;
  } else if (track > linked_track) {
    md_track.machine.trigGroup = linked_track;
    md_seq_track.set_track_step(0,timing_mid,0);
    // oneshot
    // md_track.seq_data.conditional[0] = 14;
  } else {
    md_track.machine.trigGroup = 255;
  }

  memcpy(&(md_track.seq_data), &md_seq_track, sizeof(MDSeqTrackData));

  md_track.machine.muteGroup = 127;
  md_track.link.init(mcl_actions.links[track].row, 0, steps, SEQ_SPEED_1X);

  mcl_actions.dev_sync_slot[0] = track;

  md_track.store_in_mem(track);

  prepare_link(track, steps, SLOT_RAM_RECORD, TRANSITION_UNMUTE);

}

void RAMPage::reverse(uint8_t track) {
  uint8_t model = (MD.kit.get_model(track));

  if (model != RAM_P1_MODEL && model != RAM_P2_MODEL && model != RAM_P3_MODEL &&
      model != RAM_P4_MODEL) {
    return;
  }
  if (RAMPage::slice_modes[page_id] == 0) {
    MD.setTrackParam(track, ROM_STRT, 127);
    MD.setTrackParam(track, ROM_END, 0);
  }
  if (RAMPage::slice_modes[page_id] == 1) {
    MD.setTrackParam(track, ROM_STRT, 0);
    MD.setTrackParam(track, ROM_END, 127);
  }
}

bool RAMPage::slice(uint8_t track, uint8_t linked_track) {
  uint8_t model = (MD.kit.get_model(track));

  if (grid_page.active_slots[track] != SLOT_RAM_PLAY) {
    return false;
  }
  uint8_t slices = 1 << encoders[2]->cur;

  uint8_t sample_inc = 128 / slices;
  uint8_t track_length = encoders[3]->cur * 4;
  uint8_t step_inc = track_length / slices;
  bool clear_locks = true;
  bool send_params = false;

  auto &trk = mcl_seq.md_tracks[track];
  auto &ln_trk = mcl_seq.md_tracks[linked_track];
  trk.clear_track(clear_locks, send_params);

  trk.locks_params[0] = ROM_STRT + 1;
  trk.locks_params[1] = ROM_END + 1;
  uint8_t mode = encoders[1]->cur;

  for (uint8_t s = 0; s < slices; s++) {
    uint8_t n = s * step_inc;

    if ((linked_track < track) || (linked_track == 255)) {
      trk.steps[n].trig = true;
    }
    trk.timing[n] = trk.get_timing_mid();
    if (linked_track < track) {
      trk.set_track_locks_i(n, 0, ln_trk.get_track_lock(n, 0));
      trk.set_track_locks_i(n, 1, ln_trk.get_track_lock(n, 1));
    } else if (RAMPage::slice_modes[page_id] == 0) {
      trk.set_track_locks_i(n, 0, sample_inc * s + 0);
      trk.set_track_locks_i(n, 1, (sample_inc) * (s + 1) + 0);
    } else {
      switch (mode) {
      default: {
        // Reverse
        trk.set_track_locks_i(n, 1, sample_inc * s + 0);
        uint8_t val = (sample_inc) * (s + 1) + 0;
        trk.set_track_locks_i(n, 0, val);
        break;
      }
      case 5: {
        trk.set_track_locks_i(n, 0, sample_inc * (slices - s) + 0);
        uint8_t val = (sample_inc) * (slices - s + 1) + 0;
        trk.set_track_locks_i(n, 1, val);

        break;
      }
      case 6: {
        uint8_t t;
        t = random(0, slices);
        trk.set_track_locks_i(n, 0, sample_inc * (t) + 0);
        uint8_t val = (sample_inc) * (t + 1) + 0;
        trk.set_track_locks_i(n, 1, val);

        break;
      }
      case 4:
      case 3:
      case 2:
      case 1:
        uint8_t m = mode;

        // Random
        if (m == 4) {
          if (get_random_byte() > 64) {
            m = s;
          } else {
            m = s + 1;
          }
        }

        else {
          while (m > slices) {
            m--;
          }
        }
        if (s % m == 0) {
          trk.set_track_locks_i(n, 1, sample_inc * s + 0);
          uint8_t val = (sample_inc) * (s + 1) + 0;
          trk.set_track_locks_i(n, 0, val);
        } else {
          trk.set_track_locks_i(n, 0, sample_inc * s + 0);
          uint8_t val = (sample_inc) * (s + 1) + 0;
          trk.set_track_locks_i(n, 1, val);
        }
        break;
      }
    }
  }
  return true;
}

void RAMPage::setup_ram_play(uint8_t track, uint8_t model, uint8_t pan,
                             uint8_t linked_track) {
  MDTrack md_track;
  MDSeqTrack md_seq_track;

  bool clear_locks = true;
  bool send_params = false;

  md_seq_track.clear_track(clear_locks, send_params);

  mcl_seq.md_tracks[track].clear_track(clear_locks, send_params); //make sure current track does not retrigger

  md_track.machine.init();

  uint8_t steps = encoders[3]->cur * 4;

  RAMPage::rec_states[page_id] = STATE_QUEUE;
  if (mcl_cfg.ram_page_mode == LINK) {
    RAMPage::rec_states[0] = RAMPage::rec_states[1] = STATE_QUEUE;
  }

  md_track.active = MD_TRACK_TYPE;
  md_track.machine.model = model;

  md_track.machine.params[ROM_PTCH] = 64;
  md_track.machine.params[ROM_DEC] = 64;
  md_track.machine.params[ROM_HOLD] = 127;
  md_track.machine.params[ROM_BRR] = 0;
  md_track.machine.params[ROM_STRT] = 0;
  md_track.machine.params[ROM_END] = 127;
  md_track.machine.params[ROM_RTRG] = 0;
  md_track.machine.params[ROM_RTIM] = 127;
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

  uint8_t timing_mid = md_seq_track.get_timing_mid();
  if (linked_track == 255) {
    md_track.machine.trigGroup = 255;
    md_seq_track.set_track_step(0,timing_mid,0);
  } else if (track > linked_track) {
    md_track.machine.trigGroup = linked_track;
    md_seq_track.set_track_step(0,timing_mid,0);
  } else {
    md_track.machine.trigGroup = 255;
  }

  memcpy(&(md_track.seq_data), &md_seq_track, sizeof(MDSeqTrackData));

  md_track.machine.muteGroup = 127;

  uint8_t magic = encoders[1]->cur;

  md_track.link.init(mcl_actions.links[track].row, 0, steps, SEQ_SPEED_1X);
  md_track.machine.params[MODEL_LFOD] = 0;
  md_track.machine.lfo.destinationTrack = track;

  mcl_actions.dev_sync_slot[0] = track;

  md_track.store_in_mem(track);
  prepare_link(track, steps, SLOT_RAM_PLAY);

}

void RAMPage::setup_ram_play_mono(uint8_t track) {
  RAMPage::slice_modes[page_id] = 0;
  uint8_t model = RAM_P1_MODEL;
  if (page_id == 1) {
    model = RAM_P2_MODEL;
  }
  setup_ram_play(track, model, 64);
}

void RAMPage::setup_ram_play_stereo(uint8_t track) {
  if (track == 15) {
    return;
  }

  RAMPage::slice_modes[page_id] = 0;
  setup_ram_play(track, RAM_P1_MODEL, 0, track + 1);
  setup_ram_play(track + 1, RAM_P2_MODEL, 127, track);
}

void RAMPage::setup_ram_rec_mono(uint8_t track, uint8_t lev, uint8_t source,
                                 uint8_t len, uint8_t rate) {
  uint8_t model = RAM_R1_MODEL;
  if (page_id == 1) {
    model = RAM_R2_MODEL;
  }
  uint8_t pan = 63;
  if (source == SOURCE_INPA) {
    pan = 0;
  }
  if (source == SOURCE_INPB) {
    pan = 127;
  }
  setup_ram_rec(track, model, lev, source, len, rate, pan);
}

void RAMPage::setup_ram_rec_stereo(uint8_t track, uint8_t lev, uint8_t source,
                                   uint8_t len, uint8_t rate) {
  if (track == 15) {
    return;
  }

  setup_ram_rec(track, RAM_R1_MODEL, lev, source, len, rate, 0, track + 1);
  uint8_t source_link_track = source;
  if (source >= SOURCE_INPA) {
    uint8_t source_link_track = SOURCE_INPB;
  }
  setup_ram_rec(track + 1, RAM_R2_MODEL, lev, source_link_track, len, rate, 127,
                track);
}

void RAMPage::loop() {

  // Prevent number of slices exceeding number of steps.
  uint8_t steps = encoders[3]->cur * 4;
  uint8_t slices = 1 << encoders[2]->cur;

  while (slices > steps) {
    encoders[2]->cur--;
    // encoders[2]->old = encoders[2]->cur;
    slices = 1 << encoders[2]->cur;
  }

  uint8_t n = 14 + page_id;
  if (grid_page.active_slots[n] == SLOT_RAM_RECORD) {
    if ((RAMPage::rec_states[page_id] == STATE_QUEUE) &&
        (MidiClock.div16th_counter == transition_step)) {
      if (mcl_cfg.ram_page_mode == LINK) {
        RAMPage::rec_states[0] = RAMPage::rec_states[1] = STATE_RECORD;
      } else {
        RAMPage::rec_states[page_id] = STATE_RECORD;
      }
      // else if ((RAMPage::rec_states[page_id] == STATE_RECORD) &&
      // (MidiClock.div16th_counter >= transition_step + record_len +
      // record_len)) {
    }
  } else if ((grid_page.active_slots[n] == SLOT_RAM_PLAY) &&
             (MidiClock.div16th_counter >= transition_step)) {
    if (mcl_cfg.ram_page_mode == LINK) {
      RAMPage::rec_states[0] = RAMPage::rec_states[1] = STATE_PLAY;
    } else {
      RAMPage::rec_states[page_id] = STATE_PLAY;
    }
  }
}

void RAMPage::display() {

  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
#ifndef OLED_DISPLAY
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  uint8_t x;

  GUI.put_string_at(0, "RAM");
  GUI.put_value_at1(4, page_id + 1);
  switch (RAMPage::rec_states[page_id]) {
  case STATE_QUEUE:
    GUI.put_string_at(6, "[Queue]");
    break;
  case STATE_RECORD:
    GUI.put_string_at(6, "[Record]");
    break;
  case STATE_PLAY:
    GUI.put_string_at(6, "[Play]");
    break;
  }

  GUI.setLine(GUI.LINE2);
  /*
    if (mcl_cfg.ram_page_mode == 0) {
      GUI.put_string_at(0, "MON");
    } else {
      GUI.put_string_at(0, "LNK");
    }
  */

  switch (encoders[0]->cur) {
  case SOURCE_MAIN:
    if (mcl_cfg.ram_page_mode == LINK) {
      if (page_id == 0) {
        GUI.put_string_at(0, "L");
      }
      if (page_id == 1) {
        GUI.put_string_at(0, "R");
      }
    } else {
      GUI.put_string_at(0, "M");
    }
    break;
  case SOURCE_INPA:
    if (mcl_cfg.ram_page_mode == LINK) {
      if (page_id == 0) {
        GUI.put_string_at(0, "INA");
      } else {
        GUI.put_string_at(0, "INB");
      }
    } else {
      GUI.put_string_at(0, "INA");
    }

    break;
  case SOURCE_INPB:
    if (mcl_cfg.ram_page_mode == LINK) {
      if (page_id == 0) {
        GUI.put_string_at(0, "INA");
      } else {
        GUI.put_string_at(0, "INB");
      }
    } else {
      GUI.put_string_at(0, "INB");
    }
    break;
  }
  GUI.put_value_at1(4, encoders[1]->cur);
  GUI.put_string_at(6, "S:");
  GUI.put_value_at2(8, 1 << encoders[2]->cur);
  GUI.put_string_at(11, "L:");
  GUI.put_value_at2(13, (encoders[3]->cur * 4));
#endif
#ifdef OLED_DISPLAY
  float remain;
  auto oldfont = oled_display.getFont();
  oled_display.setFont();
  oled_display.setCursor(28, 24);
  switch (RAMPage::rec_states[page_id]) {
  case STATE_QUEUE:
    oled_display.print(" [Queue]");
    break;
  case STATE_RECORD:
    oled_display.print(" [Record]");
    break;
  case STATE_PLAY:
    oled_display.print(" [Play]");
    break;
  }
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 32);

  oled_display.print("RAM ");
  oled_display.print(page_id + 1);

  oled_display.setCursor(105, 32);
  if (mcl_cfg.ram_page_mode == 0) {
    oled_display.print("MONO");
  } else {
    oled_display.print("LINK");
  }
  oled_display.setFont();
  oled_display.setCursor(0, 24);

  const char *source;

  switch (encoders[0]->cur) {
  case SOURCE_MAIN:
    if (mcl_cfg.ram_page_mode == LINK) {
      if (page_id == 0) {
        source = "L ";
      }
      if (page_id == 1) {
        source = "R";
      }
    } else {
      source = "INT";
    }
    break;
  case SOURCE_INPA:
    if (mcl_cfg.ram_page_mode == LINK) {
      if (page_id == 0) {
        source = "A ";
      } else {
        source = "B ";
      }
    } else {
      source = "A ";
    }

    break;
  case SOURCE_INPB:
    if (mcl_cfg.ram_page_mode == LINK) {
      if (page_id == 0) {
        source = "A ";
      } else {
        source = "B ";
      }
    } else {
      source = "B ";
    }
    break;
  }
  /*
    oled_display.print(encoders[1]->cur);
    oled_display.print(" S:");
    oled_display.print(1 << encoders[2]->cur);
    oled_display.print(" L:");
    oled_display.print(encoders[3]->cur * 4);
  */
  mcl_gui.draw_knob_frame();
  mcl_gui.draw_knob(0, "SRC", source);

  char val[4];

  mcl_gui.put_value_at(encoders[1]->cur, val);
  mcl_gui.draw_knob(1, "DICE", val);

  mcl_gui.put_value_at(1 << encoders[2]->cur, val);
  mcl_gui.draw_knob(2, "SLI", val);

  mcl_gui.put_value_at(encoders[3]->cur * 4, val);
  mcl_gui.draw_knob(3, "LEN", val);

  uint8_t w_x = 0, w_y = 2;
  oled_display.drawPixel(w_x + 24, w_y + 0, WHITE);
  oled_display.drawCircle(w_x + 24, w_y + 0, 2, WHITE);
  oled_display.drawLine(w_x + 12, w_y - 1, w_x + 24, w_y - 3, WHITE);
  oled_display.drawLine(w_x + 17, w_y + 15, w_x + 26, w_y + 2, WHITE);

  uint8_t progress_x = w_x + 0;
  uint8_t progress_y = w_y + 20;
  uint8_t progress_w = 19;
  oled_display.drawRoundRect(progress_x, progress_y, progress_w, 4, 1, WHITE);

  if ((RAMPage::rec_states[page_id] != STATE_NOSTATE)) {
    if (MidiClock.clock_less_than(transition_step + record_len,
                                  MidiClock.div16th_counter)) {

      remain = ((float)(MidiClock.div16th_counter) /
                (float)(transition_step + record_len));
    }
    //  else if (RAMPage::rec_states[page_id] == STATE_RECORD{
    else {
      uint8_t n = 14 + page_id;
      remain = (float)mcl_seq.md_tracks[n].step_count /
               (float)mcl_seq.md_tracks[n].length;
    }
    uint8_t width = remain * (progress_w - 1);
    oled_display.fillRect(progress_x + 1, progress_y, width, 4, WHITE);
  }

  switch (wheel_spin) {
  case 0:
    oled_display.drawBitmap(w_x, w_y, wheel_top, 19, 19, WHITE);
    break;
  case 1:
    oled_display.drawBitmap(w_x, w_y, wheel_angle, 19, 19, WHITE);
    break;
  case 2:
    oled_display.drawBitmap(w_x, w_y, wheel_side, 19, 19, WHITE);
    break;
  case 3:
    oled_display.drawBitmap(w_x, w_y, wheel_angle, 19, 19, WHITE, false, true);
    break;
  case 4:
    oled_display.drawBitmap(w_x, w_y, wheel_top, 19, 19, WHITE, false, true);
    break;
  case 5:
    oled_display.drawBitmap(w_x, w_y, wheel_angle, 19, 19, WHITE, true, true);
    break;
  case 6:
    oled_display.drawBitmap(w_x, w_y, wheel_side, 19, 19, WHITE, true, false);
    break;
  case 7:
    oled_display.drawBitmap(w_x, w_y, wheel_angle, 19, 19, WHITE, true, false);
    break;
  }
  if ((wheel_spin_last_clock != MidiClock.div16th_counter) &&
      ((RAMPage::rec_states[page_id] == STATE_RECORD) ||
       (RAMPage::rec_states[page_id] == STATE_PLAY))) {
    if ((RAMPage::slice_modes[page_id] == 1) &&
        (RAMPage::rec_states[page_id] != STATE_RECORD)) {
      if (wheel_spin == 0) {
        wheel_spin = 8;
      }
      wheel_spin--;
    } else {
      wheel_spin++;
      if (wheel_spin == 8) {
        wheel_spin = 0;
      }
    }
    wheel_spin_last_clock = MidiClock.div16th_counter;
  }
  oled_display.display();
  oled_display.setFont(oldfont);
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

  if ((mcl_cfg.ram_page_mode == MONO) || (!cc_link_enable)) {
    return;
  }

  MD.parseCC(channel, param, &track, &track_param);

  if (track_param == 32) { return; } //ignore mute

  if (grid_page.active_slots[track] != SLOT_RAM_PLAY) {
    return;
  }

  for (uint8_t n = 0; n < 16; n++) {

    if ((grid_page.active_slots[n] == SLOT_RAM_PLAY) && (n != track)) {
      if (track_param == MODEL_PAN) {
        /*
        if (n < track) {
          MD.setTrackParam(n, track_param, 127 - value);
          // Pan law.
          uint8_t lev = ((float)(value - 64) / (float)64) * (float)27 + 100;
          MD.setTrackParam(track, MODEL_VOL, lev);
          MD.setTrackParam(n, MODEL_VOL, lev);
        } else {
          MD.setTrackParam(n, track_param, 63 + (64 - value));
          uint8_t lev = ((float)(64 - value) / (float)64) * (float)27 + 100;
          MD.setTrackParam(track, MODEL_VOL, lev);
          MD.setTrackParam(n, MODEL_VOL, lev);
        }
        */
      } else {
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
    if (midi_active_peering.get_device(event->port)->id != DEVICE_MD) {
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
    if (mcl_cfg.ram_page_mode == MONO) {
      uint8_t lev = 64;
      if (encoders[0]->cur == SOURCE_MAIN) {
        lev = 64;
      }
      if (page_id == 0) {
        setup_ram_rec_mono(14, lev, encoders[0]->cur, 4 * encoders[3]->cur - 1,
                           128);
      } else {
        setup_ram_rec_mono(15, lev, encoders[0]->cur, 4 * encoders[3]->cur - 1,
                           128);
      }
    } else {
      if (encoders[0]->cur == SOURCE_MAIN) {
        setup_ram_rec_stereo(14, 64 - 16, encoders[0]->cur,
                             4 * encoders[3]->cur - 1, 128);
      }
      if (encoders[0]->cur == SOURCE_INP) {
        setup_ram_rec_stereo(14, 64 - 16, encoders[0]->cur,
                             4 * encoders[3]->cur - 1, 128);
      }
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    oled_display.textbox("DICE", "");
    RAMPage::slice_modes[page_id] = 1;
    if (mcl_cfg.ram_page_mode == MONO) {
      slice(14 + page_id, 255);
    } else {
      slice(14, 15);
      slice(15, 14);
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    RAMPage::slice_modes[page_id] = 0;
    oled_display.textbox("SLICE", "");
    if (mcl_cfg.ram_page_mode == MONO) {
      if (!slice(14 + page_id, 255)) {
        setup_ram_play_mono(14 + page_id);
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

MCLEncoder ram_a_param1(0, 1);
MCLEncoder ram_a_param2(0, 6);
MCLEncoder ram_a_param3(0, 5);
MCLEncoder ram_a_param4(1, 8);

MCLEncoder ram_b_param1(0, 1);
MCLEncoder ram_b_param2(0, 6);
MCLEncoder ram_b_param3(0, 5);
MCLEncoder ram_b_param4(1, 8);

RAMPage ram_page_a((uint8_t)0, &ram_a_param1, &ram_a_param2, &ram_a_param3,
                   &ram_a_param4);
RAMPage ram_page_b((uint8_t)1, &ram_a_param1, &ram_b_param2, &ram_b_param3,
                   &ram_b_param4);
