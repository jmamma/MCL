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
constexpr size_t kStringOffset = 9;
constexpr size_t kMaxStringParamLen = kFrameSize - kStringOffset - 1;

constexpr uint8_t kRequestLoadTrackSoundPreset = 0xA4;
constexpr uint8_t kRequestAnnounceApp = 0xAB;

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

bool TbdP4CommandTransport::send_frame(uint8_t request, uint32_t timeout_ms) {
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
  last_request_ = request;

  if (!wait_ready(timeout_ms)) {
    ready_timeouts_++;
    return false;
  }

  delay(100);
  return true;
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

void TbdP4CommandTransport::get_stats(TbdP4CommandStats &stats) const {
  stats.tx_frames = tx_frames_;
  stats.ready_timeouts = ready_timeouts_;
  stats.last_request = last_request_;
  stats.initialized = initialized_;
  stats.ready_pin = initialized_ ? ready() : false;
}

#endif // PLATFORM_TBD
