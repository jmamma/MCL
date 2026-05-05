#pragma once

#include "platform.h"
#include <inttypes.h>

#ifdef PLATFORM_TBD
#include "GUI.h"
#endif

class ElektronDevice;
class GridDeviceTrack;
class MCLGIF;
class MidiClass;
class MidiUartClass;

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
  uint8_t port; // Physical port number (UART1_PORT, UART2_PORT, UARTUSB_PORT)

  MidiDevice(MidiClass *_midi, const char *_name, const uint8_t _id,
             const bool _isElektronDevice);

  void add_track_to_grid(uint8_t grid_idx, uint8_t track_idx,
                         GridDeviceTrack *gdt);
  void cleanup(uint8_t device_idx);

  ElektronDevice *asElektronDevice() {
    if (!isElektronDevice) return nullptr;
    return (ElektronDevice *)this;
  }

  virtual void setup_listeners() {}
  virtual void cleanup_listeners() {}

  void setPort(MidiClass *_midi, uint8_t _port = 0);

  virtual void init_grid_devices(uint8_t device_idx) {}

  /** Called when the driver is successfully probed and connected.
   *  Default implementation calls init_grid_devices for backward compatibility.
   */
  virtual void on_connection(uint8_t device_idx) {
    init_grid_devices(device_idx);
  }

#ifdef PLATFORM_TBD
  /** Per-frame UI maintenance for the driver (polling, overlays, etc.). */
  virtual void ui_loop() {}

  /** Driver-specific raw UI event handler. Return true to consume. */
  virtual bool handle_ui_event(gui_event_t *event) {
    (void)event;
    return false;
  }

  /** Returns true if the driver has an active UI overlay or mode. */
  virtual bool is_ui_active() { return false; }
#endif

  virtual void setup() {}

  virtual void disconnect(uint8_t device_idx) {
    cleanup(device_idx);
    connected = false;
  }
  virtual bool probe() = 0;
  virtual uint8_t get_mute_cc() { return 255; }
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) {}
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
  virtual uint8_t *icon() { return nullptr; }
  virtual MCLGIF *gif();
  virtual uint8_t *gif_data();
};
