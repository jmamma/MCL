#ifndef SPSXTRACK_H__
#define SPSXTRACK_H__

#include "DeviceTrack.h"
#include "MCLMemory.h"
#include "MDSeqTrackData.h"
#include "MDTrack.h"
#include "SeqTrackModData.h"

#if !defined(__AVR__)
#include "SPSXSeqTrack.h"
#include "SPSXSeqTrackData.h"
#endif

// Seq data format stored in the track
#define SPSX_SEQ_VERSION_LEGACY 0  // MDSeqTrackData (AVR + rp2040)
#define SPSX_SEQ_VERSION_SPSX   1  // SPSXSeqTrackData (rp2040 only)

#if !defined(__AVR__)

class ATTR_PACKED() SPSXTrackSeqStorage {
public:
  uint8_t seq_version;

  union ATTR_PACKED() SeqDataUnion {
    MDSeqTrackData legacy;
#if !defined(__AVR__)
    SPSXSeqTrackData spsx;
#endif
  } seq_data;
};

class ATTR_PACKED() SPSXTrackStorage : public SPSXTrackSeqStorage,
                                       public SeqTrackModStorage {
public:
  void init_storage() {
    seq_version = SPSX_SEQ_VERSION_LEGACY;
    seq_data.legacy.init();
    SeqTrackModStorage::init_mod();
  }
};

static_assert(sizeof(SPSXTrackStorage) ==
                  sizeof(SPSXTrackSeqStorage) + sizeof(SeqTrackModStorage),
              "SPSXTrackStorage storage size changed");

class ATTR_PACKED() SPSXTrack : public DeviceTrack {
public:
  // Machine data first — fixed offset for memcmp_sound / get_sound_data_ptr
  SPSMachine machine;

  SPSXTrackStorage seq_storage;

  SPSXTrack() {
    active = MDSPSX_TRACK_TYPE;
    // NOTE: do NOT set seq_version here — placement new in init_track_type
    // must not wipe the value loaded from BANK1/grid
  }

  size_t _sizeof() const {
    return sizeof(SPSXTrack) - sizeof(void*);
  }

  bool has_spsx_seq() const {
#if !defined(__AVR__)
    return seq_storage.seq_version == SPSX_SEQ_VERSION_SPSX;
#else
    return false;
#endif
  }

  void init();
  void clear_track();

  uint16_t calc_latency(uint8_t tracknumber);
  uint8_t transition_countdown_resolution() override;
  bool transition_cache(uint8_t tracknumber, GridSlot slotnumber);
  void transition_send(uint8_t tracknumber, GridSlot slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber);
  void load_seq_data(SeqTrack *seq_track);
  void get_machine_from_kit(uint8_t tracknumber);

  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr);
  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track);

  void paste_track(uint8_t src_track, uint8_t dest_track, SeqTrack *seq_track);

  void scale_vol(float scale);
  void scale_seq_vol(float scale);
  void normalize();

  virtual uint16_t get_track_size() { return _sizeof(); }
  virtual uint16_t get_region_size() {
    // Region must fit the largest possible variant
    return MEMORY_ALIGN(sizeof(SPSXTrack) - sizeof(void*));
  }
#ifdef MCL_MEMORY_USE_ARRAYS
  virtual uintptr_t get_region() { return BANK1_SPSX_TRACKS_START; }
#else
  virtual uintptr_t get_region() { return BANK1_MD_TRACKS_START; }
#endif
  virtual void on_copy(GridColumn s_col, GridColumn d_col, bool destination_same);
  virtual DeviceTrack *materialize_as(uint8_t track_type,
                                      uint8_t tracknumber,
                                      SeqTrack *seq_track);
  virtual uint8_t get_model() { return machine.get_model(); }
  virtual uint8_t get_parent_model() { return MD_TRACK_TYPE; }
  virtual bool allow_cast_to_parent() { return true; }
  virtual uint8_t storage_version() const { return SEQ_TRACK_MOD_STORAGE_VERSION; }

  virtual void *get_sound_data_ptr() { return &machine; }
  virtual size_t get_sound_data_size() { return sizeof(SPSMachine); }
  virtual size_t get_sound_cmp_size() { return 27; }

private:
};

static_assert(MEMORY_ALIGN(sizeof(SPSXTrack) - sizeof(void*)) <= SPSX_TRACK_LEN,
              "SPSXTrack outgrew SPSX_TRACK_LEN — bump SPSX_TRACK_LEN or shrink");

#endif // !__AVR__

#endif /* SPSXTRACK_H__ */
