#ifndef SPSXTRACK_H__
#define SPSXTRACK_H__

#include "DeviceTrack.h"
#include "MCLMemory.h"
#include "MDSeqTrackData.h"
#include "MDTrack.h"

#if !defined(__AVR__)
#include "SPSXSeqTrack.h"
#include "SPSXSeqTrackData.h"
#endif

// Seq data format stored in the track
#define SPSX_SEQ_VERSION_LEGACY 0  // MDSeqTrackData (AVR + rp2040)
#define SPSX_SEQ_VERSION_SPSX   1  // SPSXSeqTrackData (rp2040 only)

class ATTR_PACKED() SPSXTrack : public DeviceTrack {
public:
  // Machine data first — fixed offset for memcmp_sound / get_sound_data_ptr
  MDMachine machine;

  // Seq format discriminator
  uint8_t seq_version;

  // Seq data — union so the size is max(legacy, spsx) on rp2040
  // On AVR only legacy is available
  union ATTR_PACKED() SeqDataUnion {
    MDSeqTrackData legacy;
#if !defined(__AVR__)
    SPSXSeqTrackData spsx;
#endif
  } seq_data;

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
    return seq_version == SPSX_SEQ_VERSION_SPSX;
#else
    return false;
#endif
  }

  void init();
  void clear_track();

  uint16_t calc_latency(uint8_t tracknumber);
  bool transition_cache(uint8_t tracknumber, uint8_t slotnumber);
  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       uint8_t slotnumber);
  void load_seq_data(SeqTrack *seq_track);
  void get_machine_from_kit(uint8_t tracknumber);

  bool store_in_grid(uint8_t column, uint16_t row,
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
  virtual void on_copy(int16_t s_col, int16_t d_col, bool destination_same);
  virtual uint8_t get_model() { return machine.get_model(); }
  virtual uint8_t get_device_type() { return MDSPSX_TRACK_TYPE; }
  virtual uint8_t get_parent_model() { return MD_TRACK_TYPE; }
  virtual bool allow_cast_to_parent() { return true; }

  virtual void *get_sound_data_ptr() { return &machine; }
  virtual size_t get_sound_data_size() { return sizeof(MDMachine); }
  virtual size_t get_sound_cmp_size() { return 27; }
};

#endif /* SPSXTRACK_H__ */
