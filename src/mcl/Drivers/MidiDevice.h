#pragma once

#include "platform.h"
#include "MCLFeatureConfig.h"
#include "DeviceCapabilities.h"
#include "MidiDeviceCapabilities.h"
#include "MidiID.h"
#include "MidiDeviceParam.h"
#include <inttypes.h>

#ifdef PLATFORM_TBD
#include "GUI.h"
#endif

class ElektronDevice;
class GridDeviceTrack;
class MCLGIF;
class MidiClass;
class MidiUartClass;
struct PageSelectEntry;
class SeqTrack;

struct MidiDeviceMixerParam {
#ifdef PLATFORM_TBD
  const char *label = nullptr;
  int16_t min_value = 0;
  int16_t max_value = 127;
  int16_t value = 0;
  uint8_t type = 0;
  bool sendable = false;
#else
  uint8_t value = 0;
#endif

  void set_value(int16_t value_, int16_t min_value_ = 0,
                 int16_t max_value_ = 127) {
#ifdef PLATFORM_TBD
    min_value = min_value_;
    max_value = max_value_;
    value = value_;
#else
    (void)min_value_;
    (void)max_value_;
    value = (uint8_t)value_;
#endif
  }

  void set_metadata(const char *label_, uint8_t type_, bool sendable_) {
#ifdef PLATFORM_TBD
    label = label_;
    type = type_;
    sendable = sendable_;
#else
    (void)label_;
    (void)type_;
    (void)sendable_;
#endif
  }
};

class DeviceMixerSupport {
public:
  static bool ext_level_param(uint8_t track, uint8_t param_idx,
                              const uint8_t *levels,
                              MidiDeviceMixerParam *param,
                              bool require_level_cc = false);
  static bool set_ext_level(uint8_t track, uint8_t param_idx, int16_t value,
                            uint8_t *levels, uint8_t *level,
                            bool require_level_cc = false);
  static bool parse_ext_cc(uint8_t channel, uint8_t cc, uint8_t level_cc,
                           uint8_t mute_cc, uint8_t *track,
                           uint8_t *param);
  static void update_ext_from_cc(uint8_t track, uint8_t param, int16_t value,
                                 uint8_t *levels);
  static void set_ext_record_mute(uint8_t track, bool state,
                                  bool clear);
};

/// Base class for MIDI-compatible devices.
/// Defines basic device description data and driver interfaces.
class MidiDevice {
public:
  bool connected;
  bool in_probe;
  MidiClass *midi;
  MidiUartClass *uart;
  const char *const name;
  const uint8_t id; // Device identifier
  const bool isElektronDevice;
  uint8_t track_type;
  uint8_t port; // MIDI port number (UART1_PORT, UART2_PORT, UARTUSB_PORT, etc.)

protected:
#ifdef MCL_HAS_DEVICE_CAPABILITIES
  DeviceStepTrackCapability step_track_capability_;
  DeviceExtStepTrackCapability ext_step_track_capability_;
  DeviceStepEditCapability step_edit_capability_;
  DeviceParamCapability param_capability_;
  DevicePerfCapability perf_capability_;
#endif

public:
  MidiDevice(MidiClass *_midi, const char *_name, const uint8_t _id,
             const bool _isElektronDevice) NOINLINE();

  void add_track_to_grid(GridIdx grid_idx, uint8_t track_idx,
                         GridDeviceTrack *gdt);
  void cleanup(DeviceIdx device_idx);

  ElektronDevice *asElektronDevice() {
    if (!isElektronDevice) return nullptr;
    return (ElektronDevice *)this;
  }

  virtual void setup_listeners() {}
  virtual void cleanup_listeners() {}

  void setPort(MidiClass *_midi, uint8_t _port = 0);

  virtual void init_grid_devices(DeviceIdx device_idx) { (void)device_idx; }

  /** Called when the driver is successfully probed and connected. */
#ifdef PLATFORM_TBD
  virtual void on_connection(DeviceIdx device_idx) {
    init_grid_devices(device_idx);
  }
#else
  void on_connection(DeviceIdx device_idx) {
    init_grid_devices(device_idx);
  }
#endif

#ifdef PLATFORM_TBD
  /** Per-frame UI maintenance for the driver (polling, overlays, etc.). */
  virtual void ui_loop() {}

  /** Driver-specific raw UI event handler. Return true to consume. */
  virtual bool handle_ui_event(gui_event_t *event) {
    (void)event;
    return false;
  }

  /** Driver-specific active UI entry control. Return true to consume. */
  virtual bool enter_ui(gui_event_t *event) {
    (void)event;
    return false;
  }

  /** Returns true if this driver implements an MCL-side UI mode. */
  virtual bool supports_ui() const { return false; }

  /** Returns true if the driver has an active UI overlay or mode. */
  virtual bool is_ui_active() { return false; }

  /** Returns true if the active driver UI is display-only/pass-through. */
  virtual bool is_ui_collapsed() { return false; }

  /** Ask the driver to leave any active UI mode it owns. */
  virtual void exit_ui() {}

  /** Logical active-device UI button edge for this slot. */
  virtual void on_ui_slot_button(uint8_t slot, bool pressed) {
    (void)slot;
    (void)pressed;
  }
#endif

  virtual void setup() {}
  virtual uint8_t register_page_select_entries(PageSelectEntry *entries,
                                               uint8_t max_entries) const {
    (void)entries;
    (void)max_entries;
    return 0;
  }
  virtual void page_select_prepare() {}
  virtual void page_select_popup(char *text) {
    (void)text;
  }
  virtual void page_select_cleanup() {}
#ifdef PLATFORM_TBD
  virtual bool supports_capability(MidiDeviceCapability capability) const {
    (void)capability;
    return false;
  }
#else
  bool supports_capability(MidiDeviceCapability capability) const {
    (void)capability;
    return id == DEVICE_MD;
  }
#endif

  #ifdef PLATFORM_TBD
  virtual void disconnect(DeviceIdx device_idx) {
    cleanup(device_idx);
    connected = false;
  }
  #else
  void disconnect(DeviceIdx device_idx) {
    cleanup(device_idx);
    connected = false;
  }
  #endif
  virtual bool probe() = 0;
  virtual uint8_t get_mute_cc() { return 255; }
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) {}
  virtual DeviceMixerCapability *mixer();
#ifdef MCL_HAS_DEVICE_CAPABILITIES
  virtual DeviceStepTrackCapability *step_tracks();
  virtual DeviceExtStepTrackCapability *ext_step_tracks();
  virtual DeviceStepEditCapability *step_edit();
  virtual DeviceParamCapability *params();
  virtual DevicePerfCapability *perf();
  virtual DevicePanelCapability *panel();
#endif
  virtual void triggerTrack(uint8_t track, uint8_t velocity,
                            MidiUartClass *uart_ = nullptr) {
    (void)track;
    (void)velocity;
    (void)uart_;
  }
  void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity,
                  MidiUartClass *uart_ = nullptr);
  void sendNoteOff(uint8_t channel, uint8_t note,
                   MidiUartClass *uart_ = nullptr);
  void sendCC(uint8_t channel, uint8_t cc, uint8_t value,
              MidiUartClass *uart_ = nullptr);
  void sendPolyKeyPressure(uint8_t channel, uint8_t cc, uint8_t value,
                           MidiUartClass *uart_ = nullptr);
  void sendNRPN(uint8_t channel, uint16_t parameter, uint16_t value,
                MidiUartClass *uart_ = nullptr);
  uint8_t *icon() const;
  MCLGIF *gif() const;
  uint8_t *gif_data() const;
};

class NullMidiDevice : public MidiDevice {
public:
  NullMidiDevice();
  virtual bool probe() override { return false; }
};

extern NullMidiDevice null_midi_device;
