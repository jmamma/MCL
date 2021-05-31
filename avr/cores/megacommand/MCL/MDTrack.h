/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef MDTRACK_H__
#define MDTRACK_H__

#include "DeviceTrack.h"
#include "DiagnosticPage.h"
#include "MCLMemory.h"
#include "MDSeqTrack.h"
#include "MDSeqTrackData.h"

#define LOCK_AMOUNT 256

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

class MDTrackLight_270 : public GridTrack_270 {
public:
  MDSeqTrackData_270 seq_data;
  MDMachine machine;
};

class MDTrack_270 : public MDTrackLight_270 {
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
};

class MDTrack : public DeviceTrack {
public:
  MDSeqTrackData seq_data;
  MDMachine machine;
  MDTrack() {
    active = MD_TRACK_TYPE;
    static_assert(sizeof(MDTrack) <= GRID1_TRACK_LEN);
  }
  void init();

  void clear_track();
  uint16_t calc_latency(uint8_t tracknumber);
  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber);
  void transition_clear(uint8_t tracknumber, SeqTrack *seq_track) {
    MDSeqTrack *md_seq_track = (MDSeqTrack *)seq_track;
    bool clear_locks = true;
    bool reset_params = false;
    md_seq_track->clear_track(clear_locks, reset_params);
  }

  void load_seq_data(SeqTrack *seq_track);
  void get_machine_from_kit(uint8_t tracknumber);
  bool get_track_from_sysex(uint8_t tracknumber);

  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false);
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);

  // scale machine track vol by percentage
  void scale_vol(float scale);

  // scale vol locks by percentage
  void scale_seq_vol(float scale);

  // normalize track level
  void normalize();

  bool convert(MDTrack_270 *old) {
    link.row = old->link.row;
    link.loops = old->link.loops;
    if (link.row >= GRID_LENGTH) {
      link.row = GRID_LENGTH - 1;
    }
    if (old->active == MD_TRACK_TYPE_270) {
      memcpy(&machine, &old->machine, sizeof(MDMachine));
      if (old->seq_data.speed < 64) {
        link.speed = SEQ_SPEED_1X;
      } else {
        link.speed = old->seq_data.speed - 64;
      }
      link.length = old->seq_data.length;

      seq_data.convert(&(old->seq_data));
      active = MD_TRACK_TYPE;
    } else {
      link.speed = SEQ_SPEED_1X;
      link.length = 16;
      active = EMPTY_TRACK_TYPE;
    }
    return true;
  }

  virtual uint16_t get_track_size() { return sizeof(MDTrack); }
  virtual uint32_t get_region() { return BANK1_MD_TRACKS_START; }
  virtual void on_copy(int16_t s_col, int16_t d_col, bool destination_same);
  virtual uint8_t get_model() { return machine.get_model(); }
  virtual uint8_t get_device_type() { return MD_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return &machine; }
  virtual size_t get_sound_data_size() { return sizeof(MDMachine); }
};

#endif /* MDTRACK_H__ */
