#include "A4.h"
#include "EmptyTrack.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "TurboLight.h"

uint8_t a4_sysex_hdr[5] = {0x00, 0x20, 0x3c, 0x06, 0x00};

uint8_t a4_sysex_proto_version[2] = {0x01, 0x01};

uint8_t a4_sysex_ftr[4]{0x00, 0x00, 0x00, 0x05};

const ElektronSysexProtocol a4_protocol = {
    a4_sysex_hdr,
    sizeof(a4_sysex_hdr),
    A4_KIT_REQUEST_ID,
    A4_PATTERN_REQUEST_ID,
    A4_SONG_REQUEST_ID,
    A4_GLOBAL_REQUEST_ID,

    // status request: not applicable to A4
    0,
    // get current index: not applicable/unknown to A4
    0,
    0,
    0,
    0,
    0,
    // set status: not applicable/unknown to A4
    0,
    // set tempo: not applicable/unknown to A4
    0,
    // set kit name: not applicable/unknown to A4
    0,
    0,
    // various load/save: unknown to A4
    0,
    0,
    0,
    0,
};

A4Class::A4Class()
    : ElektronDevice(&Midi2, "A4", DEVICE_A4, a4_protocol) {}

void A4Class::init_grid_devices(uint8_t device_idx) {
  uint8_t grid_idx = 1;

  GridDeviceTrack gdt;
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    uint8_t track_type = EXT_TRACK_TYPE;

    if (i < NUM_A4_SOUND_TRACKS) {
      track_type = A4_TRACK_TYPE;
    }
    gdt.init(track_type, GROUP_DEV, device_idx, &(mcl_seq.ext_tracks[i]));
    add_track_to_grid(grid_idx, i, &gdt);
  }
}

uint16_t A4Class::sendKitParams(uint8_t *masks) {
  EmptyTrack empty_track;
  for (uint8_t i = 0; i < NUM_A4_SOUND_TRACKS; i++) {
    if (masks[i] == 1) {
      auto a4_track = empty_track.load_from_mem<A4Track>(i);
      if (a4_track) {
        a4_track->sound.origPosition = i;
        a4_track->sound.soundpool = true;
        a4_track->sound.toSysex();
      }
    }
  }

  // TODO latency?
  return 0;
}

uint16_t A4Class::sendRequest(uint8_t type, uint8_t param, bool send) {

  const uint8_t len = sizeof(a4_sysex_hdr) + sizeof(a4_sysex_proto_version) +
                      sizeof(a4_sysex_ftr) + 4;
  if (send) {
    uint8_t buf[len];

    uint8_t i = 0;
    buf[i++] = 0xF0;
    for (uint8_t n = 0; n < sizeof(a4_sysex_hdr); n++) {
      buf[i++] = a4_sysex_hdr[n];
    }
    buf[i++] = type;
    for (uint8_t n = 0; n < sizeof(a4_sysex_proto_version); n++) {
      buf[i++] = a4_sysex_proto_version[n];
    }
    buf[i++] = param;
    for (uint8_t n = 0; n < sizeof(a4_sysex_ftr); n++) {
      buf[i++] = a4_sysex_ftr[n];
    }
    buf[i++] = 0xF7;
    MidiUart2.m_putc(buf, i);
  }
  return len;
}

bool A4Class::probe() {
  DEBUG_PRINT_FN();

  mcl_gui.delay_progress(300);
  if (getBlockingSettings(0)) {
    connected = true;
    DEBUG_DUMP(connected);
    turbo_light.set_speed(turbo_light.lookup_speed(mcl_cfg.uart2_turbo_speed), uart);
  }
  return connected;
}

// Caller is responsible to make sure icons_device is loaded in RM
MCLGIF* A4Class::gif() {
  return R.icons_logo->analog_gif;
}
uint8_t* A4Class::gif_data() {
  return R.icons_logo->analog_gif_data; ;
}

uint8_t* A4Class::icon() {
  return R.icons_device->icon_a4;
}


void A4Class::requestKitX(uint8_t kit) { sendRequest(A4_KITX_REQUEST_ID, kit); }

void A4Class::requestSound(uint8_t sound) {
  sendRequest(A4_SOUND_REQUEST_ID, sound);
}
void A4Class::requestSoundX(uint8_t sound) {
  sendRequest(A4_SOUNDX_REQUEST_ID, sound);
}

void A4Class::requestSongX(uint8_t song) {
  sendRequest(A4_SONGX_REQUEST_ID, song);
}

void A4Class::requestPatternX(uint8_t pattern) {
  sendRequest(A4_PATTERNX_REQUEST_ID, pattern);
}

void A4Class::requestSettings(uint8_t setting) {
  sendRequest(A4_SETTINGS_REQUEST_ID, setting);
}

void A4Class::requestSettingsX(uint8_t setting) {
  sendRequest(A4_SETTINGSX_REQUEST_ID, setting);
}

void A4Class::requestGlobalX(uint8_t global) {
  sendRequest(A4_GLOBALX_REQUEST_ID, global);
}

bool A4Class::getBlockingGeneric(uint16_t timeout) {
    SysexCallback cb;
    A4SysexListener.addOnMessageCallback(&cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
    bool connected = cb.waitBlocking(timeout);
    A4SysexListener.removeOnMessageCallback(&cb);
    return connected;
}

bool A4Class::getBlockingSound(uint8_t sound, uint16_t timeout) {
    requestSound(sound);
    return getBlockingGeneric(timeout);
}

bool A4Class::getBlockingSettings(uint8_t settings, uint16_t timeout) {
    requestSettings(settings);
    return getBlockingGeneric(timeout);
}

bool A4Class::getBlockingKitX(uint8_t kit, uint16_t timeout) {
    requestKitX(kit);
    return getBlockingGeneric(timeout);
}

bool A4Class::getBlockingPatternX(uint8_t pattern, uint16_t timeout) {
    requestPatternX(pattern);
    return getBlockingGeneric(timeout);
}

bool A4Class::getBlockingGlobalX(uint8_t global, uint16_t timeout) {
    requestGlobalX(global);
    return getBlockingGeneric(timeout);
}

bool A4Class::getBlockingSoundX(uint8_t sound, uint16_t timeout) {
    requestSoundX(sound);
    return getBlockingGeneric(timeout);
}

bool A4Class::getBlockingSettingsX(uint8_t settings, uint16_t timeout) {
    requestSettingsX(settings);
    return getBlockingGeneric(timeout);
}

void A4Class::muteTrack(uint8_t track, bool mute, MidiUartParent *uart_) {
  if (uart_ == nullptr) { uart_ = uart; }
  uart->sendCC(track, 94, mute);
}

void A4Class::setLevel(uint8_t track, uint8_t value) {
  MidiUart2.sendCC(track, 95, value);
}
