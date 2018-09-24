/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef MDTRACK_H__
#define MDTRACK_H__

#include "Grid.h"
#include "GridTrack.h"
#include "MD.h"
#include "MDSeqTrack.h"

#define LOCK_AMOUNT 256
#define MD_TRACK_TYPE 1

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

class MDTrackLight : public GridTrack {
 public:
};

class MDTrack : public GridTrack {
public:
  MDSeqTrackData seq_data;
  MDMachine machine;

  uint8_t origPosition;
  uint8_t patternOrigPosition;
  uint8_t length;
  uint64_t trigPattern;
  uint64_t accentPattern;
  uint64_t slidePattern;
  uint64_t swingPattern;

  KitExtra kitextra;

  int arraysize;
  ParameterLock locks[LOCK_AMOUNT];
 
  void place_track_in_kit(int tracknumber, uint8_t column, MDKit *kit);
  void load_seq_data(int tracknumber);
  void place_track_in_pattern(int tracknumber, uint8_t column,
                              MDPattern *pattern);

  bool get_track_from_sysex(int tracknumber, uint8_t column);
  void place_track_in_sysex(int tracknumber, uint8_t column);
  bool load_track_from_grid(int32_t column, int32_t row, int32_t len);
  bool load_track_from_grid(int32_t column, int32_t row);
  bool store_track_in_grid(int track, int32_t column, int32_t row);

  //Store/retrieve portion of track object in mem bank2
  bool store_in_mem(uint8_t column);
  bool load_from_mem(uint8_t column);
};

#endif /* MDTRACK_H__ */
