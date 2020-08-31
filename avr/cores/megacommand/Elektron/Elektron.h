/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ELEKTRON_H__
#define ELEKTRON_H__

#include "WProgram.h"

#include <inttypes.h>

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
  /** Convert compressed monomachine sysex data into normal 8-bit data. **/
  static uint16_t MNMSysexToData(uint8_t *sysex, uint8_t *data, uint16_t len,
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

  SysexCallback(uint8_t _type = 0) { type = _type; }

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

#endif /* ELEKTRON_H__ */
