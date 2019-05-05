#include "LoudnessPage.h"
#include "MCL.h"

void LoudnessPage::setup() { DEBUG_PRINT_FN(); }

void LoudnessPage::init() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  // classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  encoders[0]->cur = 100;
}

void LoudnessPage::cleanup() {
  //  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

float LoudnessPage::check_loudness() {
  //  for (uint8_t n = 0; n < 16; n++) {
  //   check_grid_loudness(n, row);
  //   }
  //

  // Get the current kit

  MidiUart.sendRaw(MIDI_STOP);
  MidiClock.handleImmediateMidiStop();

  GUI.flash_strings_fill("Recording", "Main Out");
  GUI.display();

  // Stop everything
  grid_page.prepare();

  // Get the current pattern
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  if (!MD.getBlockingPattern(MD.currentPattern)) {
    return;
  }

  // Copy pattern in to temp buffer
  //  MDPattern old_pattern;
  //  memcpy(&old_pattern, &MD.pattern, sizeof(old_pattern));

  MDTrack temp_track;
  temp_track.get_track_from_sysex(15, 15);
  // Clear track 15, and set step 1 to trig
  for (int x = 0; x < 64; x++) {
    MD.pattern.clear_step_locks(15, x);
  }
  MD.pattern.trigPatterns[15] = 1;
  // Send the modified pattern
  MD.pattern.origPosition = MD.currentPattern;
  mcl_actions.md_setsysex_recpos(8, MD.pattern.origPosition);
  MD.pattern.toSysex();
  // Set track 15 to ram_rec
  setup_ram_rec_in_kit();
  MD.kit.init_eq();
  MD.kit.init_dynamix();
  // Send back the origin kit
  mcl_actions.md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex();
  MD.loadKit(MD.pattern.kit);

  // float old_tempo = MidiClock.tempo;
  // MD.setTempo(240 * 24);
  // Disable internal seq for track 15
  mcl_seq.md_tracks[15].mute_state = SEQ_MUTE_ON;

  // Ready to go, start MD and run midi start routines
  MidiClock.handleImmediateMidiStart();
  MidiUart.sendRaw(MIDI_START);
  // MidiClock.start();
  // Record 1 bar
  while (MidiClock.div32th_counter < 31) {
  }
  // Stop everything
  MidiUart.sendRaw(MIDI_STOP);
  MidiClock.handleImmediateMidiStop();
  // Re-enable internal seq
  mcl_seq.md_tracks[15].mute_state = SEQ_MUTE_OFF;
  // MD.setTempo(old_tempo * 24);
  // Send back the orig pattern
  temp_track.place_track_in_kit(15, 15, &MD.kit, true);
  mcl_actions.md_setsysex_recpos(8, MD.pattern.origPosition);
  MD.pattern.toSysex();
  mcl_actions.md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex();
  MD.loadKit(MD.pattern.kit);
  // send recording to the megacommand and analyse
  midi_sds.use_hand_shake = false;

  MD.send_sample(49);
  if (!wait_for_sample()) {
    MD.clear_all_windows_quick();
    return 1.00;
  }
  DEBUG_PRINTLN("finished");
  MD.clear_all_windows_quick();
  midi_sds.wav_file.file.open(midi_sds.wav_file.filename, O_READ);
  int16_t peak = midi_sds.wav_file.find_peak();
  midi_sds.wav_file.file.close();
  float inc = (float)(0x7FFF - peak) / (float)0x7FFF;
  inc++;
  DEBUG_PRINTLN("Peak");
  DEBUG_PRINTLN(peak);
  DEBUG_PRINTLN(inc);
  return inc;
  //   write_tracks_to_md(0,grid_page.cur_col,0)l
}

void LoudnessPage::scale_vol(float inc) {
  DEBUG_PRINT_FN();
  EmptyTrack empty_track;

  MDTrack *md_track = (MDTrack *)&empty_track;

  grid_page.prepare();
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  MD.getBlockingPattern(MD.currentPattern);
  uint8_t seq_mute_states[NUM_MD_TRACKS];
  for (uint8_t a = 0; a < NUM_MD_TRACKS; a++) {
    seq_mute_states[a] = mcl_seq.md_tracks[a].mute_state;
    mcl_seq.md_tracks[a].mute_state = SEQ_MUTE_ON;
  }
  for (uint8_t n = 0; n < 16; n++) {
    md_track->get_track_from_sysex(n, n);
    memcpy(&(md_track->seq_data), &mcl_seq.md_tracks[n],
           sizeof(md_track->seq_data));
    md_track->scale_vol(inc);
    memcpy(&mcl_seq.md_tracks[n], &(md_track->seq_data),
           sizeof(md_track->seq_data));
    md_track->place_track_in_pattern(n, n, &MD.pattern);
    md_track->place_track_in_kit(n, n, &MD.kit);
  }

  mcl_actions.md_setsysex_recpos(8, MD.pattern.origPosition);
  MD.pattern.toSysex();

  mcl_actions.md_setsysex_recpos(4, MD.kit.origPosition);
  MD.kit.toSysex();

  MD.loadKit(MD.pattern.kit);
  for (uint8_t a = 0; a < NUM_MD_TRACKS; a++) {
    mcl_seq.md_tracks[a].mute_state = seq_mute_states[a];
  }
}

void LoudnessPage::setup_ram_rec_in_kit() {

  MD.kit.models[15] = RAM_R1_MODEL;
  // Sine wav, boosted usign FILTQ just before distortion, MLEV greater than -32
  // will cause clipping in sample. This should be a good measure of max
  // headroom. MLEV at 32
  MD.kit.params[15][RAM_R_MLEV] = 64;
  MD.kit.params[15][RAM_R_MBAL] = 64;
  MD.kit.params[15][RAM_R_CUE1] = 0;
  MD.kit.params[15][RAM_R_CUE2] = 0;
  MD.kit.params[15][RAM_R_ILEV] = 0;
  MD.kit.params[15][RAM_R_LEN] = 24;
  MD.kit.params[15][RAM_R_RATE] = 127;
  for (uint8_t n = 0; n < 16; n++) {
    if (MD.kit.trigGroups[n] == 15) {
      MD.kit.trigGroups[n] = 127;
    }
    if (MD.kit.muteGroups[n] == 15) {
      MD.kit.muteGroups[n] = 127;
    }
    if (MD.kit.lfos[n].destinationTrack == 15) {
      MD.kit.lfos[n].depth = 0;
      MD.kit.params[n][MODEL_LFOD] = 0;
    }
  }
}

void LoudnessPage::setup_ram_rec() {
  MDMachine ram_r1;

  ram_r1.model = RAM_R1_MODEL;
  ram_r1.params[RAM_R_MLEV] = 64;
  ram_r1.params[RAM_R_MBAL] = 64;
  ram_r1.params[RAM_R_ILEV] = 0;
  ram_r1.params[RAM_R_LEN] = 24;
  ram_r1.params[RAM_R_RATE] = 127;
  ram_r1.trigGroup = 127;
  ram_r1.muteGroup = 127;
  DEBUG_PRINTLN("setting machine");
  MD.setMachine(15, &ram_r1);
}

bool LoudnessPage::wait_for_sample() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif

  GUI.flash_strings_fill("SDS: Receieve", "");
  GUI.display();
#ifdef OLED_DISPLAY
  oled_display.drawRect(15, 23, 98, 6, WHITE);
#endif
  uint16_t start_clock = read_slowclock();
  while ((midi_sds.state == SDS_READY) &&
         ((clock_diff(start_clock, slowclock) < 3000))) {
    handleIncomingMidi();
  }
  if (midi_sds.state != SDS_REC) {
    return false;
  }
  DEBUG_PRINTLN("sds_rec = true");
  start_clock = read_slowclock();
  while ((midi_sds.state == SDS_REC) &&
         ((clock_diff(start_clock, slowclock) < 3000))) {
    handleIncomingMidi();
#ifdef OLED_DISPLAY
    if (midi_sds.packetNumber != last_midi_packet) {
      oled_display.fillRect(
          15, 23,
          ((float)midi_sds.samplesSoFar / (float)midi_sds.sampleLength) * 98, 6,
          WHITE);
      oled_display.display();
      last_midi_packet = midi_sds.packetNumber;
      start_clock = read_slowclock();
    }
#endif
  }
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  if (midi_sds.samplesSoFar != midi_sds.sampleLength) {
    return false;
  } else {
    return true;
  }
  //    GUI.display();
}

void LoudnessPage::check_grid_loudness(int col, int row) {
  DEBUG_PRINT_FN();

  setup_ram_rec();

  EmptyTrack empty_track;
  MDTrack *md_track = (MDTrack *)&empty_track;
  A4Track *a4_track = (A4Track *)&empty_track;
  ExtTrack *ext_track = (ExtTrack *)&empty_track;

  md_track->load_track_from_grid(col, row);
  MD.setMachine(0, &(md_track->machine));

  MD.triggerTrack(15, 127);
  MD.triggerTrack(0, 127);
  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;
  do {
    current_clock = read_slowclock();
    handleIncomingMidi();
    //    GUI.display();
  } while ((clock_diff(start_clock, current_clock) < 1000));

  MD.send_sample(49);
  delay(1000);
  wait_for_sample();
  DEBUG_PRINTLN("finished");
  midi_sds.wav_file.file.open(midi_sds.wav_file.filename, O_READ);
  DEBUG_PRINTLN(midi_sds.wav_file.find_peak());
  midi_sds.wav_file.file.close();
}

void LoudnessPage::display() {

  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
  GUI.setLine(GUI.LINE1);
  uint8_t x;
  // GUI.put_string_at(12,"Loudness");
  GUI.put_string_at(0, "LOUDNESS ");
  uint8_t msb = encoders[0]->cur / 100;
  uint8_t mantissa = encoders[0]->cur % 100;
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "Gain:");
  GUI.put_value_at(6, encoders[0]->cur);
  GUI.put_string_at(9, "%");
  /*
  GUI.put_value_at1(8,msb);
  GUI.put_string_at(9,".");
  GUI.put_value_at1(10,mantissa / 10);
  GUI.put_value_at1(11,mantissa % 10);
*/
#ifdef OLED_DISPLAY
#endif
}

bool LoudnessPage::handleEvent(gui_event_t *event) {
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

    scale_vol((float)encoders[0]->cur / (float)100);
    encoders[0]->cur = 100;
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    float inc = check_loudness();
    encoders[0]->cur = inc * 100;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON2) ||
      EVENT_PRESSED(event, Buttons.BUTTON3)) {
    GUI.setPage(&grid_page);
    return true;
  }

  return false;
}

MCLEncoder loudness_param1(0, 255, 2);
LoudnessPage loudness_page(&loudness_param1);
