/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef MDTRACK_H__
#define MDTRACK_H__

#include "DeviceTrack.h"
#include "GUI/Pages/DiagnosticPage.h"
#include "MCLMemory.h"
#include "MDSeqTrack.h"
#include "MDSeqTrackData.h"
#include "Sequencer/SeqTrackModData.h"

#define LOCK_AMOUNT 256

#define SAVE_SEQ 0
#define SAVE_MD_PATTERN_IMPORT 1
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

class ATTR_PACKED() MDTrack : public DeviceTrack {
public:
  MDMachine machine;
  SeqTrackModData mod_data;
  MDSeqTrackData seq_data;
  TrackLoadFadeData load_fade;
  MDTrack() {
    active = MD_TRACK_TYPE;
  }
  size_t _sizeof() const {
    return sizeof(MDTrack) - sizeof(void*);
  }
  void init();
  void init_defaults() override {
    machine.init();
    mod_data.init();
    seq_data.init();
    load_fade.init();
  }
  void clear_track();
  uint16_t calc_latency(uint8_t tracknumber) override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) override;
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  void load_seq_data(SeqTrack *seq_track) override;
  void get_machine_from_kit(uint8_t tracknumber);
  bool get_track_from_sysex(uint8_t tracknumber);

  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr) override;
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;

  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;

  void paste_track(uint8_t src_track, uint8_t dest_track, SeqTrack *seq_track) override;
#if !defined(__AVR__)
  // scale machine track vol by percentage
  void scale_vol(float scale);
#endif

  // scale vol locks by percentage
#if defined(__AVR__)
  void scale_seq_vol(uint8_t scale);
#else
  void scale_seq_vol(float scale);
#endif

  // normalize track level
  void normalize();

  uint16_t get_track_size() override { return _sizeof(); }
  uintptr_t get_region() override { return BANK1_MD_TRACKS_START; }
  void on_copy(GridColumn s_col, GridColumn d_col, bool destination_same) override;
  uint16_t grid_slot_label(GridSlotLabelContext ctx) override;
#if !defined(__AVR__)
  bool can_materialize_as(uint8_t track_type) override;
  DeviceTrack *materialize_as(uint8_t track_type,
                              uint8_t tracknumber,
                              SeqTrack *seq_track) override;
#endif
  uint8_t get_model() override { return machine.get_model(); }
  uint8_t storage_version() const override { return SEQ_TRACK_LOAD_FADE_STORAGE_VERSION; }
  TrackLoadFadeData *load_fade_data() override { return &load_fade; }
  const TrackLoadFadeData *load_fade_data() const override { return &load_fade; }

  void *get_sound_data_ptr() override { return &machine; }
  size_t get_sound_data_size() override { return sizeof(MDMachine); }
  virtual size_t get_sound_cmp_size() { return 27; } //params,track,level,model

private:
};

class MDTrackChunk : public DeviceTrackChunk {
public:

  uint16_t get_seq_data_size() override { return sizeof(MDSeqTrackData); }
  uint8_t get_model() override { return MD_TRACK_TYPE; }
  uint16_t get_track_size() override { return GRID1_TRACK_LEN; }
  uintptr_t get_region() override { return BANK1_MD_TRACKS_START; }
  void *get_sound_data_ptr() override { return nullptr; }
  size_t get_sound_data_size() override { return 0; }
};

static_assert(MEMORY_ALIGN(sizeof(MDTrack) - sizeof(void*)) <= GRID1_TRACK_LEN,
              "MDTrack outgrew GRID1_TRACK_LEN — on-disk row format changed, bump GRID_VERSION");

#endif /* MDTRACK_H__ */
