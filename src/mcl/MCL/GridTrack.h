/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef GRIDTRACK_H__
#define GRIDTRACK_H__

#define A4_TRACK_TYPE_270 2
#define MD_TRACK_TYPE_270 1
#define EXT_TRACK_TYPE_270 3

#define A4_TRACK_TYPE 5
#define EXT_TRACK_TYPE 6

#define MNM_TRACK_TYPE 8

#define MD_TRACK_TYPE 4
#define MDFX_TRACK_TYPE 7
#define MDROUTE_TRACK_TYPE 9
#define MDTEMPO_TRACK_TYPE 10
#define MDLFO_TRACK_TYPE 11

#define ARP_TRACK_TYPE 12
#define MD_ARP_TRACK_TYPE 13
#define EXT_ARP_TRACK_TYPE 14

#define GRIDCHAIN_TRACK_TYPE 15
#define PERF_TRACK_TYPE 16
#define MDSPSX_TRACK_TYPE 17
#define TBD_TRACK_TYPE 18
#define TBD_MIDI_TRACK_TYPE 19
#define MD_ROUTE_TRACK_TYPE 20
#define MIDI_TRACK_TYPE 21
#define A4_MIDI_TRACK_TYPE 22
#define MNM_MIDI_TRACK_TYPE 23

#define NULL_TRACK_TYPE 128
#define EMPTY_TRACK_TYPE 0

#include "Grid/GridLink.h"
#include "MCLMemory.h"
#include "Sequencer/SeqTrack.h"

class Grid;
class SeqTrack;

struct GridSlotLabelContext {
  uint8_t model;
  GridColumn column;
#if defined(PLATFORM_TBD)
  GridSlot slot;
  GridRow row;
#endif
};

constexpr uint16_t make_grid_slot_label(char a, char b) {
  return ((uint16_t)(uint8_t)a << 8) | (uint8_t)b;
}

class ATTR_PACKED() GridTrackStorageHeader {
public:
  uint8_t version = 0;
  uint8_t reserved = 0;
  uint8_t active = EMPTY_TRACK_TYPE;
  GridLink link;
};

static_assert(sizeof(GridTrackStorageHeader) == 7,
              "GridTrack storage header layout changed");

class ATTR_PACKED() GridTrack {
public:
  static constexpr uint8_t FLAG_SKIP_SOUND = 1 << 0;
  static constexpr uint16_t STORAGE_HEADER_SIZE =
      sizeof(GridTrackStorageHeader);

  uint8_t version = 0;
  uint8_t reserved = 0;
  uint8_t active = EMPTY_TRACK_TYPE;
  GridLink link;

  size_t _sizeof() const {
        return STORAGE_HEADER_SIZE;
  }
  void* _this() { return &version; }

  bool is_active() { return (active != EMPTY_TRACK_TYPE) && (active != 255); }
  bool is_ext_track() {
    return active == EXT_TRACK_TYPE || active == MNM_TRACK_TYPE ||
           active == A4_TRACK_TYPE || active == TBD_MIDI_TRACK_TYPE ||
           active == MIDI_TRACK_TYPE
#if !defined(__AVR__)
           || active == A4_MIDI_TRACK_TYPE || active == MNM_MIDI_TRACK_TYPE
#endif
        ;
  }

  // load header without data from grid
  bool load_from_grid_512(GridSlot column, GridRow row, Grid *grid = nullptr);
  bool load_from_grid(GridSlot column, GridRow row);
  // save header without data to grid
  bool write_grid(void *data, size_t len, GridSlot column, GridRow row, Grid *grid = nullptr);
  bool storage_version_at_least(uint8_t min_version) const;
  bool load_sound() const { return (link.speed & 0x80) == 0; }
  void set_load_sound(bool enabled) {
    if (enabled) {
      link.speed &= 0x7F;
    } else {
      link.speed |= 0x80;
    }
  }

  virtual bool store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track = nullptr, uint8_t merge = 0, bool online = false, Grid *grid = nullptr);

  ///  caller guarantees that the type is reconstructed correctly
  ///  uploads from the runtime object to BANK1
  bool store_in_mem(GridSlot column);

  ///  caller guarantees that the type is reconstructed correctly
  ///  downloads from BANK1 to the runtime object
  bool load_from_mem(GridSlot column, size_t size = 0);

 void init() {
    link.length = 16;
    link.set_speed(SEQ_SPEED_1X);
    link.loops = 0;
 }

  /* Load track from Grid in to sequencer, place in payload to be transmitted to device*/
  void load_link_data(SeqTrack *seq_track);

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {}

  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {}
  virtual void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) { load_immediate(tracknumber, seq_track); }

  virtual bool transition_cache(uint8_t tracknumber, GridSlot slotnumber) { return false; }
  virtual void transition_send(uint8_t tracknumber, GridSlot slotnumber) {}
  virtual void transition_load(uint8_t tracknumber, SeqTrack* seq_track, GridSlot slotnumber);
#if !defined(__AVR__)
  virtual uint8_t transition_countdown_resolution();
#endif
  virtual void load_seq_data(SeqTrack *seq_track) {}

  virtual void paste_track(uint8_t src_track, uint8_t dest_track, SeqTrack *seq_track) {
     load_immediate(dest_track, seq_track);
  }

  virtual uint16_t get_track_size() { return _sizeof(); }
#if !defined(__AVR__)
  virtual uint16_t get_store_size() { return get_track_size(); }
#endif
  virtual uint16_t get_region_size() { return MEMORY_ALIGN(get_track_size()); }
  virtual uintptr_t get_region() { return BANK1_MD_TRACKS_START; }
  /* Calibrate data members on slot copy */
  virtual void on_copy(GridColumn s_col, GridColumn d_col, bool destination_same) { }
  virtual uint16_t grid_slot_label(GridSlotLabelContext ctx) {
    (void)ctx;
    return 0;
  }
  virtual uint8_t get_model() { return EMPTY_TRACK_TYPE; }
  virtual uint8_t storage_version() const { return 0; }
  virtual void init_defaults() {}

private:
  void stamp_storage_version();
  void repair_loaded_header();
};

#endif /* GRIDTRACK_H__ */
