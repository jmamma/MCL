#include "MCL_impl.h"

void MDMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  if (param >= 16) {
    MD.parseCC(channel, param, &track, &track_param);
    MD.kit.params[track][track_param] = value;

    last_md_param = track_param;
  } else {
    track = param - 8 + (channel - MD.global.baseChannel) * 4;
    MD.kit.levels[track] = value;
  }
}

void MDMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {}

void MDMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t note = msg[1];
  if ((channel == 0x0F) && (note == MIDI_NOTE_C3)) {
    SET_BIT16(MD.mute_mask, mute_mask_track);
  }
}

void MDClass::get_mute_state() {
  /*  Midi.addOnNoteOnCallback(MDMidiEvents,
   (midi_callback_ptr_t)&MDMidiEvents::onNoteOnCallback_Midi); for (uint8_t n =
   0; n < 16; n++) { MD.assignMachine(n, MID_16_MODEL, 0);
    midi_events.mute_mask_track = n;

    if (Kit.trigGroups[n] < 16) { MD.setTrigGroup(n, 127); }
    uint16_t start_clock = read_slowclock();
    uint16_t current_clock = start_clock;
    do {
      current_clock = read_slowclock();

      handleIncomingMidi();
    } while ((clock_diff(start_clock, current_clock) < timeout) &&
   !cb->received);

    if (Kit.trigGroups[n] < 16) { MD.setTrigGroup(n, Kit.trigGroups[n]); }
    assignMachine(n, kit->models[n]);
    setLFO(track, &(kit->lfos[track]), false);
    setTrigGroup(track, kit->trigGroups[track]);
    for (uint8_t i = 0; i < 8; i++) {
      setTrackParam(track, i, kit->params[track][i]);
    }


   }
   Midi.removeOnNoteOnCallback(
        &MDMidiEvents,
   (midi_callback_ptr_t)&MDMidiEvents::onNoteOnCallback_Midi);
  */
}

void MDMidiEvents::enable_live_kit_update() {
  if (kitupdate_state) {
    return;
  }
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MDMidiEvents::onControlChangeCallback_Midi);
  kitupdate_state = true;
}

void MDMidiEvents::disable_live_kit_update() {

  if (!kitupdate_state) {
    return;
  }
  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MDMidiEvents::onControlChangeCallback_Midi);
  kitupdate_state = false;
}

uint8_t machinedrum_sysex_hdr[5] = {0x00, 0x20, 0x3c, 0x02, 0x00};

const ElektronSysexProtocol md_protocol = {
  machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr),
  MD_KIT_REQUEST_ID, 
  MD_PATTERN_REQUEST_ID, 
  MD_SONG_REQUEST_ID, 
  MD_GLOBAL_REQUEST_ID,
  MD_STATUS_REQUEST_ID,

  MD_CURRENT_TRACK_REQUEST,
  MD_CURRENT_KIT_REQUEST,
  MD_CURRENT_PATTERN_REQUEST,
  MD_CURRENT_SONG_REQUEST,
  MD_CURRENT_GLOBAL_SLOT_REQUEST,

  MD_SET_STATUS_ID,
  MD_SET_TEMPO_ID,
  MD_SET_CURRENT_KIT_NAME_ID, 16,

  MD_LOAD_GLOBAL_ID,
  MD_LOAD_PATTERN_ID,
  MD_LOAD_KIT_ID,

  MD_SAVE_KIT_ID,
};

MDClass::MDClass() : ElektronDevice(&Midi, "MD", DEVICE_MD, icon_md, MD_TRACK_TYPE, NUM_MD_TRACKS, md_protocol)
{
  uint8_t standardDrumMapping[16] = {36, 38, 40, 41, 43, 45, 47, 48,
                                     50, 52, 53, 55, 57, 59, 60, 62};

  global.baseChannel = 0;
  for (int i = 0; i < 16; i++) {
    global.drumMapping[i] = standardDrumMapping[i];
  }
}

bool MDClass::probe() {
  DEBUG_PRINT_FN();

  bool ts = md_track_select.state;
  bool ti = trig_interface.state;

  if (ts) {
    md_track_select.off();
  }
  if (ti) {
    trig_interface.off();
  }

  // Hack to prevent unnecessary delay on MC boot
  connected = false;

  if ((slowclock > 3000) || (MidiClock.div16th_counter > 4)) {
    mcl_gui.delay_progress(4600);
  }

  // Begin main probe sequence
  if (MidiUart.device.getBlockingId(DEVICE_MD, UART1_PORT, CALLBACK_TIMEOUT)) {
    DEBUG_PRINTLN("Midi ID: success");
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo), 1);
    // wait 300 ms, shoul be enought time to allow midiclock tempo to be
    // calculated before proceeding.

    mcl_gui.delay_progress(400);
    md_exploit.send_globals();
    getCurrentTrack(CALLBACK_TIMEOUT);
    for (uint8_t x = 0; x < 2; x++) {
      for (uint8_t y = 0; y < 16; y++) {
        mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
        setStatus(0x22, y);
      }
    }
    setStatus(0x22, currentTrack);
    connected = true;
    setGlobal(7);
    global.baseChannel = 9;
    if (!get_fw_caps()) {
#ifdef OLED_DISPLAY
      oled_display.textbox("UPGRADE ", "MACHINEDRUM");
      oled_display.display();
#else
      gfx.display_text("UPGRADE", "MACHINEDRUM");
#endif
      while (1)
        ;
    }
    getBlockingKit(0xF7);
  }

  if (connected == false) {
    DEBUG_PRINTLN("delay");
    mcl_gui.delay_progress(250);
  }

  if (ts) {
    md_track_select.on();
  }
  if (ti) {
    trig_interface.on();
  }

  return connected;
}

uint8_t MDClass::noteToTrack(uint8_t pitch) {
  uint8_t i;
  if (MD.loadedGlobal) {
    for (i = 0; i < sizeof(MD.global.drumMapping); i++) {
      if (pitch == MD.global.drumMapping[i])
        return i;
    }
    return 128;
  } else {
    return 128;
  }
}

void MDClass::parseCC(uint8_t channel, uint8_t cc, uint8_t *track,
                      uint8_t *param) {
  if ((channel >= global.baseChannel) && (channel < (global.baseChannel + 4))) {
    channel -= global.baseChannel;
    *track = channel * 4;
    if (cc >= 96) {
      *track += 3;
      *param = cc - 96;
    } else if (cc >= 72) {
      *track += 2;
      *param = cc - 72;
    } else if (cc >= 40) {
      *track += 1;
      *param = cc - 40;
    } else if (cc >= 16) {
      *param = cc - 16;
    } else if (cc >= 12) {
      *track += (cc - 12);
      *param = 32; // MUTE
    } else if (cc >= 8) {
      *track += (cc - 8);
      *param = 33; // LEV
    }
  } else {
    *track = 255;
  }
}

void MDClass::triggerTrack(uint8_t track, uint8_t velocity) {
  if (global.drumMapping[track] != -1 && global.baseChannel != 127) {
    MidiUart.sendNoteOn(global.baseChannel, global.drumMapping[track],
                        velocity);
  }
}

void MDClass::setTrackParam(uint8_t track, uint8_t param, uint8_t value) {
  setTrackParam_inline(track, param, value);
}

void MDClass::setTrackParam_inline(uint8_t track, uint8_t param,
                                   uint8_t value) {

  uint8_t channel = track >> 2;
  uint8_t b = track & 3;
  uint8_t cc = 0;
  if (param < 32) {
    cc = param;
    if (b < 2) {
      cc += 16 + b * 24;
    } else {
      cc += 24 + b * 24;
    }
  } else if (param == 32) { // MUTE
    cc = 12 + b;
  } else if (param == 33) { // LEV
    cc = 8 + b;
  } else {
    return;
  }
  MidiUart.sendCC(channel + global.baseChannel, cc, value);
}

void MDClass::setSampleName(uint8_t slot, char *name) {
  uint8_t data[6];
  data[0] = MD_SAMPLE_NAME_ID;
  data[1] = slot;
  data[2] = 0x7F & name[0];
  data[3] = 0x7F & name[1];
  data[4] = 0x7F & name[2];
  data[5] = 0x7F & name[3];
  sendRequest(data, 6);
}

void MDClass::sendFXParam(uint8_t param, uint8_t value, uint8_t type) {
  uint8_t data[3] = {type, param, value};
  sendRequest(data, 3);
}

void MDClass::setEchoParam(uint8_t param, uint8_t value) {
  sendFXParam(param, value, MD_SET_RHYTHM_ECHO_PARAM_ID);
}

void MDClass::setReverbParam(uint8_t param, uint8_t value) {
  sendFXParam(param, value, MD_SET_GATE_BOX_PARAM_ID);
}

void MDClass::setEQParam(uint8_t param, uint8_t value) {
  sendFXParam(param, value, MD_SET_EQ_PARAM_ID);
}

void MDClass::setCompressorParam(uint8_t param, uint8_t value) {
  sendFXParam(param, value, MD_SET_DYNAMIX_PARAM_ID);
}

/*** tunings ***/

uint8_t MDClass::trackGetCCPitch(uint8_t track, uint8_t cc, int8_t *offset) {
  tuning_t const *tuning = getModelTuning(kit.models[track]);

  if (tuning == NULL)
    return 128;

  uint8_t i;
  int8_t off = 0;
  for (i = 0; i < tuning->len; i++) {
    uint8_t ccStored = pgm_read_byte(&tuning->tuning[i]);
    off = ccStored - cc;
    if (ccStored >= cc) {
      if (offset != NULL) {
        *offset = off;
      }
      if (off <= tuning->offset)
        return i + tuning->base;
      else
        return 128;
    }
  }
  off = ABS(pgm_read_byte(&tuning->tuning[tuning->len - 1]) - cc);
  if (offset != NULL)
    *offset = off;
  if (off <= tuning->offset)
    return i + tuning->base;
  else
    return 128;
}

uint8_t MDClass::trackGetPitch(uint8_t track, uint8_t pitch) {
  tuning_t const *tuning = getModelTuning(kit.models[track]);

  if (tuning == NULL)
    return 128;

  uint8_t base = tuning->base;
  uint8_t len = tuning->len;

  if ((pitch < base) || (pitch >= (base + len))) {
    return 128;
  }

  return pgm_read_byte(&tuning->tuning[pitch - base]);
}

void MDClass::sendNoteOn(uint8_t track, uint8_t pitch, uint8_t velocity) {
  uint8_t realPitch = trackGetPitch(track, pitch);
  if (realPitch == 128)
    return;
  setTrackParam(track, 0, realPitch);
  //  setTrackParam(track, 0, realPitch);
  //  delay(20);
  triggerTrack(track, velocity);
  //  delay(20);
  //  setTrackParam(track, 0, realPitch - 10);
  //  triggerTrack(track, velocity);
}

void MDClass::sliceTrack32(uint8_t track, uint8_t from, uint8_t to,
                           bool correct) {
  uint8_t pfrom, pto;
  if (from > to) {
    pfrom = MIN(127, from * 4 + 1);
    pto = MIN(127, to * 4);
  } else {
    pfrom = MIN(127, from * 4);
    pto = MIN(127, to * 4);
    if (correct && pfrom >= 64)
      pfrom++;
  }
  setTrackParam(track, 4, pfrom);
  setTrackParam(track, 5, pto);
  triggerTrack(track, 127);
}

void MDClass::sliceTrack16(uint8_t track, uint8_t from, uint8_t to) {
  if (from > to) {
    setTrackParam(track, 4, MIN(127, from * 8 + 1));
    setTrackParam(track, 5, MIN(127, to * 8));
  } else {
    setTrackParam(track, 4, MIN(127, from * 8));
    setTrackParam(track, 5, MIN(127, to * 8));
  }
  triggerTrack(track, 100);
}

bool MDClass::isMelodicTrack(uint8_t track) {
  return (getModelTuning(kit.models[track]) != NULL);
}

void MDClass::setLFOParam(uint8_t track, uint8_t param, uint8_t value) {
  uint8_t data[3] = {0x62, (uint8_t)(track << 3 | param), value};
  sendRequest(data, countof(data));
}

void MDClass::setLFO(uint8_t track, MDLFO *lfo, bool extra) {
  setLFOParam(track, 0, lfo->destinationTrack);
  setLFOParam(track, 1, lfo->destinationParam);
  setLFOParam(track, 2, lfo->shape1);
  setLFOParam(track, 3, lfo->shape2);
  setLFOParam(track, 4, lfo->type);
  if (extra) {
    setLFOParam(track, 5, lfo->speed);
    setLFOParam(track, 6, lfo->depth);
    setLFOParam(track, 7, lfo->mix);
  }
}

void MDClass::mapMidiNote(uint8_t pitch, uint8_t track) {
  uint8_t data[3] = {0x5a, pitch, track};
  sendRequest(data, countof(data));
}

void MDClass::resetMidiMap() {
  uint8_t data[1] = {0x64};
  sendRequest(data, countof(data));
}

void MDClass::setTrackRouting(uint8_t track, uint8_t output) {
  uint8_t data[3] = {0x5c, track, output};
  sendRequest(data, countof(data));
}

void MDClass::setTrigGroup(uint8_t srcTrack, uint8_t trigTrack) {
  uint8_t data[3] = {0x65, srcTrack, trigTrack};
  sendRequest(data, countof(data));
}

void MDClass::setMuteGroup(uint8_t srcTrack, uint8_t muteTrack) {
  uint8_t data[3] = {0x66, srcTrack, muteTrack};
  sendRequest(data, countof(data));
}

void MDClass::assignMachine(uint8_t track, uint8_t model, uint8_t init) {
  uint8_t send_length = 5;
  if (init == 255) {
    send_length = 4;
  }

  uint8_t data[] = {0x5B, track, model, 0x00, init};
  if (model >= 128) {
    data[2] = (model - 128);
    data[3] = 0x01;
  } else {
    data[2] = model;
    data[3] = 0x00;
  }
  sendRequest(data, send_length);
}

void MDClass::setMachine(uint8_t track, MDKit *kit) {
  // 138 bytes approx
  assignMachine(track, kit->models[track]);
  setLFO(track, &(kit->lfos[track]), false);
  setTrigGroup(track, kit->trigGroups[track]);
  setMuteGroup(track, kit->muteGroups[track]);
  // MidiUart.useRunningStatus = true;
  for (uint8_t i = 0; i < 24; i++) {
    setTrackParam(track, i, kit->params[track][i]);
  }
  // MidiUart.useRunningStatus = false;
}

void MDClass::setMachine(uint8_t track, MDMachine *machine) {
  // 138 bytes approx
  assignMachine(track, machine->model);
  setLFO(track, &(machine->lfo), false);
  if (machine->trigGroup == 255) {
    setTrigGroup(track, 127);
  } else {
    setTrigGroup(track, machine->trigGroup);
  }
  if (machine->muteGroup == 255) {

    setMuteGroup(track, 127);
  } else {
    setMuteGroup(track, machine->muteGroup);
  }
  //  MidiUart.useRunningStatus = true;
  for (uint8_t i = 0; i < 24; i++) {
    setTrackParam(track, i, machine->params[i]);
  }
  //  MidiUart.useRunningStatus = false;
}

void MDClass::muteTrack(uint8_t track, bool mute) {
  if (global.baseChannel == 127)
    return;

  uint8_t channel = track >> 2;
  uint8_t b = track & 3;
  uint8_t cc = 12 + b;
  MidiUart.sendCC(channel + global.baseChannel, cc, mute ? 1 : 0);
}

void MDClass::setGlobal(uint8_t id) {
  uint8_t data[] = {0x56, (uint8_t)(id & 0x7F)};
  sendRequest(data, countof(data));
}

void MDClass::loadSong(uint8_t song) { setStatus(8, song); }

void MDClass::setSequencerMode(uint8_t mode) { setStatus(16, mode); }

void MDClass::setLockMode(uint8_t mode) { setStatus(32, mode); }

void MDClass::getPatternName(uint8_t pattern, char str[5]) {
  uint8_t bank = pattern / 16;
  uint8_t num = pattern % 16 + 1;
  str[0] = 'A' + bank;
  str[1] = '0' + (num / 10);
  str[2] = '0' + (num % 10);
  str[3] = ' ';
  str[4] = 0;
}

bool MDClass::checkParamSettings() {
  if (loadedGlobal) {
    return (MD.global.baseChannel <= 12);
  } else {
    return false;
  }
}

bool MDClass::checkTriggerSettings() { return false; }

bool MDClass::checkClockSettings() { return false; }

// Perform checks on current sysex buffer to see if it Sysex.
//

void MDClass::send_gui_command(uint8_t command, uint8_t value) {
  USE_LOCK();
  SET_LOCK();
  MidiUart.m_putc(0xF0);
  MidiUart.sendRaw(machinedrum_sysex_hdr, sizeof(machinedrum_sysex_hdr));
  MidiUart.m_putc(MD_GUI_CMD);
  MidiUart.m_putc(command);
  MidiUart.m_putc(value);
  MidiUart.m_putc(0xF7);
  CLEAR_LOCK();
}

void MDClass::toggle_kit_menu() {
  send_gui_command(MD_GUI_KIT_WIN, MD_GUI_CMD_ON);
}

void MDClass::toggle_lfo_menu() {
  send_gui_command(MD_GUI_LFO_WIN, MD_GUI_CMD_ON);
}

void MDClass::hold_up_arrow() {
  send_gui_command(MD_GUI_UPARROW, MD_GUI_CMD_ON);
}

void MDClass::release_up_arrow() {
  send_gui_command(MD_GUI_UPARROW, MD_GUI_CMD_OFF);
}

void MDClass::hold_down_arrow() {
  send_gui_command(MD_GUI_DOWNARROW, MD_GUI_CMD_ON);
}

void MDClass::release_down_arrow() {
  send_gui_command(MD_GUI_DOWNARROW, MD_GUI_CMD_OFF);
}

void MDClass::hold_record_button() {
  send_gui_command(MD_GUI_RECORD, MD_GUI_CMD_ON);
}

void MDClass::release_record_button() {
  send_gui_command(MD_GUI_RECORD, MD_GUI_CMD_OFF);
}

void MDClass::press_play_button() {
  send_gui_command(MD_GUI_PLAY, MD_GUI_CMD_ON);
}

void MDClass::hold_stop_button() {
  send_gui_command(MD_GUI_STOP, MD_GUI_CMD_ON);
}

void MDClass::release_stop_button() {
  send_gui_command(MD_GUI_STOP, MD_GUI_CMD_OFF);
}

void MDClass::press_extended_button() {
  send_gui_command(MD_GUI_EXTENDED, MD_GUI_CMD_ON);
}

void MDClass::press_bankgroup_button() {
  send_gui_command(MD_GUI_BANKGROUP, MD_GUI_CMD_ON);
}

void MDClass::toggle_accent_window() {
  send_gui_command(MD_GUI_ACCENT_WIN, MD_GUI_CMD_ON);
}

void MDClass::toggle_swing_window() {
  send_gui_command(MD_GUI_SWING_WIN, MD_GUI_CMD_ON);
}

void MDClass::toggle_slide_window() {
  send_gui_command(MD_GUI_SLIDE_WIN, MD_GUI_CMD_ON);
}

void MDClass::hold_trig(uint8_t trig) {

  send_gui_command(MD_GUI_TRIG_1 + trig - 1, MD_GUI_CMD_ON);
}
void MDClass::release_trig(uint8_t trig) {

  send_gui_command(MD_GUI_TRIG_1 + trig - 1, MD_GUI_CMD_OFF);
}

void MDClass::hold_bankselect(uint8_t bank) {
  send_gui_command(MD_GUI_BANK_1 + bank - 1, MD_GUI_CMD_ON);
}

void MDClass::release_bankselect(uint8_t bank) {

  send_gui_command(MD_GUI_BANK_1 + bank - 1, MD_GUI_CMD_OFF);
}

void MDClass::toggle_tempo_window() {

  send_gui_command(MD_GUI_TEMPO_WIN, MD_GUI_CMD_ON);
}

void MDClass::hold_function_button() {

  send_gui_command(MD_GUI_FUNC, MD_GUI_CMD_ON);
}
void MDClass::release_function_button() {

  send_gui_command(MD_GUI_FUNC, MD_GUI_CMD_OFF);
}

void MDClass::hold_left_arrow() {
  send_gui_command(MD_GUI_LEFTARROW, MD_GUI_CMD_ON);
}
void MDClass::release_left_arrow() {

  send_gui_command(MD_GUI_LEFTARROW, MD_GUI_CMD_OFF);
}
void MDClass::hold_right_arrow() {
  send_gui_command(MD_GUI_RIGHTARROW, MD_GUI_CMD_ON);
}

void MDClass::release_right_arrow() {
  send_gui_command(MD_GUI_RIGHTARROW, MD_GUI_CMD_OFF);
}

void MDClass::press_yes_button() {

  send_gui_command(MD_GUI_YES, MD_GUI_CMD_ON);
}

void MDClass::press_no_button() { send_gui_command(MD_GUI_NO, MD_GUI_CMD_ON); }

void MDClass::hold_scale_button() {

  send_gui_command(MD_GUI_SCALE, MD_GUI_CMD_ON);
}
void MDClass::release_scale_button() {

  send_gui_command(MD_GUI_SCALE, MD_GUI_CMD_OFF);
}

void MDClass::toggle_scale_window() {
  send_gui_command(MD_GUI_SCALE_WIN, MD_GUI_CMD_ON);
}
void MDClass::toggle_mute_window() {

  send_gui_command(MD_GUI_MUTE_WIN, MD_GUI_CMD_ON);
}
void MDClass::press_patternsong_button() {

  send_gui_command(MD_GUI_PATTERNSONG, MD_GUI_CMD_ON);
}
void MDClass::toggle_song_window() {

  send_gui_command(MD_GUI_SONG_WIN, MD_GUI_CMD_ON);
}
void MDClass::toggle_global_window() {
  send_gui_command(MD_GUI_GLOBAL_WIN, MD_GUI_CMD_ON);
}

void MDClass::copy() { send_gui_command(MD_GUI_COPY, MD_GUI_CMD_ON); }
void MDClass::clear() { send_gui_command(MD_GUI_CLEAR, MD_GUI_CMD_ON); }
void MDClass::paste() { send_gui_command(MD_GUI_PASTE, MD_GUI_CMD_ON); }
void MDClass::toggle_synth_page() {

  send_gui_command(MD_GUI_SYNTH, MD_GUI_CMD_ON);
}
void MDClass::track_select(uint8_t track) {
  send_gui_command(MD_GUI_TRACK_1 - 1 + track, MD_GUI_CMD_ON);
}
void MDClass::encoder_button_press(uint8_t encoder) {

  send_gui_command(MD_GUI_ENC_1 - 1 + encoder, MD_GUI_CMD_ON);
}
void MDClass::tap_tempo() { send_gui_command(MD_GUI_TEMPO, MD_GUI_CMD_ON); }

void MDClass::set_record_off() {
  toggle_slide_window();
  hold_record_button();
  hold_record_button();
  release_record_button();
}
void MDClass::set_record_on() {
  toggle_slide_window();
  hold_record_button();
  release_record_button();
}
void MDClass::clear_all_windows() { set_record_off(); }
void MDClass::clear_all_windows_quick() {
  toggle_slide_window();
  press_no_button();
}
void MDClass::copy_pattern() {
  clear_all_windows();
  copy();
}
void MDClass::paste_pattern() { paste(); }

void MDClass::tap_right_arrow(uint8_t count) {
  while (count > 0) {
    hold_right_arrow();
    release_right_arrow();
    count--;
  }
}

void MDClass::tap_left_arrow(uint8_t count) {

  while (count > 0) {
    hold_left_arrow();
    release_left_arrow();
    count--;
  }
}

void MDClass::tap_up_arrow(uint8_t count) {
  while (count > 0) {
    hold_up_arrow();
    release_up_arrow();
    count--;
  }
}

void MDClass::tap_down_arrow(uint8_t count) {
  while (count > 0) {
    hold_down_arrow();
    release_down_arrow();
    count--;
  }
}

void MDClass::enter_global_edit() {
  uint8_t global = MD.getCurrentGlobal();
  if (global == 255) {
    return;
  }
  DEBUG_DUMP(global);
  clear_all_windows_quick();
  delay(10);
  toggle_global_window();
  tap_up_arrow(2);
  tap_left_arrow(3);
  if (global <= 4) {
    tap_right_arrow(global);
  } else {
    tap_down_arrow(1);
    tap_right_arrow(global - 4);
  }
  press_yes_button();
  tap_left_arrow();
  tap_up_arrow(3);
}

void MDClass::enter_sample_mgr() {
  enter_global_edit();
  tap_down_arrow(2);
  tap_right_arrow();
  tap_up_arrow(2);
  tap_down_arrow(2);
  press_yes_button();
  tap_left_arrow();
  tap_up_arrow(3);
}

void MDClass::preview_sample(uint8_t pos) {
  enter_sample_mgr();
  tap_right_arrow();
  hold_function_button();
  tap_up_arrow(13);
  release_function_button();
  tap_down_arrow(pos - 1);
}

void MDClass::rec_sample(uint8_t pos) {
  enter_sample_mgr();
  tap_right_arrow();
  hold_function_button();
  tap_up_arrow(13);
  release_function_button();

  if (pos == 255) {
    tap_up_arrow();
    press_yes_button();
    return;
  } else {
    tap_down_arrow(pos - 1);
    press_yes_button();
  }
}

void MDClass::send_sample(uint8_t pos) {

  enter_sample_mgr();
  tap_down_arrow();
  tap_right_arrow();
  hold_function_button();
  tap_up_arrow(13);
  release_function_button();

  if (pos == 255) {
    tap_up_arrow();
    press_yes_button();
    return;
  } else {
    tap_down_arrow(pos - 1);
    press_yes_button();
  }
}

void MDClass::setSysexRecPos(uint8_t rec_type, uint8_t position) {
  DEBUG_PRINT_FN();

  uint8_t data[] = {0x6b, (uint8_t)(rec_type & 0x7F), position, (uint8_t)1 & 0x7f};
  sendRequest(data, countof(data));
}

void MDClass::updateKitParams() {
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    mcl_seq.md_tracks[n].update_kit_params();
  }
}

MDClass MD;
