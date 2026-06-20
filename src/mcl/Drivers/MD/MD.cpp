#include "MD.h"
#include "GridTrack.h"
#include "ResourceManager.h"
#include "Sequencer/MCLSeq.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "Devices/MidiSetup.h"
#include "Sequencer/SeqTrackUtil.h"
#include "Sequencer/SeqStepTrackRef.h"
#include "Devices/TurboLight.h"
#include "MCLGUI.h"
#include "PerfData.h"
#include "UI/MDTrackSelect.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "MCLStrings.h"
#include "KeyInterface.h"
#include <string.h>

#if !defined(__AVR__)
namespace {

void use_spsx_longest_track_sync(uint8_t &length, uint8_t &speed,
                                 uint8_t &step_count) {
  if (!mcl_seq.using_spsx_tracks) {
    return;
  }

  uint8_t best_length = 0;
  uint8_t best_speed = speed;
  uint8_t best_step = step_count;
  for (uint8_t i = 0; i < mcl_seq.num_md_tracks; i++) {
    SPSXSeqTrack &track = mcl_seq.spsx_tracks[i];
    if (track.length == 0 || track.length <= best_length) {
      continue;
    }
    best_length = track.length;
    best_speed = track.speed;
    best_step = track.step_count;
  }

  if (best_length > 0) {
    length = best_length;
    speed = best_speed;
    step_count = best_step;
  }
}

} // namespace
#endif

void MDMidiEvents::track_cc(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  if (track == 255) {
    return;
  }

  uint8_t param_limit = MD.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  if (track_param == MODEL_LEVEL) {
    MD.kit.levels[track] = value;
  } else if (track_param == MODEL_MUTE) {
    SeqTrackUtil::set_mute_state(true, track, value > 0);
  } else if (track_param < param_limit) {
    MD.kit.params[track][track_param] = value;
    last_md_param = track_param;
  }
}

void MDMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  track_cc(msg);
}

void MDMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {}

void MDMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}

void MDMidiEvents::enable_live_kit_update() {
  if (kitupdate_state) {
    return;
  }
  MD.midi->addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MDMidiEvents::onControlChangeCallback_Midi);
  kitupdate_state = true;
}

void MDMidiEvents::disable_live_kit_update() {

  if (!kitupdate_state) {
    return;
  }
  MD.midi->removeOnControlChangeCallback(
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

#ifdef PLATFORM_TBD
MDClass::MDClass()
    : ElektronDevice(&Midi, "MD", DEVICE_MD, md_protocol, "MACHINEDRUM"),
      ui(*this) {}
#else
MDClass::MDClass()
    : ElektronDevice(&Midi, "MD", DEVICE_MD, md_protocol, "MACHINEDRUM") {}
#endif

namespace {

#if !defined(__AVR__)
class MDGlobalLightSysex : public ElektronSysexObject {
public:
  uint8_t getPosition() override { return global_.getPosition(); }
  void setPosition(uint8_t pos) override { global_.setPosition(pos); }

  bool fromSysex(MidiClass *midi) override {
    if (!global_.fromSysex(midi)) {
      return false;
    }
    copy_to_light();
    return true;
  }

  uint16_t toSysex(ElektronDataToSysexEncoder *encoder) override {
    copy_from_light();
    return global_.toSysex(encoder);
  }

private:
  void copy_to_light() {
    memcpy(MD.global.drumRouting, global_.drumRouting,
           sizeof(MD.global.drumRouting));
    memcpy(MD.global.drumMapping, global_.drumMapping,
           sizeof(MD.global.drumMapping));
    MD.global.baseChannel = global_.baseChannel;
    MD.global.tempo = global_.tempo;
    MD.global.extendedMode = global_.extendedMode;
    MD.global.clockIn = global_.clockIn;
    MD.global.clockOut = global_.clockOut;
    MD.global.transportIn = global_.transportIn;
    MD.global.transportOut = global_.transportOut;
    MD.global.localOn = global_.localOn;
    MD.global.programChange = global_.programChange;
    MD.global.trigMode = global_.trigMode;
    MD.global.channelMode = global_.channelMode;
  }

  void copy_from_light() {
    memcpy(global_.drumRouting, MD.global.drumRouting,
           sizeof(global_.drumRouting));
    global_.baseChannel = MD.global.baseChannel;
    global_.tempo = MD.global.tempo;
    global_.extendedMode = MD.global.extendedMode;
    global_.clockIn = MD.global.clockIn;
    global_.clockOut = MD.global.clockOut;
    global_.transportIn = MD.global.transportIn;
    global_.transportOut = MD.global.transportOut;
    global_.localOn = MD.global.localOn;
    global_.programChange = MD.global.programChange;
    global_.trigMode = MD.global.trigMode;
    global_.channelMode = MD.global.channelMode;
  }

  MDGlobal global_;
};

ElektronSysexObject *md_global_sysex() {
  static MDGlobalLightSysex global_sysex;
  return &global_sysex;
}

uint8_t md_target_fx_type(uint8_t target) {
  return MD_FX_ECHO + target - NUM_MD_TRACKS;
}
#endif

} // namespace

void MDClass::setup_listeners() {
  MDSysexListener.setup(midi);
  key_interface.setup(midi);
  md_track_select.setup(midi);
  // MDSysexListener.setup() calls addSysexListener internally,
  // but key_interface/md_track_select.setup() only set the sysex pointer.
  // Re-register them here so cleanup_listeners() + setup_listeners() is symmetric.
  if (midi && midi->midiSysex) {
    midi->midiSysex->addSysexListener(&key_interface);
    midi->midiSysex->addSysexListener(&md_track_select);
  }
  midi_events.enable_live_kit_update();
}

void MDClass::cleanup_listeners() {
  midi_events.disable_live_kit_update();
  // Remove sysex listeners from the current (old) sysex object
  // before setPort() reassigns midi to the new port.
  if (midi && midi->midiSysex) {
    midi->midiSysex->removeSysexListener(&MDSysexListener);
    midi->midiSysex->removeSysexListener(&key_interface);
    midi->midiSysex->removeSysexListener(&md_track_select);
  }
}

void MDClass::on_forwarded_cc(uint8_t *msg) {
  if (!midi_events.kitupdate_state) {
    return;
  }
  midi_events.track_cc(msg);
}

bool MDClass::config_menu_entry(DeviceIdx device_idx,
                                DriverConfigMenuEntry *entry) const {
  (void)device_idx;
  if (entry == nullptr) {
    return false;
  }
  entry->name = full_name;
  entry->page = MD_CONFIG_PAGE;
  return true;
}

#ifdef PLATFORM_TBD
bool MDClass::supports_capability(MidiDeviceCapability capability) const {
  switch (capability) {
  case MidiDeviceCapability::MdTrigInterface:
  case MidiDeviceCapability::MdSequencerTracks:
  case MidiDeviceCapability::MdPatternImport:
    return true;
  }
  return ElektronDevice::supports_capability(capability);
}
#endif

namespace {

class MDMixerCapability final : public DeviceMixerCapability {
public:
  explicit MDMixerCapability(MDClass &device)
      : DeviceMixerCapability(device, MODEL_LEVEL, MODEL_MUTE) {}
  virtual uint8_t track_count(const DeviceContext &ctx) const override;
  virtual SeqTrack *seq_track(const DeviceContext &ctx,
                              uint8_t track) override;
  virtual bool param(const DeviceContext &ctx, uint8_t track,
                     uint8_t param_idx,
                     MidiDeviceMixerParam *param) override;
  virtual bool set_param(const DeviceContext &ctx, uint8_t track,
                         uint8_t param_idx, MidiDeviceMixerValue value,
                         bool send = true) override;
#if !defined(__AVR__)
  virtual void fill_track(const DeviceContext &ctx, uint8_t track, bool fill,
                          MidiUartClass *uart_ = nullptr) override;
#endif
  virtual void set_record_mutes(const DeviceContext &ctx, uint8_t track,
                                bool state, bool clear = false) override;
  virtual uint8_t trig_group(const DeviceContext &ctx,
                             uint8_t track) const override;
  virtual void select_track(const DeviceContext &ctx, uint8_t track) override;
  virtual void restore_track_params(const DeviceContext &ctx,
                                    uint8_t track) override;
  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const override;

private:
  MDClass &md() const { return (MDClass &)device_; }
};

static MDMixerCapability md_mixer_capability(MD);

#if !defined(__AVR__)
class MDParamCapability : public DeviceParamCapability {
public:
  explicit MDParamCapability(MDClass &device) : DeviceParamCapability(device) {}
  virtual uint8_t target_count(const DeviceContext &ctx) const override;
  virtual uint8_t param_count(const DeviceContext &ctx,
                              uint8_t target) const override;
  virtual bool target_label(const DeviceContext &ctx, uint8_t target,
                            char *out, uint8_t len) const override;
  virtual bool param_label(const DeviceContext &ctx, uint8_t target,
                           uint8_t param, char *out, uint8_t len) override;
  virtual bool get_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t *value) override;
  virtual bool set_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t value,
                         MidiUartClass *uart_ = nullptr,
                         bool update_kit = false) override;
  virtual bool sequencer_lock_param_label(const DeviceContext &ctx,
                                          uint8_t target, uint8_t param,
                                          char *out, uint8_t len) override;
  virtual bool sequencer_uses_step_pitch(const DeviceContext &ctx,
                                         uint8_t target) const override;

private:
  MDClass &md() const { return (MDClass &)device_; }
};

class MDPerfCapability : public DevicePerfCapability {
public:
  explicit MDPerfCapability(MDClass &device) : DevicePerfCapability(device) {}
  virtual bool perf_scene_autofill(const DeviceContext &ctx,
                                   uint8_t dest_offset, PerfData *data,
                                   uint8_t scene) override;

private:
  MDClass &md() const { return (MDClass &)device_; }
};
#endif

#if !defined(__AVR__)
class MDStepTrackCapability : public DeviceStepTrackCapability {
public:
  explicit MDStepTrackCapability(MDClass &device)
      : DeviceStepTrackCapability(device) {}
  virtual bool available(const DeviceContext &ctx) const override;
  virtual uint8_t track_count(const DeviceContext &ctx) const override;
  virtual SeqStepTrackRef track(const DeviceContext &ctx,
                                uint8_t track) const override;
  virtual SeqStepTrackRef active_track(const DeviceContext &ctx) const override;
  virtual bool parses_kit_cc(const DeviceContext &ctx) const override;
  virtual bool parse_kit_cc(const DeviceContext &ctx, uint8_t channel,
                            uint8_t cc, uint8_t *track,
                            uint8_t *param) const override;

private:
  MDClass &md() const { return (MDClass &)device_; }
};

class MDStepEditCapability : public DeviceStepEditCapability {
public:
  explicit MDStepEditCapability(MDClass &device)
      : DeviceStepEditCapability(device) {}
  virtual bool available(const DeviceContext &ctx) const override;
  virtual void set_rec_mode(const DeviceContext &ctx, uint8_t mode) override;
  virtual void sync_track(const DeviceContext &ctx, uint8_t length,
                          uint8_t speed, uint8_t step_count,
                          uint8_t swing_amount = 0x7F) override;
  virtual void set_trig_leds(const DeviceContext &ctx, uint16_t mask,
                             uint8_t mode, uint8_t blink = 0) override;
  virtual void set_live_param_update(const DeviceContext &ctx,
                                     bool enabled) override;
  virtual bool configure_kit_sound_panel(const DeviceContext &ctx,
                                         uint8_t target, char *info,
                                         uint8_t info_len, uint8_t *pitch_max,
                                         bool *is_midi_model) const override;
  virtual bool kit_sound_uses_note_pitch(const DeviceContext &ctx,
                                         uint8_t target) const override;
  virtual bool kit_sound_voice_allocatable(const DeviceContext &ctx,
                                           uint8_t target) const override;
  virtual uint8_t kit_sound_default_pitch(const DeviceContext &ctx,
                                          uint8_t target) const override;
  virtual uint8_t kit_sound_note_from_pitch(const DeviceContext &ctx,
                                            uint8_t target,
                                            uint8_t pitch) const override;
  virtual uint8_t kit_sound_pitch_from_note(const DeviceContext &ctx,
                                            uint8_t target, uint8_t note,
                                            uint8_t fine_tune) const override;
  virtual bool param_from_key(const DeviceContext &ctx, uint8_t target,
                              uint8_t key, uint8_t *param) const override;
  virtual bool key_for_param(const DeviceContext &ctx, uint8_t target,
                             uint8_t param, uint8_t *key) const override;
  virtual bool begin_param_editor(const DeviceContext &ctx, uint8_t target,
                                  uint8_t *params, uint8_t count) override;
  virtual void end_param_editor(const DeviceContext &ctx) override;
  virtual void draw_microtiming(const DeviceContext &ctx, uint8_t speed,
                                uint8_t timing) override;
  virtual void draw_microtiming_signed(const DeviceContext &ctx, uint8_t speed,
                                       int8_t microtiming) override;
  virtual void close_microtiming(const DeviceContext &ctx) override;
  virtual void clear_popup(const DeviceContext &ctx) override;
  virtual void popup_text(const DeviceContext &ctx, char *text,
                          uint8_t persistent = 0) override;
  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *target, uint8_t *param) const override;

private:
  MDClass &md() const { return (MDClass &)device_; }
};
#endif

#if !defined(__AVR__)
class MDPanelCapability : public DevicePanelCapability {
public:
  explicit MDPanelCapability(MDClass &device) : device_(device) {}
  virtual void set_key_repeat(uint8_t enabled) override;
  virtual void set_rec_mode(uint8_t mode) override;
  virtual void sync_seqtrack(uint8_t length, uint8_t speed,
                             uint8_t step_count,
                             uint8_t swing_amount = 0x7F) override;
  virtual void popup_text(uint8_t action_string,
                          uint8_t persistent = 0) override;
  virtual void popup_text(char *text, uint8_t persistent = 0) override;
  virtual void popup_text_P(const char *text_P,
                            uint8_t persistent = 0) override;
  virtual void popup_text_P(const char *text1_P, const char *text2_P,
                            uint8_t persistent = 0) override;

private:
  MDClass &device_;
};
#endif

} // namespace

#if !defined(__AVR__)
ElektronSysexObject *MDClass::getGlobal() {
  return md_global_sysex();
}
#endif

static uint16_t send_md_request3(MDClass &md, uint8_t command, uint8_t param,
                                 uint8_t value, bool send = true) {
  uint8_t data[3] = {command, param, value};
  return md.sendRequest(data, sizeof(data), send);
}

static constexpr uint8_t MD_GATEWAY_LOAD_SAMPLE_BANK = 0x63;

static bool read_md_sample_bank_response(MDClass &md, uint8_t msgType,
                                         uint8_t &bank) {
  uint8_t begin = md.sysex_protocol.header_size + 1;
  auto listener = md.getSysexListener();
  if (!listener || !listener->sysex || listener->msg_rd >= NUM_SYSEX_MSGS) {
    return false;
  }
  const uint8_t msg_rd = listener->msg_rd;
  if (!listener->sysex->ledger[msg_rd].ptr) {
    return false;
  }
  const uint16_t record_len = listener->sysex->ledger[msg_rd].recordLen;
  if (listener->sysex->ledger[msg_rd].state != SYSEX_STATE_FIN ||
      record_len < (uint16_t)(begin + 3)) {
    return false;
  }

  SysexView sysex(listener->sysex, msg_rd);
  if (msgType != 0x72 ||
      sysex.getByte(begin) != MD_GATEWAY_LOAD_SAMPLE_BANK ||
      sysex.getByte(begin + 1) == 0) {
    return false;
  }

  uint8_t current_bank = sysex.getByte(begin + 2);
  if (current_bank >= 128) {
    return false;
  }
  bank = current_bank;
  return true;
}

static void send_global_setting(MDClass &md, uint8_t setting, uint8_t value) {
  send_md_request3(md, 0x70, setting, value);
}

static bool md_has_required_fw_caps(const MDClass &md) NOINLINE();
static bool md_has_required_fw_caps(const MDClass &md) {
  static constexpr uint32_t required_caps =
      ((uint32_t)FW_CAP_MASTER_FX | (uint32_t)FW_CAP_TRIG_LEDS |
       (uint32_t)FW_CAP_UNDOKIT_SYNC | (uint32_t)FW_CAP_TONAL |
       (uint32_t)FW_CAP_ENHANCED_GUI | (uint32_t)FW_CAP_ENHANCED_MIDI) |
      (uint32_t)FW_CAP_MACHINE_CACHE | (uint32_t)FW_CAP_UNDO_CACHE |
      (uint32_t)FW_CAP_MID_MACHINE | (uint32_t)FW_CAPS_LENGTH_CHECK;
  return (md.fw_caps & required_caps) == required_caps;
}

DeviceMixerCapability *MDClass::mixer() {
  return &md_mixer_capability;
}

#if !defined(__AVR__)
DeviceStepTrackCapability *MDClass::step_tracks() {
  static MDStepTrackCapability capability(*this);
  return &capability;
}

DeviceStepEditCapability *MDClass::step_edit() {
  static MDStepEditCapability capability(*this);
  return &capability;
}
#endif

#if !defined(__AVR__)
DeviceParamCapability *MDClass::params() {
  static MDParamCapability capability(*this);
  return &capability;
}

DevicePerfCapability *MDClass::perf() {
  static MDPerfCapability capability(*this);
  return &capability;
}
#endif

#if !defined(__AVR__)
DevicePanelCapability *MDClass::panel() {
  static MDPanelCapability capability(*this);
  return &capability;
}

bool MDStepTrackCapability::available(const DeviceContext &ctx) const {
  (void)ctx;
  return true;
}

uint8_t MDStepTrackCapability::track_count(const DeviceContext &ctx) const {
  (void)ctx;
  return mcl_seq.num_md_tracks;
}

SeqStepTrackRef MDStepTrackCapability::track(const DeviceContext &ctx,
                                             uint8_t track_idx) const {
  (void)ctx;
  if (track_idx >= mcl_seq.num_md_tracks) {
    track_idx = 0;
  }
#if !defined(__AVR__)
  if (mcl_seq.using_spsx_tracks) {
    return SeqStepTrackRef(mcl_seq.spsx_tracks[track_idx]);
  }
#endif
  return SeqStepTrackRef(mcl_seq.md_tracks[track_idx]);
}

SeqStepTrackRef MDStepTrackCapability::active_track(
    const DeviceContext &ctx) const {
  return track(ctx, last_primary_track);
}

bool MDStepTrackCapability::parses_kit_cc(const DeviceContext &ctx) const {
  (void)ctx;
  return true;
}

bool MDStepTrackCapability::parse_kit_cc(const DeviceContext &ctx,
                                         uint8_t channel, uint8_t cc,
                                         uint8_t *track,
                                         uint8_t *param) const {
  (void)ctx;
  if (track == nullptr || param == nullptr) {
    return false;
  }
  md().parseCC(channel, cc, track, param);
  return *track != 255;
}
#endif

uint8_t MDMixerCapability::track_count(const DeviceContext &ctx) const {
  (void)ctx;
  return mcl_seq.num_md_tracks;
}

SeqTrack *MDMixerCapability::seq_track(const DeviceContext &ctx,
                                       uint8_t track) {
  (void)ctx;
  if (track >= mcl_seq.num_md_tracks) {
    return nullptr;
  }
  return &SeqTrackUtil::get_seq_track(true, track);
}

bool MDMixerCapability::param(const DeviceContext &ctx, uint8_t track,
                              uint8_t param_idx,
                              MidiDeviceMixerParam *param) {
  (void)ctx;
  if (param == nullptr || track >= NUM_MD_TRACKS) {
    return false;
  }

  MDClass &device = md();
  MidiDeviceMixerValue value = 0;
  if (param_idx == MODEL_LEVEL) {
    value = device.kit.levels[track];
  } else {
    uint8_t param_limit =
        device.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
    if (param_idx >= param_limit) {
      return false;
    }
    value = device.kit.params[track][param_idx];
  }

  param->set_value(value);
  param->set_metadata(nullptr, 0, true);
  return true;
}

bool MDMixerCapability::set_param(const DeviceContext &ctx, uint8_t track,
                                  uint8_t param_idx,
                                  MidiDeviceMixerValue value,
                                  bool send) {
  (void)ctx;
  (void)send;
  if (track >= NUM_MD_TRACKS) {
    return false;
  }
  MDClass &device = md();
  uint8_t param_limit =
      device.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  if (param_idx != MODEL_LEVEL && param_idx >= param_limit) {
    return false;
  }
  if (value < 0) value = 0;
  if (value > 127) value = 127;
  device.setTrackParam(track, param_idx, (uint8_t)value, nullptr, true);
  return true;
}

#if !defined(__AVR__)
void MDMixerCapability::fill_track(const DeviceContext &ctx, uint8_t track,
                                   bool fill, MidiUartClass *uart_) {
  (void)ctx;
  if (md().global.baseChannel == 127 || track >= NUM_MD_TRACKS) {
    return;
  }
  md().sendCC(md().global.baseChannel + (track >> 2), 68 + (track & 3),
              fill ? 127 : 0, uart_);
}
#endif

void MDMixerCapability::set_record_mutes(const DeviceContext &ctx,
                                         uint8_t track, bool state,
                                         bool clear) {
  (void)ctx;
  if (track >= NUM_MD_TRACKS) {
    return;
  }
  SeqTrack &seq_track = SeqTrackUtil::get_seq_track(true, track);
  seq_track.record_mutes = state;
  if (clear) {
    SeqTrackUtil::with_md_track(track, [](auto &t) { t.clear_mute(); });
  }
}

uint8_t MDMixerCapability::trig_group(const DeviceContext &ctx,
                                      uint8_t track) const {
  (void)ctx;
  if (track >= NUM_MD_TRACKS) {
    return 255;
  }
  return md().kit.trigGroups[track];
}

void MDMixerCapability::select_track(const DeviceContext &ctx, uint8_t track) {
  (void)ctx;
  if (track < NUM_MD_TRACKS) {
    md().setStatus(0x22, track);
  }
}

void MDMixerCapability::restore_track_params(const DeviceContext &ctx,
                                             uint8_t track) {
  (void)ctx;
  if (track >= NUM_MD_TRACKS) {
    return;
  }
  MDClass &device = md();
  uint8_t num_params =
      device.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  for (uint8_t param = 0; param < num_params; param++) {
    device.restore_kit_param(track, param);
  }
}

bool MDMixerCapability::parse_cc(const DeviceContext &ctx, uint8_t channel,
                                 uint8_t cc, uint8_t *track,
                                 uint8_t *param) const {
  (void)ctx;
  if (track == nullptr || param == nullptr) {
    return false;
  }
  md().parseCC(channel, cc, track, param);
  return *track != 255;
}

#if !defined(__AVR__)
void MDPanelCapability::set_key_repeat(uint8_t enabled) {
  device_.set_key_repeat(enabled);
}

void MDPanelCapability::set_rec_mode(uint8_t mode) {
  device_.set_rec_mode(mode);
}

void MDPanelCapability::sync_seqtrack(uint8_t length, uint8_t speed,
                                      uint8_t step_count,
                                      uint8_t swing_amount) {
  device_.sync_seqtrack(length, speed, step_count, swing_amount);
}

void MDPanelCapability::popup_text(uint8_t action_string, uint8_t persistent) {
  device_.popup_text(action_string, persistent);
}

void MDPanelCapability::popup_text(char *text, uint8_t persistent) {
  device_.popup_text(text, persistent);
}

void MDPanelCapability::popup_text_P(const char *text_P, uint8_t persistent) {
  device_.popup_text_P(text_P, persistent);
}

void MDPanelCapability::popup_text_P(const char *text1_P, const char *text2_P,
                                     uint8_t persistent) {
  device_.popup_text_P(text1_P, text2_P, persistent);
}
#endif

#if !defined(__AVR__)
bool MDStepEditCapability::available(const DeviceContext &ctx) const {
  (void)ctx;
  return md().global.extendedMode == 2;
}

void MDStepEditCapability::set_rec_mode(const DeviceContext &ctx,
                                        uint8_t mode) {
  (void)ctx;
  md().set_rec_mode(mode);
}

void MDStepEditCapability::sync_track(const DeviceContext &ctx, uint8_t length,
                                      uint8_t speed, uint8_t step_count,
                                      uint8_t swing_amount) {
  (void)ctx;
  md().sync_seqtrack(length, speed, step_count, swing_amount);
}

void MDStepEditCapability::set_trig_leds(const DeviceContext &ctx,
                                         uint16_t mask, uint8_t mode,
                                         uint8_t blink) {
  (void)ctx;
  md().set_trigleds(mask, (TrigLEDMode)mode, blink);
}

void MDStepEditCapability::set_live_param_update(const DeviceContext &ctx,
                                                 bool enabled) {
  (void)ctx;
  if (enabled) {
    md().midi_events.enable_live_kit_update();
  } else {
    md().midi_events.disable_live_kit_update();
  }
}

bool MDStepEditCapability::configure_kit_sound_panel(
    const DeviceContext &ctx, uint8_t target, char *info, uint8_t info_len,
    uint8_t *pitch_max, bool *is_midi_model) const {
  (void)ctx;
  if (target >= NUM_MD_TRACKS || info == nullptr || info_len < 6) {
    return false;
  }

  MDClass &device = md();
  uint8_t model = device.kit.get_model(target);
  bool midi_model = ((model & 0xF0) == MID_01_MODEL);
  tuning_t const *tuning = device.getKitModelTuning(target);
  if (is_midi_model != nullptr) {
    *is_midi_model = midi_model;
  }
  if (pitch_max != nullptr) {
    if (tuning) {
      *pitch_max = tuning->len - 1 + tuning->base;
    } else if (midi_model) {
      *pitch_max = 127;
    } else {
      *pitch_max = 1;
    }
  }

  const char *str1 = getMDMachineNameShort(model, 1);
  const char *str2 = getMDMachineNameShort(model, 2);
  copyMachineNameShort(str1, info);
  info[2] = '>';
  copyMachineNameShort(str2, info + 3);
  info[5] = '\0';
  return true;
}

bool MDStepEditCapability::kit_sound_uses_note_pitch(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  if (target >= NUM_MD_TRACKS) {
    return false;
  }
  MDClass &device = md();
  uint8_t model = device.kit.get_model(target);
  return ((model & 0xF0) == MID_01_MODEL) ||
         device.getKitModelTuning(target) != nullptr;
}

bool MDStepEditCapability::kit_sound_voice_allocatable(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  return target < NUM_MD_TRACKS && md().getKitModelTuning(target) != nullptr;
}

uint8_t MDStepEditCapability::kit_sound_default_pitch(const DeviceContext &ctx,
                                                      uint8_t target) const {
  (void)ctx;
  return target < NUM_MD_TRACKS ? md().kit.params[target][0] : 0;
}

uint8_t MDStepEditCapability::kit_sound_note_from_pitch(
    const DeviceContext &ctx, uint8_t target, uint8_t pitch) const {
  (void)ctx;
  if (target >= NUM_MD_TRACKS) {
    return 255;
  }
  MDClass &device = md();
  if ((device.kit.models[target] & 0xF0) == MID_01_MODEL) {
    return pitch;
  }
  tuning_t const *tuning = device.getKitModelTuning(target);
  if (tuning == nullptr) {
    return 255;
  }
  pitch -= ptc_param_fine_tune.getValue() - 32;
  for (uint8_t i = 0; i < tuning->len; i++) {
    uint8_t cc = pgm_read_byte(&tuning->tuning[i]);
    if (cc >= pitch) {
      uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
      return i + note_offset;
    }
  }
  return 255;
}

uint8_t MDStepEditCapability::kit_sound_pitch_from_note(
    const DeviceContext &ctx, uint8_t target, uint8_t note,
    uint8_t fine_tune) const {
  (void)ctx;
  if (target >= NUM_MD_TRACKS) {
    return 255;
  }
  MDClass &device = md();
  if ((device.kit.models[target] & 0xF0) == MID_01_MODEL) {
    return note;
  }
  if (fine_tune == 255) {
    fine_tune = ptc_param_fine_tune.getValue();
  }
  tuning_t const *tuning = device.getKitModelTuning(target);
  if (tuning == nullptr) {
    return 255;
  }
  uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
  note -= note_offset;
  if (note >= tuning->len) {
    return 255;
  }
  int8_t pitch = (int8_t)pgm_read_byte(&tuning->tuning[note]) +
                 (int8_t)fine_tune - 32;
  if (pitch < 0) {
    return 0;
  }
  return pitch > 127 ? 127 : (uint8_t)pitch;
}

bool MDStepEditCapability::param_from_key(const DeviceContext &ctx,
                                          uint8_t target, uint8_t key,
                                          uint8_t *param) const {
  (void)ctx;
  if (param == nullptr || target >= NUM_MD_TRACKS || key < 0x10 ||
      key > 0x17) {
    return false;
  }
  MDClass &device = md();
  uint8_t value = device.currentSynthPage * 8 + key - 0x10;
  uint8_t param_count =
      device.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  if (value >= param_count) {
    return false;
  }
  *param = value;
  return true;
}

bool MDStepEditCapability::key_for_param(const DeviceContext &ctx,
                                         uint8_t target, uint8_t param,
                                         uint8_t *key) const {
  (void)ctx;
  if (key == nullptr || target >= NUM_MD_TRACKS) {
    return false;
  }
  MDClass &device = md();
  uint8_t param_count =
      device.is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  if (param >= param_count) {
    return false;
  }
  int16_t value =
      (int16_t)param - (int16_t)device.currentSynthPage * 8 + 0x10;
  if (value < 0x10 || value > 0x17) {
    return false;
  }
  *key = (uint8_t)value;
  return true;
}

bool MDStepEditCapability::begin_param_editor(const DeviceContext &ctx,
                                              uint8_t target,
                                              uint8_t *params,
                                              uint8_t count) {
  (void)ctx;
  uint8_t param_count =
      md().is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  if (target >= NUM_MD_TRACKS || params == nullptr || count < param_count) {
    return false;
  }
  md().activate_encoder_interface(params, param_count);
  return true;
}

void MDStepEditCapability::end_param_editor(const DeviceContext &ctx) {
  (void)ctx;
  if (md().encoder_interface) {
    md().deactivate_encoder_interface();
  }
}

void MDStepEditCapability::draw_microtiming(const DeviceContext &ctx,
                                            uint8_t speed, uint8_t timing) {
  (void)ctx;
  md().draw_microtiming(speed, timing);
}

void MDStepEditCapability::draw_microtiming_signed(const DeviceContext &ctx,
                                                   uint8_t speed,
                                                   int8_t microtiming) {
  (void)ctx;
  md().draw_microtiming_signed(speed, microtiming);
}

void MDStepEditCapability::close_microtiming(const DeviceContext &ctx) {
  (void)ctx;
  md().draw_close_microtiming();
}

void MDStepEditCapability::clear_popup(const DeviceContext &ctx) {
  (void)ctx;
  md().popup_text(127, 2);
}

void MDStepEditCapability::popup_text(const DeviceContext &ctx, char *text,
                                      uint8_t persistent) {
  (void)ctx;
  md().popup_text(text, persistent);
}

bool MDStepEditCapability::parse_cc(const DeviceContext &ctx, uint8_t channel,
                                    uint8_t cc, uint8_t *target,
                                    uint8_t *param) const {
  (void)ctx;
  if (target == nullptr || param == nullptr) {
    return false;
  }
  md().parseCC(channel, cc, target, param);
  return *target != 255;
}
#endif

#if !defined(__AVR__)
bool MDPerfCapability::perf_scene_autofill(const DeviceContext &ctx,
                                           uint8_t dest_offset,
                                           PerfData *data, uint8_t scene) {
  (void)ctx;
  if (data == nullptr || scene >= NUM_SCENES) {
    return false;
  }

  MDClass &device = md();
  uint8_t num_params =
      mcl_seq.using_spsx_tracks ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  bool filled = false;
  for (uint8_t track = 0; track < NUM_MD_TRACKS; track++) {
    uint8_t dest = dest_offset + track;
    for (uint8_t param = 0; param < num_params; param++) {
      if (device.kit.params[track][param] ==
          device.kit.params_orig[track][param]) {
        continue;
      }
      uint8_t value = device.kit.params[track][param];
      if (data->add_param(dest, param, scene, value) == 255) {
        continue;
      }
      // Kit encoders go back to normal for save.
      device.setTrackParam(track, param, device.kit.params_orig[track][param],
                           nullptr, true);
      device.setTrackParam(track, param, value, nullptr, false);
      filled = true;
    }
  }

  uint8_t *fxs = (uint8_t *)&device.kit.reverb;
  uint8_t *fxs_orig = (uint8_t *)&device.kit.fx_orig;
  for (uint8_t n = 0; n < 8 * 4; n++) {
    uint8_t fx = n / 8;
    uint8_t param = n - fx * 8;
    // Delay and reverb are flipped in memory.
    if (fx == 0) {
      fx = 1;
    } else if (fx == 1) {
      fx = 0;
    }
    if (fxs[n] == fxs_orig[n]) {
      continue;
    }
    uint8_t dest = dest_offset + NUM_MD_TRACKS + fx;
    uint8_t value = fxs[n];
    if (data->add_param(dest, param, scene, value) == 255) {
      continue;
    }
    device.setFXParam(param, fxs_orig[n], fx + MD_FX_ECHO, true);
    device.setFXParam(param, value, fx + MD_FX_ECHO, false);
    filled = true;
  }
  return filled;
}

uint8_t MDParamCapability::target_count(const DeviceContext &ctx) const {
  (void)ctx;
  return NUM_MD_TRACKS + 4;
}

uint8_t MDParamCapability::param_count(const DeviceContext &ctx,
                                       uint8_t target) const {
  (void)ctx;
  if (target < NUM_MD_TRACKS) {
    return md().is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  }
  if (target < NUM_MD_TRACKS + 4) {
    return 8;
  }
  return 0;
}

bool MDParamCapability::target_label(const DeviceContext &ctx, uint8_t target,
                                     char *out, uint8_t len) const {
  (void)ctx;
  if (target < NUM_MD_TRACKS) {
    return false;
  }
  const char *label = nullptr;
  switch (target - NUM_MD_TRACKS) {
  case 0:
    label = "ECH";
    break;
  case 1:
    label = "REV";
    break;
  case 2:
    label = "EQ";
    break;
  case 3:
    label = "DYN";
    break;
  default:
    return false;
  }
  if (out != nullptr && len > 0) {
    strncpy(out, label, len - 1);
    out[len - 1] = '\0';
  }
  return true;
}

bool MDParamCapability::param_label(const DeviceContext &ctx, uint8_t target,
                                    uint8_t param, char *out, uint8_t len) {
  const char *label = nullptr;
  MDClass &device = md();
  if (target < NUM_MD_TRACKS) {
    if (param >= param_count(ctx, target)) {
      return false;
    }
    label = model_param_name(device.kit.get_model(target), param);
  } else if (target < NUM_MD_TRACKS + 4) {
    if (param >= 8) {
      return false;
    }
    label = fx_param_name(md_target_fx_type(target), param);
  } else {
    return false;
  }
  if (label == nullptr) {
    return false;
  }
  if (out != nullptr && len > 0) {
    strncpy(out, label, len - 1);
    out[len - 1] = '\0';
  }
  return true;
}

bool MDParamCapability::get_param(const DeviceContext &ctx, uint8_t target,
                                  uint8_t param, uint8_t *value) {
  if (value == nullptr) {
    return false;
  }
  MDClass &device = md();
  if (target < NUM_MD_TRACKS) {
    if (param >= param_count(ctx, target)) {
      return false;
    }
    *value = device.kit.params[target][param];
    return true;
  }
  if (target < NUM_MD_TRACKS + 4 && param < 8) {
    *value = device.kit.get_fx_param(md_target_fx_type(target), param);
    return true;
  }
  return false;
}

bool MDParamCapability::set_param(const DeviceContext &ctx, uint8_t target,
                                  uint8_t param, uint8_t value,
                                  MidiUartClass *uart_,
                                  bool update_kit) {
  MDClass &device = md();
  if (target < NUM_MD_TRACKS) {
    if (param >= param_count(ctx, target)) {
      return false;
    }
    device.setTrackParam(target, param, value, uart_, update_kit);
    return true;
  }
  if (target < NUM_MD_TRACKS + 4 && param < 8) {
    device.setFXParam(param, value, md_target_fx_type(target), false, uart_);
    return true;
  }
  return false;
}

bool MDParamCapability::sequencer_lock_param_label(const DeviceContext &ctx,
                                                   uint8_t target,
                                                   uint8_t param, char *out,
                                                   uint8_t len) {
  if (out == nullptr || len == 0 || target >= NUM_MD_TRACKS ||
      param >= param_count(ctx, target)) {
    return false;
  }
  MDClass &device = md();
  const char *label = model_param_name(device.kit.get_model(target), param);
  if (label == nullptr) {
    return false;
  }
  uint8_t pos = 0;
  while (label[pos] != '\0' && pos + 1 < len && pos < 3) {
    out[pos] = label[pos];
    pos++;
  }
  out[pos] = '\0';
  if (pos == 2 && pos + 1 < len) {
    out[pos++] = ' ';
    out[pos] = '\0';
  }
  return true;
}

bool MDParamCapability::sequencer_uses_step_pitch(const DeviceContext &ctx,
                                                  uint8_t target) const {
  (void)ctx;
  return target < NUM_MD_TRACKS;
}
#endif

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
  send_global_setting(*this, 0x4A, channel);
}

void MDClass::setLocalOn(bool localOn) {
  send_global_setting(*this, 0x4B, localOn);
}

void MDClass::setProgramChange(uint8_t val) {
  send_global_setting(*this, 0x4C, val);
}

#if !defined(__AVR__)
void MDClass::requestKit(uint8_t kit) {
  uint8_t ver = is_spsx ? SYSEX_VERSION_SPSX : SYSEX_VERSION_LEGACY;
  uint8_t data[] = {sysex_protocol.kitrequest_id, kit, ver};
  sendRequest(data, sizeof(data));
}

void MDClass::requestPattern(uint8_t pattern) {
  uint8_t ver = is_spsx ? SYSEX_VERSION_SPSX : SYSEX_VERSION_LEGACY;
  uint8_t data[] = {sysex_protocol.patternrequest_id, pattern, ver};
  sendRequest(data, sizeof(data));
}

void MDClass::requestGlobal(uint8_t global) {
  uint8_t ver = is_spsx ? SYSEX_VERSION_SPSX : SYSEX_VERSION_LEGACY;
  uint8_t data[] = {sysex_protocol.globalrequest_id, global, ver};
  sendRequest(data, sizeof(data));
}
#endif

void MDClass::setChannelMode(uint8_t mode) {
  send_global_setting(*this, 0x4F, mode);
  global.channelMode = mode;
}

void MDClass::setExternalSync() {
  uint8_t b = 0;
  //  clockIn = false;
  //  transportIn = true;
  //  clockOut = true;
  //  transportOut = true;

  b = global.clockIn;

  if (global.transportIn) {
    b |= 1 << 4;
  }

  if (global.clockOut) {
    b |= 1 << 5;
  }

  if (global.transportOut) {
    b |= 1 << 6;
  }

  send_global_setting(*this, 0x4D, b);
}

#ifdef PLATFORM_TBD
void MDClass::on_connection(DeviceIdx device_idx) {
  init_grid_devices(device_idx);
}

void MDClass::ui_loop() {
  ui.loop();
}

bool MDClass::handle_ui_event(gui_event_t *event) {
  return ui.handle_event(event);
}

bool MDClass::enter_ui(gui_event_t *event) {
  return ui.enter(event);
}

bool MDClass::is_ui_active() {
  return ui.is_active();
}

bool MDClass::is_ui_collapsed() {
  return ui.is_collapsed();
}

bool MDClass::toggle_ui_display_mode() {
  return ui.toggle_display_mode();
}

void MDClass::exit_ui() {
  ui.exit();
}

void MDClass::on_ui_slot_button(uint8_t slot, bool pressed) {
  (void)slot;
  ui.handle_ui_slot_button(pressed);
}
#endif

void MDClass::init_grid_devices(DeviceIdx device_idx) {
  uint8_t legacy_device_idx = static_cast<uint8_t>(device_idx);

  GridDeviceTrack gdt;
#if !defined(__AVR__)
  bool use_spsx_tracks = mcl_seq.using_spsx_tracks;
  if (is_spsx != use_spsx_tracks) {
    if (!(is_spsx ? mcl_seq.switch_to_spsx() : mcl_seq.switch_to_legacy())) {
      return;
    }
    use_spsx_tracks = is_spsx;
  }

  if (use_spsx_tracks) {
    for (uint8_t i = 0; i < NUM_MD_TRACKS; i++) {
      gdt.init(MDSPSX_TRACK_TYPE, GROUP_DEV, legacy_device_idx,
               (SeqTrack *)&(mcl_seq.spsx_tracks[i]));
      add_track_to_grid(GridIdx::X, i, &gdt);
    }
  } else {
#endif
    for (uint8_t i = 0; i < NUM_MD_TRACKS; i++) {
      gdt.init(MD_TRACK_TYPE, GROUP_DEV, legacy_device_idx,
               &(mcl_seq.md_tracks[i]));
      add_track_to_grid(GridIdx::X, i, &gdt);
    }
#if !defined(__AVR__)
  }
#endif

  gdt.init(MDFX_TRACK_TYPE, GROUP_DEV, legacy_device_idx,
           (SeqTrack *)&(mcl_seq.mdfx_track), 0);
  add_track_to_grid(GridIdx::Y, MDFX_TRACK_NUM, &gdt);

  gdt.init(MD_ROUTE_TRACK_TYPE, GROUP_AUX, legacy_device_idx,
           (SeqTrack *)&(mcl_seq.aux_tracks[1]), 0);
  add_track_to_grid(GridIdx::Y, MDROUTE_TRACK_NUM, &gdt);

  gdt.init(MDTEMPO_TRACK_TYPE, GROUP_TEMPO, legacy_device_idx,
           (SeqTrack *)&(mcl_seq.aux_tracks[2]), 0);
  add_track_to_grid(GridIdx::Y, MDTEMPO_TRACK_NUM, &gdt);

  gdt.init(PERF_TRACK_TYPE, GROUP_PERF, legacy_device_idx,
           (SeqTrack *)&(mcl_seq.perf_track), 0);
  add_track_to_grid(GridIdx::Y, PERF_TRACK_NUM, &gdt);
}

void MDClass::get_mutes() {
  uint16_t mutes;
  uint16_t fill_state;
  uint8_t state = get_track_state(mutes, fill_state);
  if (state) {
    for (uint8_t n = 0; n < 16; n++, mutes >>= 1) {
      uint8_t m = mutes & 1;
      SeqTrackUtil::with_md_track(n, [m](auto &t) { t.mute_state = m; });
      DEBUG_PRINTLN(m);
    }
    if (state & 2) {
      mcl_seq.set_fill_mask(DeviceIdx::Primary, fill_state);
    }
  } else {
    DEBUG_PRINTLN("mute state failed");
  }
}

bool MDClass::probe() {
  DEBUG_PRINT_FN();

  bool ti = key_interface.state;

  md_track_select.off();
  if (ti) {
    key_interface.off();
  }
  DEBUG_PRINTLN("md probe");
  connected = false;

  // Begin main probe sequence — derive port from uart pointer
  uint8_t probe_port = port;
  if (uart->device.getBlockingId(DEVICE_MD, DEVICE_SPS, probe_port, CALLBACK_TIMEOUT)) {
    uint8_t count = 3;

    while ((!get_fw_caps() || !md_has_required_fw_caps(*this)) && count) {
      DEBUG_PRINTLN("bad caps");
      mcl_gui.delay_progress(250);
      count--;
    }

    if (!md_has_required_fw_caps(*this)) {
      oled_display.textbox_P(mclstr_upgrade, mclstr_machinedrum);
      oled_display.display();
      return false;
    }

#if !defined(__AVR__)
    is_spsx = (fw_caps & FW_CAP_SPSX) != 0;
#endif

    turbo_light.set_speed(turbo_light.lookup_speed(
        (probe_port == UARTUSB_PORT) ? mcl_cfg.usb_turbo_speed : mcl_cfg.uart1_turbo_speed), uart);
    mcl_gui.delay_progress(100);

    //   if (mcl_cfg.clock_rec == 0) {
    //     MidiClock.uart_clock_recv = uart;
    //   }
    mcl_gui.delay_progress(300);
    getCurrentTrack(CALLBACK_TIMEOUT);
#if !defined(__AVR__)
    if (is_spsx) {
      uint8_t active_global = getCurrentGlobal(CALLBACK_TIMEOUT);
      if (active_global != 255) {
        getBlockingGlobal(active_global, CALLBACK_TIMEOUT);
      }
    }
#endif
    getBlockingKit(0x7F);
    MD.save_kit_params();
    setup();

    for (uint8_t i = 0; i < 32; i++) {
      mcl_gui.draw_progress_bar(60, 60, false, 60, 25);
      setStatus(0x22, i & 0x0F);
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
    DEBUG_PRINTLN(F("delay"));
    mcl_gui.delay_progress(4600);
  }

  return connected;
}

uint8_t MDClass::noteToTrack(uint8_t pitch) {
 for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
   if (pitch == MD.global.drumMapping[i])
        return i;
    }
 return 128;
}

void MDClass::parseCC(uint8_t channel, uint8_t cc, uint8_t *track,
                      uint8_t *param) {

  uint8_t control_ch = channel - global.baseChannel;

  const bool expanded_channel_mode = is_spsx && global.channelMode;

  if (control_ch >= (expanded_channel_mode ? 8 : 4)) {
    *track = 255;
    return;
  }

  // Extended channels (base+4..+7): params 24-33
  if (expanded_channel_mode && control_ch >= 4) {
    *track = (control_ch - 4) * 4;
    if (cc > 39) { *track = 255; return; }
    *track += cc % 4;
    *param = (cc / 4) + MD_PARAMS_PER_TRACK;
    return;
  }

  *track = control_ch * 4;

  if (cc < 16) {
    if (cc > 11) {
      *track += cc - 12;
      *param = MODEL_MUTE;
      return;
    }
    if (cc > 7) {
      *track += (cc - 8);
      *param = MODEL_LEVEL;
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
                           MidiUartClass *uart_) {
  if (global.drumMapping[track] != -1 && global.baseChannel != 127) {
    sendNoteOn(global.baseChannel, global.drumMapping[track], velocity, uart_);
  }
}

void MDClass::sync_seqtrack(uint8_t length, uint8_t speed, uint8_t step_count,
                            uint8_t swing_amount, uint8_t swing_mode,
                            MidiUartClass *uart_) {
#if !defined(__AVR__)
  use_spsx_longest_track_sync(length, speed, step_count);
#endif
  uint8_t data[7] = {0x70, 0x3D, length, speed, step_count, swing_amount,
                     swing_mode};
  sendRequest(data, sizeof(data), true, uart_);
}

void MDClass::parallelTrig(uint16_t mask, MidiUartClass *uart_) {
  uint8_t a;
  uint8_t b;
  uint8_t c;

  a = mask & 0x7F;
  mask = mask >> 7;
  c = mask >> 7 & 0xF7;
  b = mask & 0x7F;

  sendNoteOn(global.baseChannel + 1, a, b, uart_);
  if (c > 0) {
    sendNoteOn(global.baseChannel + 2, c, 0, uart_);
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
                            MidiUartClass *uart_, bool update_kit) {
  setTrackParam_inline(track, param, value, uart_, update_kit);
}

void MDClass::setTrackParam_inline(uint8_t track, uint8_t param, uint8_t value,
                                   MidiUartClass *uart_, bool update_kit) {

  uint8_t channel = track >> 2;
  uint8_t b = track & 3;
  uint8_t cc = 0;
  if (param < MD_PARAMS_PER_TRACK) {
    cc = param;
    if (b < 2) {
      cc += 16 + b * 24;
    } else {
      cc += 24 + b * 24;
    }
    if (update_kit) {
      kit.params[track][param] = value;
    }
  } else if (param >= MD_PARAMS_PER_TRACK && param < SPS_PARAMS_PER_TRACK &&
             is_spsx && global.channelMode) {
    // Extended params on extended channels (base+4..+7)
    uint8_t ext_channel = channel + 4 + global.baseChannel;
    cc = (param - MD_PARAMS_PER_TRACK) * 4 + b;
    if (update_kit) {
      kit.params[track][param] = value;
      sendCC(ext_channel, cc, value, uart_);
    } else {
      sendPolyKeyPressure(ext_channel, cc, value, uart_);
    }
    return;
  } else if (param == MODEL_MUTE) { // MUTE
    cc = 12 + b;
  } else if (param == MODEL_LEVEL) { // LEV
    if (update_kit) {
      kit.levels[track] = value;
    }
    cc = 8 + b;
  } else {
    return;
  }
  if (update_kit) {
    sendCC(channel + global.baseChannel, cc, value, uart_);
  } else {
    sendPolyKeyPressure(channel + global.baseChannel, cc, value, uart_);
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

static uint8_t md_send_fx_bulk(MDClass &md, uint8_t cmd, uint8_t *values,
                               bool send) NOINLINE();
static uint8_t md_send_fx_bulk(MDClass &md, uint8_t cmd, uint8_t *values,
                               bool send) {
  uint8_t data[2 + 8 * 4] = {0x70, cmd};
  memcpy(&data[2], values, 8 * 4);
  return md.sendRequest(data, sizeof(data), send);
}

uint8_t MDClass::assignFXParamsBulk(uint8_t *values, bool send) {
  return md_send_fx_bulk(*this, 0x5a, values, send);
}

uint8_t MDClass::sendFXParamsBulk(uint8_t *values, bool send) {
  return md_send_fx_bulk(*this, 0x61, values, send);
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
                         bool update_kit, MidiUartClass *uart_) {

  uint8_t len = 4;
  if (update_kit) {
    uint8_t *fx_params = MD.kit.fx_params(type);
    if (fx_params != nullptr) {
      fx_params[param] = value;
    }
    len = 3;
  }
  uint8_t data[4] = {type, param, value, 0x7F};
  sendRequest(data, len, true, uart_);
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
  send_md_request3(*this, 0x62, (uint8_t)(track << 3 | param), value);
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
  send_md_request3(*this, 0x5a, pitch, track);
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
  return send_md_request3(*this, 0x5c, track, output, send);
}

void MDClass::setTrigGroup(uint8_t srcTrack, uint8_t trigTrack) {
  send_md_request3(*this, 0x65, srcTrack, trigTrack);
}

void MDClass::setMuteGroup(uint8_t srcTrack, uint8_t muteTrack) {
  send_md_request3(*this, 0x66, srcTrack, muteTrack);
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
  uint8_t num_params = is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  for (uint8_t i = 0; i < num_params; i++) {
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
  for (uint8_t i = 0; i < MD_PARAMS_PER_TRACK; i++) {
    setTrackParam(track, i, machine->params[i]);
  }
}

#if !defined(__AVR__)
void MDClass::setMachine(uint8_t track, SPSMachine *machine) {
  assignMachine(track, machine->get_model(), machine->get_tonal());
  setLFO(track, &(machine->lfos[0]), false);
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
  uint8_t num_params = is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  for (uint8_t i = 0; i < num_params; i++) {
    setTrackParam(track, i, machine->params[i]);
  }
}
#endif

uint8_t MDClass::assignMachineBulk(uint8_t track, MDMachine *machine,
                                   uint8_t level, uint8_t mode, bool send) {

  DEBUG_PRINT("assign machine bulk: ");
  DEBUG_PRINTLN(track);
  uint8_t data[44]; // Increased size by 1 for length byte
  data[0] = 0x70;
  data[1] = 0x5b;
  uint8_t i = 2;

  // Reserve space for length - will be calculated later
  uint8_t length_index = i++;

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
    DEBUG_PRINT("level ");
    DEBUG_PRINTLN(level);
    data[i++] = level;
  }
  DEBUG_PRINT("i : ");
  DEBUG_PRINTLN(i);
end:
  // Calculate and insert length (total data length minus the command bytes and length byte itself)
  uint8_t payload_length = i - 1;
  data[length_index] = payload_length;

  DEBUG_PRINT("payload length: ");
  DEBUG_PRINTLN(payload_length);

  return sendRequest(data, i, send);
}

#if !defined(__AVR__)
uint8_t MDClass::assignMachineBulk(uint8_t track, SPSMachine *machine,
                                   uint8_t level, uint8_t mode, bool send) {
  uint8_t data[54];
  data[0] = 0x70;
  data[1] = 0x5b;
  uint8_t i = 2;

  uint8_t length_index = i++;

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

  uint8_t num_params = is_spsx ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  uint8_t num_lfos = is_spsx ? 2 : 1;

  if (mode == 0) {
    goto end;
  }

  memcpy(data + i, machine->params, num_params);
  i += num_params;

  if (mode == 1) {
    goto end;
  }

  for (uint8_t lfo = 0; lfo < num_lfos; lfo++) {
    memcpy(data + i, &machine->lfos[lfo], 5);
    i += 5;
  }
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
  data[length_index] = i - 1;
  return sendRequest(data, i, send);
}
#endif

void MDClass::loadMachinesCache(uint32_t track_mask, MidiUartClass *uart_) {
  DEBUG_PRINTLN("load machine cache");
  uint8_t a = track_mask & 0x7F;
  uint8_t b = (track_mask >> 7) & 0x7F;
  uint8_t c = (track_mask >> 14) & 0x7F;
  uint8_t data[5] = {0x70, 0x62, a, b, c};
  sendRequest(data, countof(data), true, uart_);
}

bool MDClass::loadSampleBank(uint8_t bank, bool send) {
  if (bank >= 128 || !connected || !(fw_caps & FW_CAP_SAMPLE_BANK)) {
    return false;
  }

  uint8_t data[3] = {0x70, MD_GATEWAY_LOAD_SAMPLE_BANK, bank};
  return sendRequest(data, sizeof(data), send) != 0;
}

bool MDClass::querySampleBank(uint8_t &bank) {
  if (!connected || !(fw_caps & FW_CAP_SAMPLE_BANK)) {
    return false;
  }

  uint8_t data[2] = {0x70, MD_GATEWAY_LOAD_SAMPLE_BANK};
  sendRequest(data, sizeof(data));

  return read_md_sample_bank_response(*this, waitBlocking(), bank);
}

void MDClass::setOrigParams(uint8_t track, MDMachine *machine) {
  MDKit *kit_ = &kit;
  memcpy(kit_->params_orig[track], machine->params, MD_PARAMS_PER_TRACK);
}

#if !defined(__AVR__)
void MDClass::setOrigParams(uint8_t track, SPSMachine *machine) {
  MDKit *kit_ = &kit;
  memcpy(kit_->params_orig[track], machine->params, SPS_PARAMS_PER_TRACK);
}
#endif

void MDClass::insertMachineInKit(uint8_t track, MDMachine *machine,
                                 bool set_level) {

  DEBUG_PRINT("insert machine in kit ");
  DEBUG_PRINTLN(track);

  MDKit *kit_ = &kit;

  memcpy(kit_->params[track], machine->params, MD_PARAMS_PER_TRACK);
#if !defined(__AVR__)
  if (is_spsx) {
    // Mirror MDKit::fromSysex legacy fallback (host MDTypes.cpp): envelope
    // bypass-on, retrig off + RENV on. Keeps single-machine inserts
    // consistent with full-kit inserts.
    memset(&kit_->params[track][MD_PARAMS_PER_TRACK], 0,
           SPS_PARAMS_PER_TRACK - MD_PARAMS_PER_TRACK);
    kit_->params[track][MODEL_ENVATT]  = 0;
    kit_->params[track][MODEL_ENVHLD]  = 0;
    kit_->params[track][MODEL_ENVDCY]  = 127;
    kit_->params[track][MODEL_ENVMIX]  = 127;
    kit_->params[track][MODEL_LFO2SPD] = 64;
    kit_->params[track][MODEL_LFO2DEP] = 0;
    kit_->params[track][MODEL_LFO2MIX] = 0;
    kit_->params[track][MODEL_RTRG]    = 0;
    kit_->params[track][MODEL_RTIM]    = 0;
    kit_->params[track][MODEL_RENV]    = 1;   // envelope reset ON
  }
#endif
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

#if !defined(__AVR__)
void MDClass::insertMachineInKit(uint8_t track, SPSMachine *machine,
                                 bool set_level) {
  MDKit *kit_ = &kit;

  memcpy(kit_->params[track], machine->params, SPS_PARAMS_PER_TRACK);
  setOrigParams(track, machine);

  if (set_level) {
    kit_->levels[track] = machine->level;
  }
  kit_->models[track] = machine->get_model_raw();

  if (machine->lfos[0].destinationTrack > 15) {
    machine->lfos[0].destinationTrack = track;
  }
  if (machine->lfos[1].destinationTrack > 15) {
    machine->lfos[1].destinationTrack = track;
  }
  memcpy(&(kit_->lfos[track]),  &machine->lfos[0], sizeof(MDLFO));
  memcpy(&(kit_->lfosB[track]), &machine->lfos[1], sizeof(MDLFO));

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
#endif

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
    DEBUG_PRINTLN("level changing");
    DEBUG_PRINTLN(machine->level);
    level = machine->level;
  }

  bytes = MD.assignMachineBulk(track, machine, level, 255, send);

  return bytes;
}

#if !defined(__AVR__)
uint8_t MDClass::sendMachine(uint8_t track, SPSMachine *machine, bool send_level,
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
#endif

void MDClass::muteTrack(uint8_t track, bool mute, MidiUartClass *uart_) {
  if (global.baseChannel == 127)
    return;
  uint8_t channel = track >> 2;
  uint8_t b = track & 3;
  uint8_t cc = 12 + b;
  sendCC(channel + global.baseChannel, cc, (uint8_t)mute, uart_);
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
  uint8_t max_base = MD.global.channelMode ? 8 : 12;
  return (MD.global.baseChannel <= max_base);
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

void MDClass::press_yes_button() { send_gui_command(MD_GUI_YES, MD_GUI_CMD_ON); }
void MDClass::release_yes_button() { send_gui_command(MD_GUI_YES, MD_GUI_CMD_OFF); }
void MDClass::press_no_button() { send_gui_command(MD_GUI_NO, MD_GUI_CMD_ON); }
void MDClass::release_no_button() { send_gui_command(MD_GUI_NO, MD_GUI_CMD_OFF); }

void MDClass::hold_scale_button() {

  send_gui_command(MD_GUI_SCALE, MD_GUI_CMD_ON);
}
void MDClass::release_scale_button() {

  send_gui_command(MD_GUI_SCALE, MD_GUI_CMD_OFF);
}

void MDClass::toggle_scale_window() {
  send_gui_command(MD_GUI_SCALE_WIN, MD_GUI_CMD_ON);
}
void MDClass::press_page_button() {
  send_gui_command(MD_GUI_PAGE, MD_GUI_CMD_ON);
}
void MDClass::release_page_button() {
  send_gui_command(MD_GUI_PAGE, MD_GUI_CMD_OFF);
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
    SeqTrack &bt = SeqTrackUtil::get_seq_track(true, n);
    old_mutes[n] = bt.mute_state;
    bt.mute_state = SEQ_MUTE_ON;
  }
  undokit_sync();

  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    SeqTrack &bt = SeqTrackUtil::get_seq_track(true, n);
    bt.mute_state = old_mutes[n];
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
