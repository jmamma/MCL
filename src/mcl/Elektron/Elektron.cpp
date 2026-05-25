#include "Elektron.h"
#include "Project.h"
#include "ResourceManager.h"

#define SYSEX_RETRIES 1

namespace {

void send_system_command(ElektronDevice *device, uint8_t command,
                         uint8_t value) {
  uint8_t data[3] = {0x70, command, value};
  device->sendRequest(data, sizeof(data));
}

void send_system_command(ElektronDevice *device, uint8_t command) {
  uint8_t data[2] = {0x70, command};
  device->sendRequest(data, sizeof(data));
}

void send_request_value(ElektronDevice *device, uint8_t request,
                        uint8_t value) {
  uint8_t data[2] = {request, (uint8_t)(value & 0x7F)};
  device->sendRequest(data, sizeof(data));
}

#if !defined(__AVR__) && defined(DEBUGMODE)
const char *blocking_data_type_name(DataType type) {
  switch (type) {
  case DataType::Kit:     return "kit";
  case DataType::Pattern: return "pattern";
  case DataType::Global:  return "global";
  }
  return "?";
}

void trace_blocking_data(const char *stage, DataType type, uint8_t index) {
  DEBUG_PRINT("[mcl-blocking] ");
  DEBUG_PRINT(stage);
  DEBUG_PRINT(" type=");
  DEBUG_PRINT(blocking_data_type_name(type));
  DEBUG_PRINT(" index=");
  DEBUG_PRINTLN((unsigned)index);
}

void trace_blocking_listener(const char *stage,
                             ElektronSysexListenerClass *listener) {
  DEBUG_PRINT("[mcl-blocking] ");
  DEBUG_PRINT(stage);
  if (!listener) {
    DEBUG_PRINTLN(" listener=null");
    return;
  }

  DEBUG_PRINT(" msgType=");
  DEBUG_PRINT((unsigned)listener->msgType);
  DEBUG_PRINT(" msg_rd=");
  DEBUG_PRINT((unsigned)listener->msg_rd);
  if (listener->sysex && listener->msg_rd < NUM_SYSEX_MSGS) {
    const uint8_t msg_rd = listener->msg_rd;
    DEBUG_PRINT(" state=");
    DEBUG_PRINT((unsigned)listener->sysex->ledger[msg_rd].state);
    DEBUG_PRINT(" len=");
    DEBUG_PRINT((unsigned)listener->sysex->ledger[msg_rd].recordLen);
  }
  DEBUG_PRINTLN();
}
#else
void trace_blocking_data(const char *, DataType, uint8_t) {}
void trace_blocking_listener(const char *, ElektronSysexListenerClass *) {}
#endif

} // namespace

ElektronSysexListenerClass::ElektronSysexListenerClass()
    : MidiSysexListenerClass(NULL, 0, 0x20, 0x3c) {
}

void ElektronHelper::beginSysexEncode(ElektronDataToSysexEncoder *encoder,
                                       uint8_t *hdr, uint8_t hdr_size,
                                       uint8_t msg_id, uint8_t version,
                                       uint8_t origPosition) {
  encoder->stop7Bit();
  encoder->begin();
  encoder->pack(hdr, hdr_size);
  encoder->pack8(msg_id);
  encoder->pack8(version);
  encoder->pack8(0x01); // revision
  encoder->startChecksum();
  encoder->pack8(origPosition);
}

uint16_t ElektronHelper::finishSysexEncode(ElektronDataToSysexEncoder *encoder) {
  uint16_t enclen = encoder->finish();
  encoder->finishChecksum();
  return enclen + 5;
}

uint16_t ElektronDevice::sendRequest(uint8_t *data, uint8_t len, bool send, MidiUartClass *uart_) {
    if (!send) {
        return len + sysex_protocol.header_size + 2;
    }

    if (!connected && !in_probe) { return 0; }

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

uint16_t ElektronDevice::sendRequest(uint8_t type, uint8_t param, bool send) {
  uint8_t data[] = {type, param};
  return sendRequest(data, 2, send);
}

bool ElektronDevice::get_tempo(uint16_t &tempo) {

  send_system_command(this, 0x3F);

  uint8_t msgType = waitBlocking();

  auto begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();
  SysexView sysex(listener->sysex, listener->msg_rd);

  tempo = 0;
  if (msgType == 0x72 && sysex.getByte(begin) == 0x3F) {
      tempo = sysex.getByte(begin+1) << 7;
      tempo |= (sysex.getByte(begin+2));
      return true;
  }

  return false;
}


bool ElektronDevice::get_mute_state(uint16_t &mute_state) {

  send_system_command(this, 0x33);

  uint8_t msgType = waitBlocking();

  auto begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();
  SysexView sysex(listener->sysex, listener->msg_rd);
  mute_state = 0;
  if (msgType == 0x72 && sysex.getByte(begin) == 0x33) {
      mute_state = sysex.getByte(begin+1);
      mute_state |= (sysex.getByte(begin+2) << 7);
      mute_state |= (sysex.getByte(begin+3) << 14);
      return true;
  }

  return false;
}


bool ElektronDevice::get_fw_caps() {

  send_system_command(this, 0x30);

  uint8_t msgType = waitBlocking();

  fw_caps = 0;

  uint8_t begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();
  DEBUG_PRINTLN("caps");
  if (!listener || !listener->sysex || listener->msg_rd >= NUM_SYSEX_MSGS) {
    return false;
  }

  const uint8_t msg_rd = listener->msg_rd;
  if (!listener->sysex->ledger[msg_rd].ptr) {
    return false;
  }
  const uint16_t record_len = listener->sysex->ledger[msg_rd].recordLen;
  if (listener->sysex->ledger[msg_rd].state != SYSEX_STATE_FIN ||
      record_len <= begin) {
    return false;
  }

  SysexView sysex(listener->sysex, msg_rd);
  uint8_t b = 0;
  if (msgType == 0x72 && sysex.getByte(begin) == 0x30) {
    if (record_len < begin + 5) {
      return false;
    }
    begin++;
    uint8_t *caps = (uint8_t *)&fw_caps;
    for (uint8_t n = 0; n < 4; n++) {
      b = sysex.getByte(begin++);
      if (b == 0xF7) { break; }
      caps[n] = b;
    }
    return true;
  }
  return false;
}

void ElektronDevice::activate_encoder_interface(uint8_t *params, uint8_t count) {
  static constexpr uint8_t kLegacyParamCount = 24;
  static constexpr uint8_t kMaxParamCount = 34;
  if (params == nullptr) {
    return;
  }
  if (count < kLegacyParamCount) {
    count = kLegacyParamCount;
  }
  if (count > kMaxParamCount) {
    count = kMaxParamCount;
  }

  encoder_interface = true;
  const uint8_t mask_count = (uint8_t)((count + 6) / 7);
  uint8_t data[3 + 5 + kMaxParamCount] = {0x70, 0x36, 0x01};
  uint8_t mod7 = 0;
  uint8_t cnt = 0;

  for (uint8_t n = 0; n < count; n++) {
    if (params[n] != 255) {
      data[3 + cnt] |= (1 << mod7);
      data[3 + mask_count + n] = params[n];
    }
    mod7++;
    if (mod7 == 7) {
      mod7 = 0;
      cnt++;
    }
  }
  sendRequest(data, (uint8_t)(3 + mask_count + count));
  //waitBlocking();
}

void ElektronDevice::deactivate_encoder_interface() {
  encoder_interface = false;
  send_system_command(this, 0x36, 0);
}

void ElektronDevice::sendCommand(ElektronCommand command, uint8_t param) {
  uint8_t data[3] = {0x70, 0x00, 0x00};
  bool needsWait = false;
  uint8_t l = 3;
  switch (command) {
    case ElektronCommand::ActivateEncoderInterface:
      data[1] = 0x36;
      data[2] = param;
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
    case ElektronCommand::ActivateKeyInterface:
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

void ElektronDevice::draw_microtiming_signed(uint8_t speed,
                                             int8_t microtiming) {
  int16_t encoded = (int16_t)microtiming + 127;
  if (encoded < 0) {
    encoded = 0;
  } else if (encoded > 254) {
    encoded = 254;
  }
  uint8_t a = (uint8_t)encoded >> 7;
  uint8_t b = (uint8_t)encoded & 0x7F;
  uint8_t data[6] = {0x70, 0x3C, 0x27, speed, a, b};
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
  uint8_t data[67];
  data[0] = 0x70;
  data[1] = 0x3B;
  data[2] = persistent;
  uint8_t len = strlen(str);
  strcpy((char*) (data + 3), str);
  sendRequest(data, 3 + len + 1);
  // waitBlocking();
}

void ElektronDevice::popup_text_P(const char *str_P, uint8_t persistent) {
  uint8_t data[67];
  data[0] = 0x70;
  data[1] = 0x3B;
  data[2] = persistent;
  uint8_t len = strlen_P(str_P);
  strcpy_P((char*) (data + 3), str_P);
  sendRequest(data, 3 + len + 1);
  // waitBlocking();
}

void ElektronDevice::popup_text_P(const char *str1_P, const char *str2_P, uint8_t persistent) {
  uint8_t data[67];
  data[0] = 0x70;
  data[1] = 0x3B;
  data[2] = persistent;
  uint8_t len1 = strlen_P(str1_P);
  uint8_t len2 = strlen_P(str2_P);
  strcpy_P((char*) (data + 3), str1_P);
  data[3 + len1] = ' ';
  strcpy_P((char*) (data + 3 + len1 + 1), str2_P);
  sendRequest(data, 3 + len1 + 1 + len2 + 1);
  // waitBlocking();
}

void ElektronDevice::draw_bank(uint8_t bank) {
  uint8_t data[5] = {0x70, 0x3C, 0x22, bank};
  sendRequest(data, 5);
  // waitBlocking();
}

void ElektronDevice::activate_enhanced_midi() { send_system_command(this, 0x3E, 1); }
void ElektronDevice::deactivate_enhanced_midi() { send_system_command(this, 0x3E, 0); }
void ElektronDevice::activate_enhanced_gui() { send_system_command(this, 0x37, 1); }
void ElektronDevice::deactivate_enhanced_gui() { send_system_command(this, 0x37, 0); }
void ElektronDevice::set_seq_page(uint8_t page) { send_system_command(this, 0x38, page); }
void ElektronDevice::set_rec_mode(uint8_t mode) { send_system_command(this, 0x3A, mode); }
void ElektronDevice::set_key_repeat(uint8_t mode) { send_system_command(this, 0x4E, mode); }
void ElektronDevice::activate_key_interface() { send_system_command(this, 0x31, 1); }
void ElektronDevice::deactivate_key_interface() { send_system_command(this, 0x31, 0); }
void ElektronDevice::activate_track_select() {
  send_system_command(this, 0x32, 1);
  waitBlocking();
}
void ElektronDevice::deactivate_track_select() {
  send_system_command(this, 0x32, 0);
  waitBlocking();
}
void ElektronDevice::undokit_sync() { send_system_command(this, 0x42); }
void ElektronDevice::reset_dsp_params() { send_system_command(this, 0x43); }
void ElektronDevice::draw_close_bank() { send_system_command(this, 0x3C, 0x23); }
void ElektronDevice::draw_close_microtiming() { send_system_command(this, 0x3C, 0x21); }
void ElektronDevice::draw_open_swing() { send_system_command(this, 0x3C, 0x25); }
void ElektronDevice::draw_close_swing() { send_system_command(this, 0x3C, 0x26); }


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
    platform_wait_poll();
    current_clock = read_slowclock();
    handleIncomingMidi();
  } while ((clock_diff(start_clock, current_clock) < timeout) &&
           (listener->msgType == 255));
#if !defined(__AVR__)
  // The hosted wasm/desktop path uses a virtual cable with a host-side MIDI
  // worker. A reply can be queued just as the timeout expires; drain once more
  // before reporting failure so the next UI loop does not consume a reply that
  // belonged to this blocking request.
  if (listener->msgType == 255) {
    platform_wait_poll();
    handleIncomingMidi();
  }
#endif
  DEBUG_PRINTLN(listener->msgType);
  return listener->msgType;
}

void ElektronDevice::requestKit(uint8_t kit) {
#if defined(__AVR__)
  sendRequest(sysex_protocol.kitrequest_id, kit);
#else
  uint8_t data[] = {sysex_protocol.kitrequest_id, kit, SYSEX_VERSION_LEGACY};
  sendRequest(data, sizeof(data));
#endif
}

void ElektronDevice::requestPattern(uint8_t pattern) {
#if defined(__AVR__)
  sendRequest(sysex_protocol.patternrequest_id, pattern);
#else
  uint8_t data[] = {sysex_protocol.patternrequest_id, pattern, SYSEX_VERSION_LEGACY};
  sendRequest(data, sizeof(data));
#endif
}

void ElektronDevice::requestSong(uint8_t song) {
#if defined(__AVR__)
  sendRequest(sysex_protocol.songrequest_id, song);
#else
  uint8_t data[] = {sysex_protocol.songrequest_id, song, SYSEX_VERSION_LEGACY};
  sendRequest(data, sizeof(data));
#endif
}

void ElektronDevice::requestGlobal(uint8_t global) {
#if defined(__AVR__)
  sendRequest(sysex_protocol.globalrequest_id, global);
#else
  uint8_t data[] = {sysex_protocol.globalrequest_id, global, SYSEX_VERSION_LEGACY};
  sendRequest(data, sizeof(data));
#endif
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
           ((MidiClock.mod12_counter > 6) || (MidiClock.mod12_counter == 0))) {
        platform_poll();
    }

    while (count--) {

        listener->addOnMessageCallback(&cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
        trace_blocking_data("request", type, index);
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
        trace_blocking_listener(ret ? "received" : "timeout", listener);

        listener->removeOnMessageCallback(&cb);

        if (ret) {
            midi->midiSysex->rd_cur = listener->msg_rd;
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
                trace_blocking_data("parse-ok", type, index);
                return true;
            }
            trace_blocking_data("parse-failed", type, index);
        }
    }
    trace_blocking_data("failed", type, index);
    return false;
}


bool ElektronDevice::getBlockingKit(uint8_t kit, uint16_t timeout) {
  return getBlockingData(DataType::Kit, kit, timeout);
}

bool ElektronDevice::getBlockingPattern(uint8_t pattern, uint16_t timeout) {
  return getBlockingData(DataType::Pattern, pattern, timeout);
}

bool ElektronDevice::getBlockingGlobal(uint8_t global, uint16_t timeout) {
   return getBlockingData(DataType::Global, global, timeout);
}

uint8_t ElektronDevice::getCurrentTrack(uint16_t timeout) {
  uint8_t value =
      getBlockingStatus(sysex_protocol.track_index_request_id, timeout);
  if (value == 255) {
    return 255;
  }
  // Stray byte from a noisy reply has been observed to land here as
  // 24 (= 16 + 8) and propagate through MD.currentTrack into kit /
  // pattern indexing — guard at the source.
  if (value >= 16) {
    return 255;
  }
  currentTrack = value;
  return value;
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

void ElektronDevice::setKitName(const char *name, MidiUartClass *uart_) {
  uint8_t data[64];
  uint8_t i = 0;


  data[i++] = sysex_protocol.kitname_set_id;

  memcpy(data + i, name, sysex_protocol.kitname_length);

  i += sysex_protocol.kitname_length;
  sendRequest(data, i, true, uart_);
}

uint8_t ElektronDevice::setTempo(float tempo, bool send) {
  uint16_t qtempo = (uint16_t)(tempo * 24.0f + 0.5f);
  uint8_t data[3] = {sysex_protocol.tempo_set_id, (uint8_t)(qtempo >> 7),
                     (uint8_t)(qtempo & 0x7F)};
  return sendRequest(data, countof(data), send);
}

void ElektronDevice::loadGlobal(uint8_t id) {
  send_request_value(this, sysex_protocol.load_global_id, id);
}

void ElektronDevice::loadKit(uint8_t kit) {
  send_request_value(this, sysex_protocol.load_kit_id, kit);
}

void ElektronDevice::loadPattern(uint8_t pattern) {
  send_request_value(this, sysex_protocol.load_pattern_id, pattern);
}

void ElektronDevice::saveCurrentKit(uint8_t pos) {
  send_request_value(this, sysex_protocol.save_kit_id, pos);
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
