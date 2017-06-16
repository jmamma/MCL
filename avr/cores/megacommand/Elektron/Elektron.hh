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
  model_param_name_t *names;
} model_to_param_names_t;

/**
 * Class grouping various helper functions to convert elektron sysex
 * data. These are deprecated and should be replaced by the elektron
 * data encoders.
 **/
class ElektronHelper {
public:
	/** Convert the sysex into normal data by converting 7 bit values to 8 bit. **/
  static uint16_t ElektronSysexToData(uint8_t *sysex, uint8_t *data, uint16_t len);
	/** Convert normal 8-bit into 7-bit sysex data. **/
  static uint16_t ElektronDataToSysex(uint8_t *data, uint8_t *sysex, uint16_t len);
	/** Convert normal 8-bit data into 7-bit compressed elektron monomachine sysex data. **/
  static uint16_t MNMDataToSysex(uint8_t *data, uint8_t *sysex, uint16_t len, uint16_t maxLen);
	/** Convert compressed monomachine sysex data into normal 8-bit data. **/
  static uint16_t MNMSysexToData(uint8_t *sysex, uint8_t *data, uint16_t len, uint16_t maxLen);

  static uint16_t to16Bit7(uint8_t b1, uint8_t b2);
  static uint16_t to16Bit(uint8_t b1, uint8_t b2);
  static uint16_t to16Bit(uint8_t *b);
  static uint32_t to32Bit(uint8_t *b);
  static void from16Bit(uint16_t num, uint8_t *b);
  static void from32Bit(uint32_t num, uint8_t *b);
  static uint64_t to64Bit(uint8_t *b);
  static void from64Bit(uint64_t num, uint8_t *b);
	
	static bool checkSysexChecksum(uint8_t *data, uint16_t len);
	static void calculateSysexChecksum(uint8_t *data, uint16_t len);
};

#include "MNMDataEncoder.hh"
#include "ElektronDataEncoder.hh"

#endif /* ELEKTRON_H__ */
