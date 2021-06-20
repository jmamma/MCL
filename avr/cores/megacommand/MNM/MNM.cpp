#include "MCL_impl.h"
#include "ResourceManager.h"

const ElektronSysexProtocol mnm_protocol = {
    monomachine_sysex_hdr,
    sizeof(monomachine_sysex_hdr),
    MNM_KIT_REQUEST_ID,
    MNM_PATTERN_REQUEST_ID,
    MNM_SONG_REQUEST_ID,
    MNM_GLOBAL_REQUEST_ID,
    MNM_STATUS_REQUEST_ID,

    MNM_CURRENT_AUDIO_TRACK_REQUEST,
    MNM_CURRENT_KIT_REQUEST,
    MNM_CURRENT_PATTERN_REQUEST,
    MNM_CURRENT_SONG_REQUEST,
    MNM_CURRENT_GLOBAL_SLOT_REQUEST,

    MNM_SET_STATUS_ID,
    MNM_SET_TEMPO_ID,
    MNM_SET_CURRENT_KIT_NAME_ID,
    11,

    MNM_LOAD_GLOBAL_ID,
    MNM_LOAD_PATTERN_ID,
    MNM_LOAD_KIT_ID,

    MNM_SAVE_KIT_ID,
};

MNMClass::MNMClass()
    : ElektronDevice(&Midi2, "MM", DEVICE_MNM, mnm_protocol) {
  global.baseChannel = 0;
  midiuart = &MidiUart2;
}

void MNMClass::init_grid_devices() {
  uint8_t grid_idx = 1;
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    add_track_to_grid(grid_idx, i, &(mcl_seq.ext_tracks[i]), MNM_TRACK_TYPE);
  }

}

bool MNMClass::probe() {
  if (255 != MNM.getCurrentKit(CALLBACK_TIMEOUT)) {
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo),
                          UART2_PORT);
    // wait 400 ms, should be enought time to allow midiclock tempo to be
    // calculated before proceeding.
    mcl_gui.delay_progress(400);

    if (!get_fw_caps()) {
#ifdef OLED_DISPLAY
      oled_display.textbox("UPGRADE ", "MONOMACHINE");
      oled_display.display();
#else
      gfx.display_text("UPGRADE", "MONOMACHINE");
#endif
      while (1)
        ;
    }

    if (!MNM.getBlockingGlobal(7)) {
      return false;
    }

    global.clockIn = true;
    global.clockOut = true;
    global.ctrlIn = true;
    global.ctrlOut = true;

    global.arpOut = 2;
    global.autotrackChannel = 9;
    global.baseChannel = 0;
    global.channelSpan = 6;
    global.keyboardOut = 2;
    global.midiClockOut = 1;
    global.transportIn = true;
    global.transportOut = true;
    global.origPosition = 7;

    MNMDataToSysexEncoder encoder(midi->uart);
    global.toSysex(&encoder);

    auto currentAudioMidiMode = getBlockingStatus(0x21);
    setStatus(0x21, 1);
    for (uint8_t x = 0; x < 3; x++) {
      for (uint8_t y = 0; y < 6; y++) {
        mcl_gui.delay_progress(10);
        setStatus(0x22, y);
      }
    }
    setStatus(0x21, currentAudioMidiMode);

    loadGlobal(7);
    return MNM.connected;
  }

  return false;
}

// Caller is responsible to make sure icons_device is loaded in RM
uint8_t* MNMClass::icon() {
  return R.icons_device->icon_mnm;
}


void MNMClass::sendNoteOn(uint8_t track, uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(track + global.baseChannel, note, velocity);
}

void MNMClass::sendNoteOff(uint8_t track, uint8_t note) {
  midiuart->sendNoteOff(track + global.baseChannel, note);
}

void MNMClass::sendMultiTrigNoteOn(uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(global.multitrigChannel, note, velocity);
}

void MNMClass::sendMultiTrigNoteOff(uint8_t note) {
  midiuart->sendNoteOff(global.multitrigChannel, note);
}

void MNMClass::sendMultiMapNoteOn(uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(global.multimapChannel, note, velocity);
}

void MNMClass::sendMultiMapNoteOff(uint8_t note) {
  midiuart->sendNoteOff(global.multimapChannel, note);
}

void MNMClass::sendAutoNoteOn(uint8_t note, uint8_t velocity) {
  midiuart->sendNoteOn(global.autotrackChannel, note, velocity);
}

void MNMClass::sendAutoNoteOff(uint8_t note) {
  midiuart->sendNoteOff(global.autotrackChannel, note);
}

void MNMClass::triggerTrack(uint8_t track, bool amp, bool lfo, bool filter) {
  midiuart->sendNRPN(global.baseChannel, (uint16_t)(0x7F << 7),
                     (uint8_t)((track << 3) | (amp ? 4 : 0) | (lfo ? 2 : 0) |
                               (filter ? 1 : 0)));
}

void MNMClass::setMultiEnvParam(uint8_t param, uint8_t value) {
  midiuart->sendNRPN(global.baseChannel, 0x40 + param, value);
}

void MNMClass::setMute(uint8_t track, bool mute) {
  midiuart->sendCC(track + global.baseChannel, 3, mute ? 0 : 1);
}

void MNMClass::setAutoMute(bool mute) {
  midiuart->sendCC(global.autotrackChannel, 3, mute ? 0 : 1);
}

void MNMClass::setMidiParam(uint8_t track, uint8_t param, uint8_t value) {
  midiuart->sendNRPN(global.baseChannel, (track << 7) | (0x38 + param), value);
}

void MNMClass::setTrackPitch(uint8_t track, uint8_t pitch) {
  midiuart->sendNRPN(global.baseChannel, (112 + track) << 7, pitch);
}

void MNMClass::setTrackLevel(uint8_t track, uint8_t level) {
  midiuart->sendCC(global.baseChannel + track, 7, level);
}

void MNMClass::setAutoParam(uint8_t param, uint8_t value) {
  if (param < 0x30) {
    midiuart->sendCC(global.autotrackChannel, param + 0x30, value);
  } else {
    midiuart->sendCC(global.autotrackChannel, param + 0x38, value);
  }
}

void MNMClass::setAutoLevel(uint8_t level) {
  midiuart->sendCC(global.autotrackChannel, 7, level);
}

void MNMClass::setParam(uint8_t track, uint8_t param, uint8_t value) {
  uint8_t cc = 0;
  if (param == 100) {
    cc = 0x3; // MUT
  } else if (param == 101) {
    cc = 0x7; // LEV;
  } else if (param < 0x10) {
    cc = param + 0x30;
  } else if (param < 0x28) {
    cc = param + 0x38;
  } else {
    cc = param + 0x40;
  }
  midiuart->sendCC(global.baseChannel + track, cc, value);
}

bool MNMClass::parseCC(uint8_t channel, uint8_t cc, uint8_t *track,
                       uint8_t *param) {
  if ((channel >= global.baseChannel) && (channel < (global.baseChannel + 6))) {
    *track = channel - global.baseChannel;
    if ((cc >= 0x30) && (cc <= 0x3f)) {
      *param = cc - 0x30;
      return true;
    } else if ((cc >= 0x48) && (cc <= 0x5f)) {
      *param = cc - 0x38;
      return true;
    } else if ((cc >= 0x68) && (cc <= 0x77)) {
      *param = cc - 0x40;
      return true;
    } else if (cc == 0x3) {
      *param = 100; // MUTE
      return true;
    } else if (cc == 0x07) {
      *param = 101; // LEV
      return true;
    }
  }

  return false;
}

void MNMClass::loadSong(uint8_t id) { setStatus(8, id); }

void MNMClass::setSequencerMode(bool songMode) {
  setStatus(0x10, songMode ? 1 : 0);
}

void MNMClass::setAudioMode(bool polyMode) {
  setStatus(0x20, polyMode ? 1 : 0);
}

void MNMClass::setSequencerModeMode(bool midiMode) {
  setStatus(0x21, midiMode ? 1 : 0);
}

void MNMClass::setAudioTrack(uint8_t track) { setStatus(0x22, track); }

void MNMClass::setMidiTrack(uint8_t track) { setStatus(0x23, track); }

void MNMClass::revertToCurrentKit(bool reloadKit) {
  if (!reloadKit) {
    if (loadedKit) {
      MNM.loadKit(MNM.currentKit);
    }
  } else {
    uint8_t kit = getCurrentKit(500);
    if (kit != 255) {
      MNM.loadKit(MNM.currentKit);
    }
  }
}

void MNMClass::revertToTrack(uint8_t track, bool reloadKit) {
  if (!reloadKit) {
    if (loadedKit) {
      setMachine(track, track);
    }
  }
}

void MNMClass::assignMachine(uint8_t track, uint8_t model, bool initAll,
                             bool initSynth) {
  uint8_t data[] = {MNM_LOAD_GLOBAL_ID, track, model, 0x00};
  if (initAll) {
    data[3] = 0x01;
  } else if (initSynth) {
    data[3] = 0x02;
  } else {
    data[3] = 0x00;
  }
  sendRequest(data, countof(data));
}

void MNMClass::insertMachineInKit(uint8_t track, MNMMachine *machine,
                                  bool set_level) {
  MNMKit *kit_ = &kit;

  memcpy(kit_->parameters[track], machine->params, 72);
  if (set_level) {
    kit_->levels[track] = machine->level;
  }
  kit_->models[track] = machine->model;

  memcpy(kit_->destPages + track, machine->modifier.destPage, 6 * 2);
  memcpy(kit_->destParams + track, machine->modifier.destParam, 6 * 2);
  memcpy(kit_->destRanges + track, machine->modifier.range, 6 * 2);

  kit_->hpKeyTrack = machine->modifier.HPKeyTrack;
  kit_->lpKeyTrack = machine->modifier.LPKeyTrack;
  kit_->mirrorLR = machine->modifier.mirrorLR;
  kit_->mirrorUD = machine->modifier.mirrorUD;

  kit_->trigLegatoAmp = machine->trig.legatoAmp;
  kit_->trigLegatoFilter = machine->trig.legatoFilter;
  kit_->trigLegatoLFO = machine->trig.legatoLFO;
  kit_->trigPortamento = machine->trig.portamento;
  kit_->trigTracks[track] = machine->trig.track;

  kit_->types[track] = machine->type;
}

void MNMClass::setMachine(uint8_t track, uint8_t idx) {
  assignMachine(track, kit.models[idx]);
  for (int i = 0; i < 56; i++) {
    setParam(track, i, kit.parameters[idx][i]);
  }
  setTrackLevel(track, kit.levels[idx]);
}

void MNMClass::updateKitParams() {
  for (uint8_t n = 0; n < NUM_MNM_TRACKS; n++) {
    // mcl_seq.ext_tracks[n].update_kit_params(); // huh.
  }
}

uint16_t MNMClass::sendKitParams(uint8_t *masks) {
  DEBUG_PRINT_FN();
  /// Ignores masks and scratchpad, and send the whole kit.
  auto kit_pos = getCurrentKit();
  kit.origPosition = kit_pos;
  // md_setsysex_recpos(4, kit_->origPosition);
  MNMDataToSysexEncoder encoder(&MidiUart2);
  kit.toSysex(&encoder);
  MNM.loadKit(kit_pos);
  //  mcl_seq.disable();
  // md_set_kit(&MNM.kit);
  uint16_t mnm_latency_ms =
      10000.0 * ((float)sizeof(MNMKit) / (float)MidiUart.speed);
  mnm_latency_ms += 10;
  DEBUG_DUMP(mnm_latency_ms);

  return mnm_latency_ms;
}
