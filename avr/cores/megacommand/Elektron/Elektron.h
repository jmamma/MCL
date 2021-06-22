/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ELEKTRON_H__
#define ELEKTRON_H__

#include "WProgram.h"
#include <inttypes.h>
#include "MidiID.h"
#include "MidiSysex.h"
#include "MCLMemory.h"

/** Store the name of a monomachine machine. **/
typedef struct mnm_machine_name_s {
  char name[11];
  uint8_t id;
} mnm_machine_name_t;

/** Store the name of a machinedrum machine. **/
typedef struct md_machine_name_s {
  char name[7];
  uint8_t id;
} md_machine_name_t;

/** Store the name of a parameter for a machine model. **/
typedef struct model_param_name_s {
  char name[4];
  uint8_t id;
} model_param_name_t;

/** Data structure holding the parameter names for a machine model. **/
typedef struct model_to_param_names_s {
  uint8_t model;
  uint16_t offset; // offset of the first param in the lookup table
} model_to_param_names_t;

typedef struct short_machine_name_s {
  char name1[3];
  char name2[3];
  uint8_t id;
} short_machine_name_t;

/**
 * Class grouping various helper functions to convert elektron sysex
 * data. These are deprecated and should be replaced by the elektron
 * data encoders.
 **/
class ElektronHelper {
public:
  /** Convert the sysex into normal data by converting 7 bit values to 8 bit.
   * **/
  static uint16_t ElektronSysexToData(uint8_t *sysex, uint8_t *data,
                                      uint16_t len);
  /** Convert normal 8-bit into 7-bit sysex data. **/
  static uint16_t ElektronDataToSysex(uint8_t *data, uint8_t *sysex,
                                      uint16_t len);
  /** Convert normal 8-bit data into 7-bit compressed elektron monomachine sysex
   * data. **/
  static uint16_t MNMDataToSysex(uint8_t *data, uint8_t *sysex, uint16_t len,
                                 uint16_t maxLen);

  static uint16_t to16Bit7(uint8_t b1, uint8_t b2);
  static uint16_t to16Bit(uint8_t b1, uint8_t b2);
  static uint16_t to16Bit(uint8_t *b);
  static uint32_t to32Bit(uint8_t *b);
  static void from16Bit(uint16_t num, uint8_t *b);
  static void from32Bit(uint32_t num, uint8_t *b);
  static uint64_t to64Bit(uint8_t *b);
  static void from64Bit(uint64_t num, uint8_t *b);

  static bool checkSysexChecksum(uint8_t *data, uint16_t len);
  static bool checkSysexChecksum(MidiClass *midi, uint16_t offset,
                                 uint16_t len);
  static void calculateSysexChecksum(uint8_t *data, uint16_t len);

  /*Checksum calcs different offsets for Analog4+AnalogRYTM*/
  static bool checkSysexChecksumAnalog(uint8_t *data, uint16_t len);
  static bool checkSysexChecksumAnalog(MidiClass *midi, uint16_t offset,
                                       uint16_t len);
  static void calculateSysexChecksumAnalog(uint8_t *data, uint16_t len);
};

class SysexCallback {
public:
  uint8_t type;
  uint8_t value;
  bool received;

  SysexCallback(uint8_t _type = 0) { type = _type; received = false; }

  void onSysexReceived() { received = true; }

  void onStatusResponse(uint8_t _type, uint8_t param) {
    if (type == _type) {
      value = param;
      received = true;
    }
  }

  // timeout in slowclock ticks
  bool waitBlocking(uint16_t timeout) {
    uint16_t start_clock = read_slowclock();
    uint16_t current_clock = start_clock;
    do {
      // MCl Code, trying to replicate main loop

      //    if ((MidiClock.mode == MidiClock.EXTERNAL_UART1 ||
      //         MidiClock.mode == MidiClock.EXTERNAL_UART2)) {
      //     MidiClock.updateClockInterval();
      //   }
      //    GUI.display();
      current_clock = read_slowclock();
      handleIncomingMidi();
    } while(clock_diff(start_clock, current_clock) < timeout && !received);
    return received;
  }
};

/**
 * Standard method prototype for argument-less sysex callbacks.
 **/
typedef void (SysexCallback::*sysex_callback_ptr_t)();
/**
 * Standard method prototype for sysex status callbacks, called with the
 * status type and the status parameter. This is used to get the
 * current kit, current pattern, current global, etc...
 **/
typedef void (SysexCallback::*sysex_status_callback_ptr_t)(uint8_t type,
                                                           uint8_t param);

#include "ElektronDataEncoder.h"
#include "MNMDataEncoder.h"

/// forward declaration
class ElektronDevice;
class SeqTrack;
/// Base class for MIDI-compatible devices
/// Defines basic device description data and driver interfaces.

class GridDeviceTrack {
public:
  uint8_t slot_number;
  uint8_t track_type;
  uint8_t group_type;
  uint8_t mem_slot_idx;
  SeqTrack *seq_track;
  uint8_t get_slot_number() { return slot_number; }
  SeqTrack *get_seq_track() { return seq_track; }
};

#define GROUP_DEV 0
#define GROUP_AUX 1
#define GROUP_TEMPO 2

class GridDevice {
public:
  uint8_t num_tracks;
  uint8_t get_num_tracks() { return num_tracks; }

  GridDeviceTrack tracks[GRID_WIDTH];

  GridDevice() { init(); }

  void init() { num_tracks = 0; }

  void add_track(uint8_t track_idx, uint8_t slot_number, SeqTrack *seq_track, uint8_t track_type, uint8_t group_type = GROUP_DEV, uint8_t mem_slot_idx = 255) {
    tracks[track_idx].slot_number = slot_number;
    tracks[track_idx].seq_track = seq_track;
    tracks[track_idx].track_type = track_type;
    tracks[track_idx].mem_slot_idx = mem_slot_idx;
    if (mem_slot_idx == 255) {
    tracks[track_idx].mem_slot_idx = track_idx;
    }
    tracks[track_idx].group_type = group_type;
    num_tracks++;
  }
};

class MidiDevice {
public:
  bool connected;
  MidiClass* midi;
  MidiUartParent* uart;
  const char* const name;
  const uint8_t id; // Device identifier
  const bool isElektronDevice;

  GridDevice grid_devices[NUM_GRIDS];

  MidiDevice(MidiClass* _midi, const char* _name, const uint8_t _id, const bool _isElektronDevice)
    : name(_name), id(_id), isElektronDevice(_isElektronDevice)
  {
    midi = _midi;
    uart = midi ? midi->uart : nullptr;
    connected = false;
  }

  void cleanup() {
    memset(grid_devices,0, sizeof(GridDevice) * NUM_GRIDS);
  }

  void add_track_to_grid(uint8_t grid_idx, uint8_t track_idx, SeqTrack *seq_track, uint8_t track_type, uint8_t group_type = GROUP_DEV, uint8_t mem_slot_idx = 255) {
    auto *devp = &grid_devices[grid_idx];
    devp->add_track(track_idx, track_idx + grid_idx * GRID_WIDTH, seq_track, track_type, group_type, mem_slot_idx);
  }

  ElektronDevice* asElektronDevice() {
    if (!isElektronDevice) return nullptr;
    return (ElektronDevice*) this;
  }

  virtual void init_grid_devices() {};

  virtual void setup() { };

  virtual void disconnect() { cleanup(); connected = false; }
  virtual bool probe() = 0;
  // 34x42 bitmap icon of the device
  virtual uint8_t *icon() { return nullptr; }
};

/// Base class for Elektron sysex listeners
class ElektronSysexListenerClass : public MidiSysexListenerClass {
public:
  /** Vector storing the onGlobalMessage callbacks (called when a global message
   * is received). **/
  CallbackVector<SysexCallback, 1> onGlobalMessageCallbacks;
  /** Vector storing the onKitMessage callbacks (called when a kit message is
   * received). **/
  CallbackVector<SysexCallback, 1> onKitMessageCallbacks;
  /** Vector storing the onSongMessage callbacks (called when a song messages is
   * received). **/
  CallbackVector<SysexCallback, 1> onSongMessageCallbacks;
  /** Vector storing the onPatternMessage callbacks (called when a pattern
   * message is received). **/
  CallbackVector<SysexCallback, 1> onPatternMessageCallbacks;
  /** Vector storing the onStatusResponse callbacks (when a status response is
   * received). **/
  CallbackVector2<SysexCallback, 1, uint8_t, uint8_t> onStatusResponseCallbacks;

  void addOnStatusResponseCallback(SysexCallback *obj,
                                   sysex_status_callback_ptr_t func) {
    onStatusResponseCallbacks.add(obj, func);
  }
  void removeOnStatusResponseCallback(SysexCallback *obj,
                                      sysex_status_callback_ptr_t func) {
    onStatusResponseCallbacks.remove(obj, func);
  }
  void removeOnStatusResponseCallback(SysexCallback *obj) {
    onStatusResponseCallbacks.remove(obj);
  }

  void addOnGlobalMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onGlobalMessageCallbacks.add(obj, func);
  }
  void removeOnGlobalMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onGlobalMessageCallbacks.remove(obj, func);
  }
  void removeOnGlobalMessageCallback(SysexCallback *obj) {
    onGlobalMessageCallbacks.remove(obj);
  }

  void addOnKitMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onKitMessageCallbacks.add(obj, func);
  }
  void removeOnKitMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onKitMessageCallbacks.remove(obj, func);
  }
  void removeOnKitMessageCallback(SysexCallback *obj) {
    onKitMessageCallbacks.remove(obj);
  }

  void addOnPatternMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onPatternMessageCallbacks.add(obj, func);
  }
  void removeOnPatternMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onPatternMessageCallbacks.remove(obj, func);
  }
  void removeOnPatternMessageCallback(SysexCallback *obj) {
    onPatternMessageCallbacks.remove(obj);
  }

  void addOnSongMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSongMessageCallbacks.add(obj, func);
  }
  void removeOnSongMessageCallback(SysexCallback *obj, sysex_callback_ptr_t func) {
    onSongMessageCallbacks.remove(obj, func);
  }
  void removeOnSongMessageCallback(SysexCallback *obj) {
    onSongMessageCallbacks.remove(obj);
  }

};

enum TrigLEDMode {
  TRIGLED_OVERLAY = 0,
  TRIGLED_STEPEDIT = 1,
  TRIGLED_EXCLUSIVE = 2,
  TRIGLED_EXCLUSIVENDYNAMIC = 3
};

/// sysex constants for constructing data frames
class ElektronSysexProtocol {
public:
  uint8_t* const header;
  const size_t header_size;

  const uint8_t kitrequest_id;
  const uint8_t patternrequest_id;
  const uint8_t songrequest_id;
  const uint8_t globalrequest_id;
  const uint8_t statusrequest_id;

  const uint8_t track_index_request_id;
  const uint8_t kit_index_request_id;
  const uint8_t pattern_index_request_id;
  const uint8_t song_index_request_id;
  const uint8_t global_index_request_id;

  const uint8_t status_set_id;
  const uint8_t tempo_set_id;
  const uint8_t kitname_set_id;
  const uint8_t kitname_length;

  const uint8_t load_global_id;
  const uint8_t load_pattern_id;
  const uint8_t load_kit_id;
  const uint8_t save_kit_id;
};

/// Base class for objects that can be transferred via Elektron sysex protocols.
class ElektronSysexObject {
public:
  virtual uint8_t getPosition() = 0;
  virtual void setPosition(uint8_t) = 0;
  /** Read in a global message from a sysex buffer. **/
  virtual bool fromSysex(uint8_t *sysex, uint16_t len) = 0;
  /** Read in a global message from a sysex buffer. **/
  virtual bool fromSysex(MidiClass *midi) = 0;
  /** Convert the pattern object into a sysex buffer to be sent to the UART. **/
  virtual uint16_t toSysex(uint8_t *sysex, uint16_t len) = 0;
  /** Convert the global object and encode it into a sysex encoder, for example to send directly to the UART.  **/
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder) = 0;
};

#define FW_CAP_LOW(x) (1 << x)
#define FW_CAP_HIGH(x) (FW_CAP_LOW(x + 8))

 //#define FW_CAP_DEBUG        FW_CAP_LOW(0)
#define FW_CAP_TRIG_INTERFACE FW_CAP_LOW(1)
#define FW_CAP_MUTE_STATE     FW_CAP_LOW(2)
#define FW_CAP_SAMPLE         FW_CAP_LOW(3)
#define FW_CAP_TRIG_LEDS      FW_CAP_LOW(4)
#define FW_CAP_KIT_WORKSPACE  FW_CAP_LOW(5)
#define FW_CAP_MASTER_FX      FW_CAP_LOW(6)

#define FW_CAP_UNDOKIT_SYNC   FW_CAP_HIGH(0)
#define FW_CAP_TONAL          FW_CAP_HIGH(1)
#define FW_CAP_ENHANCED_GUI FW_CAP_HIGH(2)
#define FW_CAP_ENHANCED_MIDI FW_CAP_HIGH(3)

/// Base class for Elektron MidiDevice
class ElektronDevice : public MidiDevice {
public:
  const ElektronSysexProtocol sysex_protocol;

  /// Runtime variables
  uint16_t fw_caps;
  /** Stores the current global of the MD, usually set by the MDTask. **/
  uint8_t currentGlobal;
  /** Stores the current kit of the MD, usually set by the MDTask. **/
  uint8_t currentKit;

  uint8_t currentTrack;
  uint8_t currentSynthPage;

  uint8_t currentBank;
  /** Stores the current pattern of the MD, usually set by the MDTask. **/
  uint8_t currentPattern;
  /** Set to true if the kit was loaded (usually set by MDTask). **/
  bool loadedKit;
  /** Set to true if the global was loaded (usually set by MDTask). **/
  bool loadedGlobal;

  ElektronDevice(
      MidiClass* _midi, const char* _name, const uint8_t _id,
      const ElektronSysexProtocol& protocol)
    : MidiDevice(_midi, _name, _id, true), sysex_protocol(protocol) {

      currentGlobal = -1;
      currentKit = -1;
      currentPattern = -1;

      loadedKit = false;
      loadedGlobal = false;
    }

  virtual bool canReadWorkspaceKit() {
    // TODO fw cap for live kit access
    //return fw_caps & FW_CAP
    return false;
  }

  virtual bool canReadKit() {
    // TODO fw cap for live kit access
    //return fw_caps & FW_CAP
    return false;
  }

  virtual ElektronSysexObject* getKit() = 0;
  virtual ElektronSysexObject* getPattern() = 0;
  virtual ElektronSysexObject* getGlobal() = 0;
  virtual ElektronSysexListenerClass* getSysexListener() = 0;

  /**
   * Device-specific kit parameter update routine.
   * This will be called when we want to merge P-locked params' default
   * values back into the kit.
   **/
  virtual void updateKitParams() {}
  /**
   * Device-specific kit parameter send routine.
   * Caller provides a scratchpad buffer (for example, EmptyTrack*).
   * Returns the estimated latency for kit sending.
   **/
  virtual uint16_t sendKitParams(uint8_t* send_mask) { return 0; }
  /**
   * Return a pointer to a program-space string representing the name of the
   *given machine.
   **/
  virtual const char* getMachineName(uint8_t machine) { return nullptr; }

  bool get_fw_caps();

  void activate_encoder_interface(uint8_t *params);
  void deactivate_encoder_interface();

  void activate_enhanced_gui();
  void deactivate_enhanced_gui();

  void activate_enhanced_midi();
  void deactivate_enhanced_midi();

  void set_seq_page(uint8_t page);

  void set_rec_mode(uint8_t mode);

  void popup_text(uint8_t action_string, uint8_t persistent = 0);
  void popup_text(char *str, uint8_t persistent = 0);

  void draw_bank(uint8_t bank);
  void draw_close_bank();

  void draw_close_microtiming();
  void draw_microtiming(uint8_t speed, uint8_t timing);
  void draw_pattern_idx(uint8_t idx, uint8_t idx_other, uint8_t chain_mask);
  void activate_trig_interface();
  void deactivate_trig_interface();

  void activate_track_select();
  void deactivate_track_select();
  void set_trigleds(uint16_t bitmask, TrigLEDMode mode, uint8_t blink = 0);
  void set_key_repeat(uint8_t mode);

  void undokit_sync();
  /**
   * Send a sysex request to the device. All the request calls
   * are wrapped in appropriate methods like requestKit,
   * requestPattern, etc...
   **/
  virtual uint16_t sendRequest(uint8_t *data, uint8_t len, bool send = true, MidiUartParent *uart_ = nullptr);
  virtual uint16_t sendRequest(uint8_t type, uint8_t param, bool send = true);
  /**
   * Wait for a blocking answer to a status request. Timeout is in clock ticks.
   **/
  uint8_t waitBlocking(uint16_t timeout = 1000);

  /* requests */
  /**
   * Request a kit from the machinedrum, which will answer by sending a long
   *sysex message. Register a callback with the MDSysexListener to act on that
   *message.
   **/
  void requestKit(uint8_t kit);
  /**
   * Request a pattern from the machinedrum, which will answer by sending a long
   *sysex message. Register a callback with the MDSysexListener to act on that
   *message.
   **/
  void requestPattern(uint8_t pattern);
  /**
   * Request a song from the machinedrum, which will answer by sending a long
   *sysex message. Register a callback with the MDSysexListener to act on that
   *message.
   **/
  void requestSong(uint8_t song);
  /**
   * Request a global from the machinedrum, which will answer by sending a long
   *sysex message. Register a callback with the MDSysexListener to act on that
   *message.
   **/
  void requestGlobal(uint8_t global);

  /**
   * Get the status answer from the device, blocking until either
   * a message is received or the timeout has run out.
   *
   * This method normally doesn't have to be used, because the
   * standard requests (get kit, get pattern, etc...) are covered by
   * their own methods.
   **/
  uint8_t getBlockingStatus(uint8_t type, uint16_t timeout = 3000);
  /**
   * Get the given kit of the device, blocking for an answer.
   * The sysex message will be stored in the Sysex receive buffer.
   **/
  virtual bool getBlockingKit(uint8_t kit, uint16_t timeout = 3000);
  /**
   * Get the given pattern of the device, blocking for an answer.
   * The sysex message will be stored in the Sysex receive buffer.
   **/
  virtual bool getBlockingPattern(uint8_t pattern, uint16_t timeout = 3000);
  /**
   * Get the given song of the device, blocking for an answer.
   * The sysex message will be stored in the Sysex receive buffer.
   **/
  bool getBlockingSong(uint8_t song, uint16_t timeout = 3000);
  /**
   * Get the given global of the device, blocking for an answer.
   * The sysex message will be stored in the Sysex receive buffer.
   **/
  bool getBlockingGlobal(uint8_t global, uint16_t timeout = 3000);
  /**
   * Get the current kit of the device, blocking for an answer.
   **/
  uint8_t getCurrentKit(uint16_t timeout = 3000);
  /**
   * Get the current pattern of the device, blocking for an answer.
   **/
  uint8_t getCurrentPattern(uint16_t timeout = 3000);

  uint8_t getCurrentTrack(uint16_t timeout = 3000);

  uint8_t getCurrentGlobal(uint16_t timeout = 3000);

  /**
   * Send a sysex message to set the type id to the given value. This
   * is used by more specific methods and there should be no need to
   * use this method directly.
   **/
  virtual void setStatus(uint8_t id, uint8_t value);
  virtual void setKitName(const char* name);
  /**
   * Set the tempo.
   **/
  virtual uint8_t setTempo(float tempo, bool send = true);
  /**
   * Send a sysex message to load the given global.
   **/
  virtual void loadGlobal(uint8_t id);
  /**
   * Send a sysex message to load the given kit.
   **/
  virtual void loadKit(uint8_t kit);
  /**
   * Send a sysex message to load the given pattern.
   **/
  virtual void loadPattern(uint8_t pattern);
  /**
   * Save the current kit at the given position.
   **/
  virtual void saveCurrentKit(uint8_t pos);

};

extern const char* getMachineNameShort(uint8_t machine, uint8_t type, const short_machine_name_t* table, size_t size);
#define copyMachineNameShort(src, dst) \
  (dst)[0] = (src)[0]; \
  (dst)[1] = (src)[1];

#endif /* ELEKTRON_H__ */
