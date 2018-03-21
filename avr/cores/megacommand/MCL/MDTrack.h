/* Justin Mammarella jmamma@gmail.com 2018 */
#include "MCL.h"

#ifndef MDTRACK_H__
#define MDTRACK_H__

#define LOCK_AMOUNT 256

class ParameterLock {
public:
  uint8_t param_number;
  uint8_t value;
  uint8_t step;
};

class KitExtra {
public:
  /** The settings of the reverb effect. f**/
  uint8_t reverb[8];
  /** The settings of the delay effect. **/
  uint8_t delay[8];
  /** The settings of the EQ effect. **/
  uint8_t eq[8];
  /** The settings of the compressor effect. **/
  uint8_t dynamics[8];
  uint32_t swingAmount;
  uint8_t accentAmount;
  uint8_t patternLength;
  uint8_t doubleTempo;
  uint8_t scale;
};

class MDTrack {
public:
  uint8_t active = MD_TRACK_TYPE;
  char kitName[17];
  char trackName[17];
  uint8_t origPosition;
  uint8_t patternOrigPosition;
  uint8_t length;
  uint64_t trigPattern;
  uint64_t accentPattern;
  uint64_t slidePattern;
  uint64_t swingPattern;
  // Machine object for Track Machine Type
  MDMachine machine;
  //
  MDSeqTrackData seq_data;
  // Array to hold parameter locks.
  int arraysize;
  KitExtra kitextra;
  uint8_t param_number[LOCK_AMOUNT];
  int8_t value[LOCK_AMOUNT];
  uint8_t step[LOCK_AMOUNT];

  bool getTrack_from_sysex(int tracknumber, uint8_t column);
  bool placeTrack_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int m);
  bool store_track_in_grid(int track, int32_t column, int32_t row);
};

#endif /* MDTRACK_H__ */
