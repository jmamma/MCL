/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ELEKTRON_H__
#define ELEKTRON_H__

#include "WProgram.h"

#include <inttypes.h>
#include "MidiID.h"
#include "MidiSysex.h"

/**
 * \addtogroup Elektron
 *
 * @{
 *
 * \addtogroup elektron_helpers Elektron Helpers
 *
 * @{
 *
 * \file
 * Elektron helper routines and data structures
 **/

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
  const model_param_name_t *names;
} model_to_param_names_t;

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

/// Base class for MIDI-compatible devices
/// Defines basic device description data and driver interfaces.
class MidiDevice {
public:
  bool connected;
  MidiClass* midi;
  MidiUartParent* uart;
  const char* const name;
  const uint8_t id; // Device identifier
  const uint8_t* const icon;

  MidiDevice(MidiClass* _midi, const char* _name, const uint8_t _id, const uint8_t* _icon)
    : name(_name), id(_id), icon(_icon)
  {
    midi = _midi;
    uart = midi->uart;
    connected = false;
  }

  virtual void disconnect() { connected = false; }
  virtual bool probe() = 0;
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
  TRIGLED_EXCLUSIVE = 2
};

/// sysex constants for constructing data frames
class ElektronSysexProtocol {
public:
  const uint8_t* const header;
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
};

/// Base class for objects that can be transferred via Elektron sysex protocols.
class ElektronSysexObject {
public:
  /** Read in a global message from a sysex buffer. **/
  virtual bool fromSysex(uint8_t *sysex, uint16_t len) = 0;
  /** Read in a global message from a sysex buffer. **/
  virtual bool fromSysex(MidiClass *midi) = 0;
  /** Convert the pattern object into a sysex buffer to be sent to the UART. **/
  virtual uint16_t toSysex(uint8_t *sysex, uint16_t len) = 0;
  /** Convert the global object and encode it into a sysex encoder, for example to send directly to the UART.  **/
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder) = 0;
};

/// Base class for Elektron MidiDevice
class ElektronDevice : public MidiDevice {
public:
  const ElektronSysexProtocol sysex_protocol;

  /// Runtime variables
  uint64_t fw_caps;
  /** Stores the current global of the MD, usually set by the MDTask. **/
  int currentGlobal;
  /** Stores the current kit of the MD, usually set by the MDTask. **/
  int currentKit;
  int currentTrack;
  /** Stores the current pattern of the MD, usually set by the MDTask. **/
  int currentPattern;
  /** Set to true if the kit was loaded (usually set by MDTask). **/
  bool loadedKit;
  /** Set to true if the global was loaded (usually set by MDTask). **/
  bool loadedGlobal;

  ElektronDevice(
      MidiClass* _midi, const char* _name, const uint8_t _id, const uint8_t* _icon,
      const ElektronSysexProtocol& protocol)
    : MidiDevice(_midi, _name, _id, _icon), sysex_protocol(protocol) {

      currentGlobal = -1;
      currentKit = -1;
      currentPattern = -1;

      loadedKit = false;
      loadedGlobal = false;
    }

  virtual ElektronSysexObject* getKit() = 0;
  virtual ElektronSysexObject* getPattern() = 0;
  virtual ElektronSysexObject* getGlobal() = 0;
  virtual ElektronSysexListenerClass* getSysexListener() = 0;

  bool get_fw_caps();

  void activate_trig_interface();
  void deactivate_trig_interface();

  void activate_track_select();
  void deactivate_track_select();
  void set_trigleds(uint16_t bitmask, TrigLEDMode mode);

  /**
   * Send a sysex request to the device. All the request calls
   * are wrapped in appropriate methods like requestKit,
   * requestPattern, etc...
   **/
  virtual void sendRequest(uint8_t *data, uint8_t len);
  virtual void sendRequest(uint8_t type, uint8_t param);
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
  bool getBlockingKit(uint8_t kit, uint16_t timeout = 3000);
  /**
   * Get the given pattern of the device, blocking for an answer.
   * The sysex message will be stored in the Sysex receive buffer.
   **/
  bool getBlockingPattern(uint8_t pattern, uint16_t timeout = 3000);
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

  /* @} */
  uint8_t getCurrentTrack(uint16_t timeout = 3000);

  uint8_t getCurrentGlobal(uint16_t timeout = 3000);

};

#endif /* ELEKTRON_H__ */
