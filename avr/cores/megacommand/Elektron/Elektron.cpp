#include "Elektron.h"
#include "Project.h"
#include "ResourceManager.h"

#define SYSEX_RETRIES 1

void MidiDevice::add_track_to_grid(uint8_t grid_idx, uint8_t track_idx, GridDeviceTrack *gdt) {
  proj.grids[grid_idx].add_track(track_idx, gdt);
}

void MidiDevice::cleanup(uint8_t device_idx) {
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.grids[n].cleanup(device_idx);
  }
}
uint8_t *MidiDevice::gif_data() { return R.icons_logo->midi_gif_data; }
MCLGIF *MidiDevice::gif() { return R.icons_logo->midi_gif; }

uint16_t ElektronDevice::sendRequest(uint8_t *data, uint8_t len, bool send, MidiUartParent *uart_) {
   if (!send) {
        return len + sysex_protocol.header_size + 2;
    }

    uart_ = uart_ ? uart_ : uart;

    uint8_t buf[256];
    uint8_t i = 0;

    buf[i++] = 0xF0;
    memcpy(buf + i, sysex_protocol.header, sysex_protocol.header_size);
    i += sysex_protocol.header_size;

    for (uint8_t n = 0; n < len; n++) {
        buf[i++] = data[n] & 0x7F;
    }
    buf[i++] = 0xF7;

    uart_->m_putc(buf, i);

    return i;
}
/*
  if (uart_ == nullptr) { uart_ = uart; }

  uint8_t buf[256];

  if (send) {
    uint8_t i = 0;
    buf[i++] = 0xF0;

    for (uint8_t n = 0; n < sysex_protocol.header_size; n++) {
      buf[i++] = sysex_protocol.header[n];
    }

    for (uint8_t n = 0; n < len; n++) {
      buf[i++] = data[n] & 0x7F;
    }
    buf[i++] = 0xF7;
    uart_->m_putc(buf, i);
  }
  return len + sysex_protocol.header_size + 2;
}
*/
uint16_t ElektronDevice::sendRequest(uint8_t type, uint8_t param, bool send) {
  uint8_t data[] = {type, param};
  return sendRequest(data, 2, send);
}

bool ElektronDevice::get_tempo(uint16_t &tempo) {

  uint8_t data[2] = {0x70, 0x3F};
  sendRequest(data, sizeof(data));

  uint8_t msgType = waitBlocking();

  auto begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();
  listener->sysex->rd_cur = listener->msg_rd;

  tempo = 0;
  if (msgType == 0x72 && listener->sysex->getByte(begin) == 0x3F) {
      tempo = listener->sysex->getByte(begin+1) << 7;
      tempo |= (listener->sysex->getByte(begin+2));
      return true;
  }

  return false;
}


bool ElektronDevice::get_mute_state(uint16_t &mute_state) {

  uint8_t data[2] = {0x70, 0x33};
  sendRequest(data, sizeof(data));

  uint8_t msgType = waitBlocking();

  auto begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();
  listener->sysex->rd_cur = listener->msg_rd;
  mute_state = 0;
  if (msgType == 0x72 && listener->sysex->getByte(begin) == 0x33) {
      mute_state = listener->sysex->getByte(begin+1);
      mute_state |= (listener->sysex->getByte(begin+2) << 7);
      mute_state |= (listener->sysex->getByte(begin+3) << 14);
      return true;
  }

  return false;
}


bool ElektronDevice::get_fw_caps() {

  uint8_t data[2] = {0x70, 0x30};
  sendRequest(data, sizeof(data));

  uint8_t msgType = waitBlocking();

  fw_caps = 0;

  auto begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();
  DEBUG_PRINTLN("caps");
  listener->sysex->rd_cur = listener->msg_rd;
  if (msgType == 0x72 && listener->sysex->getByte(begin) == 0x30) {
      ((uint8_t *)&(fw_caps))[0] = listener->sysex->getByte(begin+1);
      ((uint8_t *)&(fw_caps))[1] = listener->sysex->getByte(begin+2);
      return true;
  }

  return false;
}

void ElektronDevice::activate_encoder_interface(uint8_t *params) {
  encoder_interface = true;
  uint8_t data[3 + 4 + 24] = {0x70, 0x36, 0x01};
  uint8_t mod7 = 0;
  uint8_t cnt = 0;

  for (uint8_t n = 0; n < 24; n++) {
    if (params[n] != 255) {
      data[3 + cnt] |= (1 << mod7);
      data[3 + 4 + n] = params[n];
    }
    mod7++;
    if (mod7 == 7) {
      mod7 = 0;
      cnt++;
    }
  }
  sendRequest(data, sizeof(data));
  //waitBlocking();
}

void ElektronDevice::deactivate_encoder_interface() { sendCommand(ElektronCommand::ActivateEncoderInterface, 0); }

void ElektronDevice::sendCommand(ElektronCommand command, uint8_t param) {
  uint8_t data[3] = {0x70, 0x00, 0x00};
  bool needsWait = false;
  uint8_t l = 3;
  switch (command) {
    case ElektronCommand::ActivateEncoderInterface:
      data[1] = 0x36;
      data[2] = param;;
      encoder_interface = param;
      break;
    case ElektronCommand::ActivateEnhancedMidi:
      data[1] = 0x3E;
      data[2] = param;
      break;
    case ElektronCommand::ActivateEnhancedGui:
      data[1] = 0x37;
      data[2] = param;
      break;
    case ElektronCommand::SetSeqPage:
      data[1] = 0x38;
      data[2] = param;
      break;
    case ElektronCommand::SetRecMode:
      data[1] = 0x3A;
      data[2] = param;
      break;
    case ElektronCommand::SetKeyRepeat:
      data[1] = 0x4E;
      data[2] = param;
      break;
    case ElektronCommand::ActivateTrigInterface:
      data[1] = 0x31;
      data[2] = param;
      break;
    case ElektronCommand::ActivateTrackSelect:
      data[1] = 0x32;
      data[2] = param;
      needsWait = true;
      break;
    case ElektronCommand::UndokitSync:
      data[1] = 0x42;
      l = 2;
      break;
    case ElektronCommand::ResetDspParams:
      data[1] = 0x43;
      l = 2;
      break;
    case ElektronCommand::DrawCloseBank:
      data[1] = 0x3C;
      data[2] = 0x23;
      break;
    case ElektronCommand::DrawCloseMicrotiming:
      data[1] = 0x3C;
      data[2] = 0x21;
      break;
    default:
      return; // Invalid command
  }

  sendRequest(data, l);
  if (needsWait) {
    waitBlocking();
  }
}


void ElektronDevice::draw_microtiming(uint8_t speed, uint8_t timing) {
  uint8_t a = timing >> 7;
  uint8_t b = timing & 0x7F;
  uint8_t data[6] = {0x70, 0x3C, 0x20, speed, a, b};
  sendRequest(data, 6);
  // waitBlocking();
}

void ElektronDevice::draw_pattern_idx(uint8_t idx, uint8_t idx_other, uint8_t chain_mask) {
  uint8_t data[6] = {0x70, 0x3C, 0x24, idx, idx_other, chain_mask };
  sendRequest(data, 6);
  // waitBlocking();
}


void ElektronDevice::popup_text(uint8_t action_string, uint8_t persistent) {
  uint8_t data[4] = {0x70, 0x3B, persistent, action_string};
  sendRequest(data, 4);
  // waitBlocking();
}

void ElektronDevice::popup_text(char *str, uint8_t persistent) {
  uint8_t data[67] = {0x70, 0x3B, persistent};
  uint8_t len = strlen(str);
  strcpy((char*) (data + 3), str);
  sendRequest(data, 3 + len + 1);
  // waitBlocking();
}

void ElektronDevice::draw_bank(uint8_t bank) {
  uint8_t data[5] = {0x70, 0x3C, 0x22, bank};
  sendRequest(data, 5);
  // waitBlocking();
}

// Implement the individual functions using the central sendCommand function
void ElektronDevice::activate_enhanced_midi() { sendCommand(ElektronCommand::ActivateEnhancedMidi, 1); }
void ElektronDevice::deactivate_enhanced_midi() { sendCommand(ElektronCommand::ActivateEnhancedMidi, 0); }
void ElektronDevice::activate_enhanced_gui() { sendCommand(ElektronCommand::ActivateEnhancedGui, 1); }
void ElektronDevice::deactivate_enhanced_gui() { sendCommand(ElektronCommand::ActivateEnhancedGui, 0); }
void ElektronDevice::set_seq_page(uint8_t page) { sendCommand(ElektronCommand::SetSeqPage, page); }
void ElektronDevice::set_rec_mode(uint8_t mode) { sendCommand(ElektronCommand::SetRecMode, mode); }
void ElektronDevice::set_key_repeat(uint8_t mode) { sendCommand(ElektronCommand::SetKeyRepeat, mode); }
void ElektronDevice::activate_trig_interface() { sendCommand(ElektronCommand::ActivateTrigInterface, 1); }
void ElektronDevice::deactivate_trig_interface() { sendCommand(ElektronCommand::ActivateTrigInterface, 0); }
void ElektronDevice::activate_track_select() { sendCommand(ElektronCommand::ActivateTrackSelect, 1); }
void ElektronDevice::deactivate_track_select() { sendCommand(ElektronCommand::ActivateTrackSelect, 0); }
void ElektronDevice::undokit_sync() { sendCommand(ElektronCommand::UndokitSync, 0); }
void ElektronDevice::reset_dsp_params() { sendCommand(ElektronCommand::ResetDspParams, 0); }
void ElektronDevice::draw_close_bank() { sendCommand(ElektronCommand::DrawCloseBank, 0); }
void ElektronDevice::draw_close_microtiming() { sendCommand(ElektronCommand::DrawCloseMicrotiming, 0); }


void ElektronDevice::set_trigleds(uint16_t bitmask, TrigLEDMode mode,
                                  uint8_t blink) {
  uint8_t data[5] = {0x70, 0x35, 0x00, 0x00, 0x00};
  // trigleds[0..6]
  data[2] = bitmask & 0x7F;
  // trigleds[7..13]
  data[3] = (bitmask >> 7) & 0x7F;
  // trigleds[14..15]
  data[4] = (bitmask >> 14) | (mode << 2) | (blink << 5);
  sendRequest(data, sizeof(data));
  // waitBlocking();
}

uint8_t ElektronDevice::waitBlocking(uint16_t timeout) {
  DEBUG_PRINTLN("wait block");
  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;
  auto listener = getSysexListener();
  listener->start();
  do {
    current_clock = read_slowclock();
    handleIncomingMidi();
  } while ((clock_diff(start_clock, current_clock) < timeout) &&
           (listener->msgType == 255));
  DEBUG_PRINTLN(listener->msgType);
  return listener->msgType;
}

void ElektronDevice::requestKit(uint8_t kit) {
  sendRequest(sysex_protocol.kitrequest_id, kit);
}

void ElektronDevice::requestPattern(uint8_t pattern) {
  sendRequest(sysex_protocol.patternrequest_id, pattern);
}

void ElektronDevice::requestSong(uint8_t song) {
  sendRequest(sysex_protocol.songrequest_id, song);
}

void ElektronDevice::requestGlobal(uint8_t global) {
  sendRequest(sysex_protocol.globalrequest_id, global);
}

uint8_t ElektronDevice::getBlockingStatus(uint8_t type, uint16_t timeout) {
  SysexCallback cb(type);

  auto listener = getSysexListener();

  listener->addOnStatusResponseCallback(
      &cb, (sysex_status_callback_ptr_t)&SysexCallback::onStatusResponse);
  sendRequest(sysex_protocol.statusrequest_id, type);
  connected = cb.waitBlocking(timeout);
  listener->removeOnStatusResponseCallback(&cb);

  return connected ? cb.value : 255;
}

bool ElektronDevice::getBlockingData(DataType type, uint8_t index, uint16_t timeout) {
    SysexCallback cb;
    uint8_t count = (type == DataType::Global) ? 1 : SYSEX_RETRIES;
    auto listener = getSysexListener();
    bool ret = false;

    while ((MidiClock.state == 2) &&
           ((MidiClock.mod12_counter > 6) || (MidiClock.mod12_counter == 0)))
        ;

    while (count--) {

        listener->addOnMessageCallback(&cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
        switch (type) {
            case DataType::Kit:
                requestKit(index);
                break;
            case DataType::Pattern:
                requestPattern(index);
                break;
            case DataType::Global:
                requestGlobal(index);
                break;
        }

        ret = cb.waitBlocking(timeout);

        listener->removeOnMessageCallback(&cb);

        if (ret) {
            midi->midiSysex.rd_cur = listener->msg_rd;
            void* data = nullptr;
            switch (type) {
                case DataType::Kit:
                    data = getKit();
                    break;
                case DataType::Pattern:
                    data = getPattern();
                    break;
                case DataType::Global:
                    data = getGlobal();
                    break;
            }
            if (data != nullptr && ((ElektronSysexObject*)data)->fromSysex(midi)) {
                if (type == DataType::Global) connected = true;
                return true;
            }
        }
    }
    return false;
}


bool ElektronDevice::getBlockingKit(uint8_t kit, uint16_t timeout) {
  return getBlockingData(DataType::Kit, kit, timeout);;
}

bool ElektronDevice::getBlockingPattern(uint8_t pattern, uint16_t timeout) {
  return getBlockingData(DataType::Pattern, pattern, timeout);;
}

bool ElektronDevice::getBlockingGlobal(uint8_t global, uint16_t timeout) {
   return getBlockingData(DataType::Global, global, timeout);
}

uint8_t ElektronDevice::getCurrentTrack(uint16_t timeout) {
  uint8_t value =
      getBlockingStatus(sysex_protocol.track_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentTrack = value;
    return value;
  }
}
uint8_t ElektronDevice::getCurrentKit(uint16_t timeout) {
  uint8_t value =
      getBlockingStatus(sysex_protocol.kit_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentKit = value;
    return value;
  }
}

uint8_t ElektronDevice::getCurrentPattern(uint16_t timeout) {
  uint8_t value =
      getBlockingStatus(sysex_protocol.pattern_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentPattern = value;
    return value;
  }
}

uint8_t ElektronDevice::getCurrentGlobal(uint16_t timeout) {
  uint8_t value =
      getBlockingStatus(sysex_protocol.global_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentGlobal = value;
    return value;
  }
}

void ElektronDevice::setStatus(uint8_t id, uint8_t value) {
  uint8_t data[] = {sysex_protocol.status_set_id, (uint8_t)(id & 0x7F),
                    (uint8_t)(value & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::setKitName(const char *name, MidiUartParent *uart_) {
  uint8_t data[64];
  uint8_t i = 0;


  data[i++] = sysex_protocol.kitname_set_id;

  memcpy(data + i, name, sysex_protocol.kitname_length);

  i += sysex_protocol.kitname_length;
  sendRequest(data, i, true, uart_);
}

uint8_t ElektronDevice::setTempo(float tempo, bool send) {
  uint16_t qtempo = round(tempo * 24.0f);
  uint8_t data[3] = {sysex_protocol.tempo_set_id, (uint8_t)(qtempo >> 7),
                     (uint8_t)(qtempo & 0x7F)};
  return sendRequest(data, countof(data), send);
}

void ElektronDevice::loadGlobal(uint8_t id) {
  uint8_t data[] = {sysex_protocol.load_global_id, (uint8_t)(id & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::loadKit(uint8_t kit) {
  uint8_t data[] = {sysex_protocol.load_kit_id, (uint8_t)(kit & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::loadPattern(uint8_t pattern) {
  uint8_t data[] = {sysex_protocol.load_pattern_id, (uint8_t)(pattern & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::saveCurrentKit(uint8_t pos) {
  uint8_t data[2] = {sysex_protocol.save_kit_id, (uint8_t)(pos & 0x7F)};
  sendRequest(data, countof(data));
}

const char *getMachineNameShort(uint8_t machine, uint8_t type,
                                const short_machine_name_t *table,
                                size_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (table[i].id == machine) {
      if (type == 1) {
        return table[i].name1;
      } else {
        return table[i].name2;
      }
    }
  }
  return nullptr;
}
