#include "MCL_impl.h"
#include "ResourceManager.h"

void MDMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  if (track == 255) {
    return;
  }

  if (param >= 16) {
    if (track > 15) {
      return;
    }
    if (track_param > 23) {
      return;
    }
    MD.kit.params[track][track_param] = value;
    last_md_param = track_param;
  } else {
    if (param > 7 && param < 12) {
      track = param - 8 + (channel - MD.global.baseChannel) * 4;
      MD.kit.levels[track] = value;
    }
  }
}

void MDMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {}

void MDMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}

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
    machinedrum_sysex_hdr,
    sizeof(machinedrum_sysex_hdr),
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
    MD_SET_CURRENT_KIT_NAME_ID,
    11,

    MD_LOAD_GLOBAL_ID,
    MD_LOAD_PATTERN_ID,
    MD_LOAD_KIT_ID,

    MD_SAVE_KIT_ID,
};

MDClass::MDClass() : ElektronDevice(&Midi, "MD", DEVICE_MD, md_protocol) {}

void MDClass::setup() {
  resetMidiMap();
  setTrackRoutings(mcl_cfg.routing);

  if (mcl_cfg.clock_rec == 0) {
    global.clockIn = false;
    global.clockOut = true;
  } else {
    global.clockIn = true;
    global.clockOut = false;
  }
  global.transportIn = true;
  global.transportOut = true;

  if (global.baseChannel == 0) {
    setBaseChannel(9);
  }

  setExternalSync();
  setProgramChange(2);
  setLocalOn(true);
}

void MDClass::setBaseChannel(uint8_t channel) {
  uint8_t data[3] = {0x70, 0x4A, channel};
  sendRequest(data, sizeof(data));
}

void MDClass::setLocalOn(bool localOn) {
  uint8_t data[3] = {0x70, 0x4B, localOn};
  sendRequest(data, sizeof(data));
}

void MDClass::setProgramChange(uint8_t val) {
  uint8_t data[3] = {0x70, 0x4C, val};
  sendRequest(data, sizeof(data));
}

void MDClass::setExternalSync() {
  uint8_t b = 0;
  //  clockIn = false;
  //  transportIn = true;
  //  clockOut = true;
  //  transportOut = true;

  b = global.clockIn;

  if (!global.transportIn) {
    b |= 1 << 4;
  }

  if (global.clockOut) {
    b |= 1 << 5;
  }

  if (global.transportOut) {
    b |= 1 << 6;
  }

  uint8_t data[3] = {0x70, 0x4D, b};
  sendRequest(data, sizeof(data));
}

void MDClass::init_grid_devices(uint8_t device_idx) {
  uint8_t grid_idx = 0;

  GridDeviceTrack gdt;
  for (uint8_t i = 0; i < NUM_MD_TRACKS; i++) {
    gdt.init(MD_TRACK_TYPE, GROUP_DEV, device_idx, &(mcl_seq.md_tracks[i]));
    add_track_to_grid(grid_idx, i, &gdt);
  }
  grid_idx = 1;

  gdt.init(MDFX_TRACK_TYPE, GROUP_DEV, device_idx,
           (SeqTrack *)&(mcl_seq.mdfx_track), 0);
  add_track_to_grid(grid_idx, MDFX_TRACK_NUM, &gdt);

  gdt.init(MDLFO_TRACK_TYPE, GROUP_PERF, device_idx,
           (SeqTrack *)&(mcl_seq.aux_tracks[0]), 0);
  add_track_to_grid(grid_idx, MDLFO_TRACK_NUM, &gdt);

  gdt.init(MDROUTE_TRACK_TYPE, GROUP_AUX, device_idx,
           (SeqTrack *)&(mcl_seq.aux_tracks[1]), 0);
  add_track_to_grid(grid_idx, MDROUTE_TRACK_NUM, &gdt);

  gdt.init(MDTEMPO_TRACK_TYPE, GROUP_TEMPO, device_idx,
           (SeqTrack *)&(mcl_seq.aux_tracks[2]), 0);
  add_track_to_grid(grid_idx, MDTEMPO_TRACK_NUM, &gdt);

  gdt.init(PERF_TRACK_TYPE, GROUP_PERF, device_idx,
           (SeqTrack *)&(mcl_seq.perf_track), 0);
  add_track_to_grid(grid_idx, PERF_TRACK_NUM, &gdt);
}

void MDClass::get_mutes() {
  uint16_t mutes;
  if (get_mute_state(mutes)) {
    for (uint8_t n = 0; n < 16; n++) {
      mcl_seq.md_tracks[n].mute_state = IS_BIT_SET16(mutes, n);
      DEBUG_PRINTLN(mcl_seq.md_tracks[n].mute_state);
    }
  } else {
    DEBUG_PRINTLN("mute state failed");
  }
}

bool MDClass::probe() {
  DEBUG_PRINT_FN();

  bool ts = md_track_select.state;
  bool ti = trig_interface.state;

  md_track_select.off();
  if (ti) {
    trig_interface.off();
  }
  DEBUG_PRINTLN("md probe");
  connected = false;

  // Begin main probe sequence
  if (uart->device.getBlockingId(DEVICE_MD, UART1_PORT, CALLBACK_TIMEOUT)) {
    uint8_t count = 3;

    uint16_t fw_caps_mask =
        ((uint16_t)FW_CAP_MASTER_FX | (uint16_t)FW_CAP_TRIG_LEDS |
         (uint16_t)FW_CAP_UNDOKIT_SYNC | (uint16_t)FW_CAP_TONAL |
         (uint16_t)FW_CAP_ENHANCED_GUI | (uint16_t)FW_CAP_ENHANCED_MIDI) |
        (uint16_t)FW_CAP_MACHINE_CACHE | (uint16_t)FW_CAP_UNDO_CACHE | (uint16_t)FW_CAP_MID_MACHINE;

    while ((!get_fw_caps() || ((fw_caps & fw_caps_mask) != fw_caps_mask)) &&
           count) {
      DEBUG_PRINTLN("bad caps");
      mcl_gui.delay_progress(250);
      count--;
    }

    if (((fw_caps & fw_caps_mask) != fw_caps_mask)) {
      oled_display.textbox("UPGRADE ", "MACHINEDRUM");
      oled_display.display();
      return false;
    }

    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart1_turbo_speed), uart);
    mcl_gui.delay_progress(100);

    //   if (mcl_cfg.clock_rec == 0) {
    //     MidiClock.uart_clock_recv = uart;
    //   }
    mcl_gui.delay_progress(300);
    getCurrentTrack(CALLBACK_TIMEOUT);
    getBlockingKit(0x7F);
    MD.save_kit_params();
    setup();

    uint8_t y = 0;
    for (uint8_t i = 0; i < 32; i++) {
      mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
      setStatus(0x22, y);
      y++;
      if (y == 16) y = 0;
    }
    setStatus(0x22, currentTrack);

    connected = true;
  }
  if (connected) {
    get_mutes();
    md_track_select.on();
    activate_enhanced_gui();
    activate_enhanced_midi();
    MD.set_key_repeat(1);
    MD.set_trigleds(0, TRIGLED_EXCLUSIVE);
    MD.global.extendedMode = 2;
    seq_ptc_page.setup();

  }

  else {
  abort:
    DEBUG_PRINTLN(F("delay"));
    mcl_gui.delay_progress(4600);
  }

  return connected;
}

// Caller is responsible to make sure icons_device is loaded in RM
uint8_t *MDClass::icon() { return R.icons_device->icon_md; }
MCLGIF *MDClass::gif() { return R.icons_logo->machinedrum_gif; }
uint8_t *MDClass::gif_data() { return R.icons_logo->machinedrum_gif_data; }

uint8_t MDClass::noteToTrack(uint8_t pitch) {
 for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
   if (pitch == MD.global.drumMapping[i])
        return i;
    }
 return 128;
}

void MDClass::parseCC(uint8_t channel, uint8_t cc, uint8_t *track,
                      uint8_t *param) {

  *track = (channel - global.baseChannel) * 4;

  if (cc < 16) {
    if (cc > 11) {
      *track += cc - 12;
      *param = 32; // MUTE
      return;
    }
    if (cc > 7) {
      *track += (cc - 8);
      *param = 33; // LEV
      return;
    }
    // Ignore General MIDI CC below 8
    *track = 255;
    return;
  }

  *param = cc;

  if (cc > 71) {
    *param -= 72 - 16;
    *track += 2;
  }

  *param -= 16;

  if (*param > 23) {
    *track += 1;
    *param -= 24;
  }

  if (*param > 23) {
    *track = 255;
    return;
  }

  return;
}

void MDClass::triggerTrack(uint8_t track, uint8_t velocity,
                           MidiUartParent *uart_) {
  if (uart_ == nullptr) {
    uart_ = uart;
  }

  if (global.drumMapping[track] != -1 && global.baseChannel != 127) {
    uart_->sendNoteOn(global.baseChannel, global.drumMapping[track], velocity);
  }
}

void MDClass::sync_seqtrack(uint8_t length, uint8_t speed, uint8_t step_count,
                            MidiUartParent *uart_) {
  if (uart_ == nullptr) {
    uart_ = uart;
  }
  uint8_t data[6] = {0x70, 0x3D, length, speed, step_count};
  sendRequest(data, sizeof(data), uart_);
}

void MDClass::parallelTrig(uint16_t mask, MidiUartParent *uart_) {
  if (uart_ == nullptr) {
    uart_ = uart;
  }
  uint8_t a;
  uint8_t b;
  uint8_t c;

  a = mask & 0x7F;
  mask = mask >> 7;
  c = mask >> 7 & 0xF7;
  b = mask & 0x7F;

  uart_->sendNoteOn(global.baseChannel + 1, a, b);
  if (c > 0) {
    uart_->sendNoteOn(global.baseChannel + 2, c, 0);
  }
}

void MDClass::save_kit_params() {
  memcpy(kit.params_orig, kit.params, sizeof(kit.params));
  memcpy(kit.fx_orig, kit.reverb, sizeof(kit.reverb) * 4);
}


void MDClass::restore_kit_params() {
  memcpy(kit.params, kit.params_orig, sizeof(kit.params));
  memcpy(kit.reverb, kit.fx_orig, sizeof(kit.reverb) * 4);
}

void MDClass::restore_kit_param(uint8_t track, uint8_t param) {
  if (MD.kit.params[track][param] != MD.kit.params_orig[track][param]) {
    MD.setTrackParam(track, param, MD.kit.params_orig[track][param], nullptr,
                     true);
  }
}

void MDClass::setTrackParam(uint8_t track, uint8_t param, uint8_t value,
                            MidiUartParent *uart_, bool update_kit) {
  setTrackParam_inline(track, param, value, uart_, update_kit);
}

void MDClass::setTrackParam_inline(uint8_t track, uint8_t param, uint8_t value,
                                   MidiUartParent *uart_, bool update_kit) {

  if (uart_ == nullptr) {
    uart_ = uart;
  }
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
    if (update_kit) {
      kit.params[track][param] = value;
    }
  } else if (param == 32) { // MUTE
    cc = 12 + b;
  } else if (param == 33) { // LEV
    if (update_kit) {
      kit.levels[track] = value;
    }
    cc = 8 + b;
  } else {
    return;
  }
  if (update_kit) {
    uart_->sendCC(channel + global.baseChannel, cc, value);
  } else {
    uart_->sendPolyKeyPressure(channel + global.baseChannel, cc, value);
  }
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

uint8_t MDClass::assignFXParamsBulk(uint8_t *values, bool send) {
  uint8_t data[2 + 8 * 4] = {0x70, 0x5a};
  memcpy(&data[2], values, 8 * 4);
  return sendRequest(data, sizeof(data), send);
}

uint8_t MDClass::sendFXParamsBulk(uint8_t *values, bool send) {
  uint8_t data[2 + 8 * 4] = {0x70, 0x61};
  memcpy(&data[2], values, 8 * 4);
  return sendRequest(data, sizeof(data), send);
}

uint8_t MDClass::sendFXParams(uint8_t *values, uint8_t type, bool send) {
  uint8_t data[2 + 8] = {0x70, type};
  memcpy(&data[2], values, 8);
  return sendRequest(data, sizeof(data), send);
}

uint8_t MDClass::setEchoParams(uint8_t *values, bool send) {
  return sendFXParams(values, MD_SET_RHYTHM_ECHO_PARAM_ID, send);
}
uint8_t MDClass::setReverbParams(uint8_t *values, bool send) {
  return sendFXParams(values, MD_SET_GATE_BOX_PARAM_ID, send);
}
uint8_t MDClass::setEQParams(uint8_t *values, bool send) {
  return sendFXParams(values, MD_SET_EQ_PARAM_ID, send);
}
uint8_t MDClass::setCompressorParams(uint8_t *values, bool send) {
  return sendFXParams(values, MD_SET_DYNAMIX_PARAM_ID, send);
}

void MDClass::setFXParam(uint8_t param, uint8_t value, uint8_t type,
                         bool update_kit, MidiUartParent *uart_) {

  if (uart_ == nullptr) {
    uart_ = uart;
  }
  uint8_t len = 4;
  if (update_kit) {
    switch (type) {
    case MD_FX_ECHO:
      MD.kit.delay[param] = value;
      break;
    case MD_FX_DYN:
      MD.kit.dynamics[param] = value;
      break;
    case MD_FX_REV:
      MD.kit.reverb[param] = value;
      break;
    case MD_FX_EQ:
      MD.kit.eq[param] = value;
      break;
    }
    len = 3;
  }
  uint8_t data[4] = {type, param, value, 0x7F};
  sendRequest(data, len);
}

void MDClass::setEchoParam(uint8_t param, uint8_t value) {
  return setFXParam(param, value, MD_SET_RHYTHM_ECHO_PARAM_ID);
}

void MDClass::setReverbParam(uint8_t param, uint8_t value) {
  return setFXParam(param, value, MD_SET_GATE_BOX_PARAM_ID);
}

void MDClass::setEQParam(uint8_t param, uint8_t value) {
  return setFXParam(param, value, MD_SET_EQ_PARAM_ID);
}

void MDClass::setCompressorParam(uint8_t param, uint8_t value) {
  return setFXParam(param, value, MD_SET_DYNAMIX_PARAM_ID);
}

/*** tunings ***/

uint8_t MDClass::trackGetCCPitch(uint8_t track, uint8_t cc, int8_t *offset) {
  tuning_t const *tuning = getKitModelTuning(track);

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
  tuning_t const *tuning = getKitModelTuning(track);

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
  return (getKitModelTuning(track) != NULL);
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

uint8_t MDClass::setTrackRoutings(uint8_t *values, bool send) {
  uint8_t data[2 + 16] = {0x70, 0x5c};
  memcpy(data + 2, values, 16);
  return sendRequest(data, sizeof(data), send);
}

uint8_t MDClass::setTrackRouting(uint8_t track, uint8_t output, bool send) {
  uint8_t data[3] = {0x5c, track, output};
  return sendRequest(data, countof(data), send);
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
  assignMachine(track, kit->get_model(track), kit->get_tonal(track));
  setLFO(track, &(kit->lfos[track]), false);
  setTrigGroup(track, kit->trigGroups[track]);
  setMuteGroup(track, kit->muteGroups[track]);
  // uart->useRunningStatus = true;
  for (uint8_t i = 0; i < 24; i++) {
    setTrackParam(track, i, kit->params[track][i]);
  }
  // uart->useRunningStatus = false;
}

void MDClass::setMachine(uint8_t track, MDMachine *machine) {
  // 138 bytes approx
  assignMachine(track, machine->get_model(), machine->get_tonal());
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
  //  uart->useRunningStatus = true;
  for (uint8_t i = 0; i < 24; i++) {
    setTrackParam(track, i, machine->params[i]);
  }
  //  uart->useRunningStatus = false;
}

uint8_t MDClass::assignMachineBulk(uint8_t track, MDMachine *machine,
                                   uint8_t level, uint8_t mode, bool send) {

  DEBUG_PRINT("assign machine bulk: ");
  DEBUG_PRINTLN(track);
  uint8_t data[43] = {0x70, 0x5b};
  uint8_t i = 2;
  data[i++] = track;
  if (machine->get_model() >= 128) {
    data[i++] = (machine->get_model() - 128);
    data[i] = 0x01;
  } else {
    data[i++] = machine->get_model();
    data[i] = 0x00;
  }
  if (machine->get_tonal()) {
    data[i] += 2;
  }
  i++;

  if (mode == 0) {
    goto end;
  }

  memcpy(data + i, machine->params, 24);
  i += 24;

  if (mode == 1) {
    goto end;
  }

  memcpy(data + i, &machine->lfo, 5);
  i += 5;
  if (machine->trigGroup > 15) {
    machine->trigGroup = 127;
  }
  if (machine->muteGroup > 15) {
    machine->muteGroup = 127;
  }
  data[i++] = machine->trigGroup;
  data[i++] = machine->muteGroup;
  if (level != 255) {
    data[i++] = level;
  }

end:
  return sendRequest(data, i, send);
}

void MDClass::loadMachinesCache(uint32_t track_mask, MidiUartParent *uart_) {
  DEBUG_PRINTLN("load machine cache");
  uint8_t a = track_mask & 0x7F;
  uint8_t b = (track_mask >> 7) & 0x7F;
  uint8_t c = (track_mask >> 14) & 0x7F;
  uint8_t data[5] = {0x70, 0x62, a, b, c};
  sendRequest(data, countof(data), uart_);
}

void MDClass::setOrigParams(uint8_t track, MDMachine *machine) {
  MDKit *kit_ = &kit;
  memcpy(kit_->params_orig[track], machine->params, 24);
}

void MDClass::insertMachineInKit(uint8_t track, MDMachine *machine,
                                 bool set_level) {

  DEBUG_PRINT("insert machine in kit ");
  DEBUG_PRINTLN(track);

  MDKit *kit_ = &kit;

  memcpy(kit_->params[track], machine->params, 24);
  setOrigParams(track, machine);

  if (set_level) {
    kit_->levels[track] = machine->level;
  }
  kit_->models[track] = machine->get_model_raw();

  if (machine->lfo.destinationTrack == track) {

    machine->lfo.destinationTrack = track;
  }
  // sanity check.
  if (machine->lfo.destinationTrack > 15) {
    DEBUG_PRINTLN(F("warning: lfo dest was out of bounds"));
    machine->lfo.destinationTrack = track;
  }
  memcpy(&(kit_->lfos[track]), &machine->lfo, sizeof(machine->lfo));

  if ((machine->trigGroup < 16) && (machine->trigGroup != track)) {
    kit_->trigGroups[track] = machine->trigGroup;
  } else {
    kit_->trigGroups[track] = 255;
  }

  if ((machine->muteGroup < 16) && (machine->muteGroup != track)) {
    kit_->muteGroups[track] = machine->muteGroup;
  } else {
    kit_->muteGroups[track] = 255;
  }
}

uint8_t MDClass::sendMachine(uint8_t track, MDMachine *machine, bool send_level,
                             bool send) {
  uint16_t bytes = 0;

  MDKit *kit_ = &kit;

  uint8_t level = 255;

  uint8_t track_ = track;
  if (track_ > 15) {
    track_ -= 16;
  }

  if ((send_level) && (kit_->levels[track_] != machine->level)) {
    level = machine->level;
  }

  bytes = MD.assignMachineBulk(track, machine, level, 255, send);

  return bytes;
}

void MDClass::muteTrack(uint8_t track, bool mute, MidiUartParent *uart_) {
  if (global.baseChannel == 127)
    return;
  if (uart_ == nullptr) {
    uart_ = uart;
  }
  uint8_t channel = track >> 2;
  uint8_t b = track & 3;
  uint8_t cc = 12 + b;
  uart_->sendCC(channel + global.baseChannel, cc, (uint8_t)mute);
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
  return (MD.global.baseChannel <= 12);
}

bool MDClass::checkTriggerSettings() { return false; }

bool MDClass::checkClockSettings() { return false; }

// Perform checks on current sysex buffer to see if it Sysex.
//

void MDClass::send_gui_command(uint8_t command, uint8_t value) {
  uint8_t buf[64];
  uint8_t i = 0;

  buf[i++] = 0xF0;

  for (uint8_t n = 0; n < sizeof(machinedrum_sysex_hdr); n++) {
    buf[i++] = machinedrum_sysex_hdr[n];
  }

  buf[i++] = MD_GUI_CMD;
  buf[i++] = command;
  buf[i++] = value;
  buf[i++] = 0xF7;

  uart->m_putc(buf, i);
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

  uint8_t data[] = {0x6b, (uint8_t)(rec_type & 0x7F), position,
                    (uint8_t)1 & 0x7f};
  sendRequest(data, countof(data));
}

void MDClass::updateKitParams() {

  uint16_t old_mutes[16];

  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    old_mutes[n] = mcl_seq.md_tracks[n].mute_state;
    mcl_seq.md_tracks[n].mute_state = SEQ_MUTE_ON;
  }
  undokit_sync();

  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    mcl_seq.md_tracks[n].mute_state = old_mutes[n];
  }
}

uint16_t MDClass::sendKitParams(uint8_t *masks) {
  /// Ignores masks and scratchpad, and send the whole kit.
  MD.kit.origPosition = 0x7F;
  MD.kit.toSysex();
  get_fw_caps();           //<-- includes waitBlocking, we need to wait for the
                           // sysex message to be received before unmuting seq
  return 0;
}
