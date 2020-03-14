/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef MDTRACK_H__
#define MDTRACK_H__

#include "Bank1Object.h"
#include "Grid.h"
#include "GridTrack.h"
#include "MCLMemory.h"
#include "MD.h"
#include "MDSeqTrack.h"
#include "MDSeqTrackData.h"

#define LOCK_AMOUNT 256
#define MD_TRACK_TYPE 1

#define SAVE_SEQ 0
#define SAVE_MD 1
#define SAVE_MERGE 2

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

  uint64_t accentPattern;
  uint64_t slidePattern;
  uint64_t swingPattern;

  uint32_t accentEditAll;
  uint32_t slideEditAll;
  uint32_t swingEditAll;
};

class MDTrackLight
    : public GridTrack,
      public Bank1Object<MDTrackLight, 0, BANK1_MD_TRACKS_START> {
public:
  MDSeqTrackData seq_data;
  MDMachine machine;
};

class MDTrack : public MDTrackLight {
public:
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

  MDTrack() {
  arraysize = 0;
  }

  void init();

  void clear_track();

  void place_track_in_kit(uint8_t tracknumber, uint8_t column, MDKit *kit,
                          bool levels = true);
  void load_seq_data(uint8_t tracknumber);
  void place_track_in_pattern(uint8_t tracknumber, uint8_t column,
                              MDPattern *pattern);
  void get_machine_from_kit(uint8_t tracknumber, uint8_t column);
  bool get_track_from_kit(uint8_t tracknumber, uint8_t column);
  bool get_track_from_pattern(uint8_t tracknumber, uint8_t column);
  bool get_track_from_sysex(uint8_t tracknumber, uint8_t column);
  void place_track_in_sysex(uint8_t tracknumber, uint8_t column);
  bool load_track_from_grid(uint8_t column, uint8_t row, int32_t len);
  bool load_track_from_grid(uint8_t column, uint8_t row);

  bool store_track_in_grid(uint8_t column, uint8_t row, uint8_t track = 255,
                           bool storepattern = false, uint8_t merge = 0,
                           bool online = false);

  // scale machine track vol by percentage
  void scale_vol(float scale);

  // scale vol locks by percentage
  void scale_seq_vol(float scale);

  // normalize track level
  void normalize();
};

#endif /* MDTRACK_H__ */
