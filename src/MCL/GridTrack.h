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

#define NULL_TRACK_TYPE 128
#define EMPTY_TRACK_TYPE 0

#include "GridLink.h"
#include "MCLMemory.h"
#include "SeqTrack.h"

class Grid;

class GridTrack_270 {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  char trackName[17];
  GridLink_270 link;
};

class GridTrack {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  GridLink link;
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);

  bool is_active() { return (active != EMPTY_TRACK_TYPE) && (active != 255); }
  bool is_ext_track() {
    return (active == EXT_TRACK_TYPE || active == MNM_TRACK_TYPE ||
            active == A4_TRACK_TYPE);
  }

  // load header without data from grid
  bool load_from_grid_512(uint8_t column, uint16_t row, Grid *grid = nullptr);
  bool load_from_grid(uint8_t column, uint16_t row);
  // save header without data to grid
  bool write_grid(void *data, size_t len, uint8_t column, uint16_t row,
                  Grid *grid = nullptr);

  virtual bool store_in_grid(uint8_t column, uint16_t row,
                             SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                             bool online = false, Grid *grid = nullptr);

  ///  caller guarantees that the type is reconstructed correctly
  ///  uploads from the runtime object to BANK1
  bool store_in_mem(uint8_t column) {
    volatile uint8_t *ptr = get_region() + get_track_size() * column;
    memcpy_bank1(ptr, this, get_track_size());
    return true;
  }

  ///  caller guarantees that the type is reconstructed correctly
  ///  downloads from BANK1 to the runtime object
  bool load_from_mem(uint8_t column, size_t size = 0) {
    uint16_t bytes = size ? size : get_track_size();
    volatile uint8_t *ptr = get_region() + get_track_size() * column;
    memcpy_bank1(this, ptr, bytes);
    return true;
  }

  void init() {
    link.length = 16;
    link.speed = SEQ_SPEED_1X;
    link.loops = 0;
  }

  /* Load track from Grid in to sequencer, place in payload to be transmitted to
   * device*/
  void load_link_data(SeqTrack *seq_track);

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {}

  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {}
  virtual void load_immediate_cleared(uint8_t tracknumber,
                                      SeqTrack *seq_track) {
    load_immediate(tracknumber, seq_track);
  }

  virtual bool transition_cache(uint8_t tracknumber, uint8_t slotnumber) {
    return false;
  }
  virtual void transition_send(uint8_t tracknumber, uint8_t slotnumber) {}
  virtual void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                               uint8_t slotnumber);
  virtual void load_seq_data(SeqTrack *seq_track) {}

  virtual void paste_track(uint8_t src_track, uint8_t dest_track,
                           SeqTrack *seq_track) {
    load_immediate(dest_track, seq_track);
  }

  virtual uint16_t get_track_size() { return sizeof(GridTrack); }
  virtual uint16_t get_region_size() { return get_track_size(); }
  uint8_t *get_region() { return BANK1_MD_TRACKS_START; }
  bool is_external() { return get_region() != BANK1_MD_TRACKS_START; }
  /* Calibrate data members on slot copy */
  virtual void on_copy(int16_t s_col, int16_t d_col, bool destination_same) {}
  virtual uint8_t get_model() { return EMPTY_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return DEVICE_NULL; }
  virtual uint8_t get_parent_model() { return NULL_TRACK_TYPE; }
  virtual bool allow_cast_to_parent() { return false; }
};

#endif /* GRIDTRACK_H__ */
