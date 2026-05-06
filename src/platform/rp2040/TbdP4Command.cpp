#include "TbdP4Command.h"

#ifdef PLATFORM_TBD

#include <Arduino.h>
#include <SPI.h>
#include <string.h>

namespace {

constexpr uint32_t kSpiSpeed = 30000000;
constexpr uint8_t kSpiSclk = 34;
constexpr uint8_t kSpiMosi = 35;
constexpr uint8_t kSpiMiso = 32;
constexpr uint8_t kSpiCs = 33;
constexpr uint8_t kSpiReady = 18;

constexpr size_t kFrameSize = 2048;
constexpr size_t kResponseStringOffset = 7;
constexpr size_t kStringOffset = 9;
constexpr size_t kMaxStringParamLen = kFrameSize - kStringOffset - 1;

constexpr uint8_t kRequestSetActiveSampleKit = 0x18;
constexpr uint8_t kRequestGetMacroSoundPreset = 0xA1;
constexpr uint8_t kRequestGetMacroDefinition = 0xA2;
constexpr uint8_t kRequestActivateTrackMachine = 0xA3;
constexpr uint8_t kRequestLoadTrackSoundPreset = 0xA4;
constexpr uint8_t kRequestGetTrackDefaultPresets = 0xA5;
constexpr uint8_t kRequestGetKitIndexJSON = 0xA7;
constexpr uint8_t kRequestLoadTrackMacroDefinition = 0xAA;
constexpr uint8_t kRequestAnnounceApp = 0xAB;
constexpr uint8_t kRequestSetTrackMute = 0xBD;

SPISettings spi_settings(kSpiSpeed, MSBFIRST, SPI_MODE3);

uint8_t out_buf[kFrameSize];
uint8_t in_buf[kFrameSize];

} // namespace

TbdP4CommandTransport tbd_p4_command;

void TbdP4CommandTransport::init() {
  if (initialized_) {
    return;
  }

  SPI.setMISO(kSpiMiso);
  SPI.setMOSI(kSpiMosi);
  SPI.setCS(kSpiCs);
  SPI.setSCK(kSpiSclk);
  pinMode(kSpiReady, INPUT);

  memset(out_buf, 0, sizeof(out_buf));
  memset(in_buf, 0, sizeof(in_buf));
  out_buf[0] = 0xCA;
  out_buf[1] = 0xFE;

  initialized_ = true;
  delay(100);
}

bool TbdP4CommandTransport::ready() const {
  return digitalRead(kSpiReady) != LOW;
}

bool TbdP4CommandTransport::wait_ready(uint32_t timeout_ms) const {
  const uint32_t start_ms = millis();
  while (!ready()) {
    if (timeout_ms != 0 && (millis() - start_ms) >= timeout_ms) {
      return false;
    }
    delay(1);
  }
  return true;
}

void TbdP4CommandTransport::reset_frame(uint8_t request) {
  memset(out_buf, 0, sizeof(out_buf));
  out_buf[0] = 0xCA;
  out_buf[1] = 0xFE;
  out_buf[2] = request;
}

bool TbdP4CommandTransport::transfer_frame(uint32_t timeout_ms) {
  init();

  if (!wait_ready(timeout_ms)) {
    ready_timeouts_++;
    return false;
  }

  SPI.begin(true);
  SPI.beginTransaction(spi_settings);
  SPI.transfer(out_buf, in_buf, sizeof(out_buf));
  SPI.endTransaction();
  SPI.end();

  tx_frames_++;
  return true;
}

bool TbdP4CommandTransport::send_frame(uint8_t request, uint32_t timeout_ms) {
  if (!transfer_frame(timeout_ms)) {
    return false;
  }

  last_request_ = request;

  if (!wait_ready(timeout_ms)) {
    ready_timeouts_++;
    return false;
  }

  delay(100);
  return true;
}

bool TbdP4CommandTransport::receive_response(uint8_t request, char *response,
                                             size_t response_len,
                                             uint32_t timeout_ms) {
  if (response == nullptr || response_len == 0) {
    return false;
  }
  response[0] = '\0';

  uint32_t total_len = 0;
  uint32_t remaining = 0;
  size_t copied = 0;

  do {
    if (!transfer_frame(timeout_ms)) {
      return false;
    }
    last_request_ = request;

    if (in_buf[0] != 0xCA || in_buf[1] != 0xFE || in_buf[2] != request) {
      return false;
    }

    if (total_len == 0) {
      memcpy(&total_len, in_buf + 3, sizeof(total_len));
      remaining = total_len;
    }

    const uint32_t frame_payload =
        remaining > (kFrameSize - kResponseStringOffset)
            ? (kFrameSize - kResponseStringOffset)
            : remaining;
    const size_t copy_room =
        copied < response_len ? response_len - copied - 1 : 0;
    const size_t copy_len =
        frame_payload < copy_room ? frame_payload : copy_room;
    if (copy_len > 0) {
      memcpy(response + copied, in_buf + kResponseStringOffset, copy_len);
      copied += copy_len;
    }
    remaining -= frame_payload;
  } while (remaining > 0);

  response[copied] = '\0';
  return total_len < response_len;
}

bool TbdP4CommandTransport::announce_app(const char *app_name, uint8_t flags,
                                         uint32_t timeout_ms) {
  if (app_name == nullptr) {
    return false;
  }

  const size_t len = strlen(app_name);
  if (len + 1 > kMaxStringParamLen) {
    return false;
  }

  reset_frame(kRequestAnnounceApp);
  out_buf[3] = flags;
  memcpy(out_buf + kStringOffset, app_name, len + 1);
  return send_frame(kRequestAnnounceApp, timeout_ms);
}

bool TbdP4CommandTransport::get_track_default_presets(
    char *response, size_t response_len, const char *template_name,
    uint32_t timeout_ms) {
  if (response == nullptr || response_len == 0) {
    return false;
  }

  reset_frame(kRequestGetTrackDefaultPresets);
  if (template_name != nullptr && template_name[0] != '\0') {
    const size_t len = strlen(template_name);
    if (len + 1 > kMaxStringParamLen) {
      return false;
    }
    memcpy(out_buf + kStringOffset, template_name, len + 1);
  } else {
    out_buf[kStringOffset] = '\0';
  }

  if (!send_frame(kRequestGetTrackDefaultPresets, timeout_ms)) {
    return false;
  }
  return receive_response(kRequestGetTrackDefaultPresets, response,
                          response_len, timeout_ms);
}

bool TbdP4CommandTransport::get_macro_sound_preset(
    const char *preset_id, char *response, size_t response_len,
    uint32_t timeout_ms) {
  if (preset_id == nullptr || preset_id[0] == '\0') {
    return false;
  }

  const size_t len = strlen(preset_id);
  if (len + 1 > kMaxStringParamLen) {
    return false;
  }

  reset_frame(kRequestGetMacroSoundPreset);
  memcpy(out_buf + kStringOffset, preset_id, len + 1);
  if (!send_frame(kRequestGetMacroSoundPreset, timeout_ms)) {
    return false;
  }
  return receive_response(kRequestGetMacroSoundPreset, response, response_len,
                          timeout_ms);
}

bool TbdP4CommandTransport::get_macro_definition(
    const char *macro_id, char *response, size_t response_len,
    uint32_t timeout_ms) {
  if (macro_id == nullptr || macro_id[0] == '\0') {
    return false;
  }

  const size_t len = strlen(macro_id);
  if (len + 1 > kMaxStringParamLen) {
    return false;
  }

  reset_frame(kRequestGetMacroDefinition);
  memcpy(out_buf + kStringOffset, macro_id, len + 1);
  if (!send_frame(kRequestGetMacroDefinition, timeout_ms)) {
    return false;
  }
  return receive_response(kRequestGetMacroDefinition, response, response_len,
                          timeout_ms);
}

bool TbdP4CommandTransport::get_kit_index_json(char *response,
                                               size_t response_len,
                                               uint32_t timeout_ms) {
  reset_frame(kRequestGetKitIndexJSON);
  if (!send_frame(kRequestGetKitIndexJSON, timeout_ms)) {
    return false;
  }
  return receive_response(kRequestGetKitIndexJSON, response, response_len,
                          timeout_ms);
}

bool TbdP4CommandTransport::set_active_sample_kit(uint8_t kit_index,
                                                  uint32_t timeout_ms) {
  reset_frame(kRequestSetActiveSampleKit);
  out_buf[3] = kit_index;
  return send_frame(kRequestSetActiveSampleKit, timeout_ms);
}

bool TbdP4CommandTransport::activate_track_machine(uint8_t track_index,
                                                   const char *machine_id,
                                                   uint32_t timeout_ms) {
  if (machine_id == nullptr || machine_id[0] == '\0') {
    return false;
  }

  const size_t len = strlen(machine_id);
  if (len + 1 > kMaxStringParamLen) {
    return false;
  }

  reset_frame(kRequestActivateTrackMachine);
  out_buf[3] = track_index;
  memcpy(out_buf + kStringOffset, machine_id, len + 1);
  return send_frame(kRequestActivateTrackMachine, timeout_ms);
}

bool TbdP4CommandTransport::load_track_sound_preset(uint8_t track_index,
                                                    const char *preset_id,
                                                    uint8_t rom_bank,
                                                    int32_t sample_slice,
                                                    uint32_t timeout_ms) {
  if (preset_id == nullptr || preset_id[0] == '\0') {
    return false;
  }

  const size_t len = strlen(preset_id);
  if (len + 1 > kMaxStringParamLen) {
    return false;
  }

  reset_frame(kRequestLoadTrackSoundPreset);
  out_buf[3] = track_index;
  out_buf[4] = rom_bank;
  memcpy(out_buf + 5, &sample_slice, sizeof(sample_slice));
  memcpy(out_buf + kStringOffset, preset_id, len + 1);
  return send_frame(kRequestLoadTrackSoundPreset, timeout_ms);
}

bool TbdP4CommandTransport::load_track_macro_definition(
    uint8_t track_index, const char *macro_id, uint32_t timeout_ms) {
  if (macro_id == nullptr || macro_id[0] == '\0') {
    return false;
  }

  const size_t len = strlen(macro_id);
  if (len + 1 > kMaxStringParamLen) {
    return false;
  }

  reset_frame(kRequestLoadTrackMacroDefinition);
  out_buf[3] = track_index;
  memcpy(out_buf + kStringOffset, macro_id, len + 1);
  return send_frame(kRequestLoadTrackMacroDefinition, timeout_ms);
}

bool TbdP4CommandTransport::set_track_mute(uint8_t track_index, bool muted,
                                           uint32_t timeout_ms) {
  if (track_index >= 16) {
    return false;
  }

  reset_frame(kRequestSetTrackMute);
  out_buf[3] = track_index;
  out_buf[4] = muted ? 1 : 0;
  return send_frame(kRequestSetTrackMute, timeout_ms);
}

void TbdP4CommandTransport::get_stats(TbdP4CommandStats &stats) const {
  stats.tx_frames = tx_frames_;
  stats.ready_timeouts = ready_timeouts_;
  stats.last_request = last_request_;
  stats.initialized = initialized_;
  stats.ready_pin = initialized_ ? ready() : false;
}

#endif // PLATFORM_TBD
