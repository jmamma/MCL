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

#define EMPTY_TRACK_TYPE 0

#include "GridLink.h"
#include "MCLMemory.h"
#include "SeqTrack.h"

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
  bool is_ext_track() { return (active == EXT_TRACK_TYPE || active == MNM_TRACK_TYPE || active == A4_TRACK_TYPE); }

  // load header without data from grid
  bool load_from_grid(uint8_t column, uint16_t row);
  // save header without data to grid
  virtual bool store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track = nullptr, uint8_t merge = 0, bool online = false);

  ///  caller guarantees that the type is reconstructed correctly
  ///  uploads from the runtime object to BANK1
  bool store_in_mem(uint8_t column) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(ptr, this, get_track_size());
    return true;
  }

  ///  caller guarantees that the type is reconstructed correctly
  ///  downloads from BANK1 to the runtime object
  bool load_from_mem(uint8_t column) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(this, ptr, get_track_size());
    return true;
  }

  void init() {
    link.length = 16;
    link.speed = SEQ_SPEED_1X;
  }

  /* Load track from Grid in to sequencer, place in payload to be transmitted to device*/
  void load_link_data(SeqTrack *seq_track);

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {}
  virtual void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {}
  virtual void transition_send(uint8_t tracknumber, uint8_t slotnumber) {}
  virtual void transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber);

  virtual void transition_clear(uint8_t tracknumber, SeqTrack* seq_track) {}

  virtual uint16_t get_track_size() { return sizeof(GridTrack); }
  virtual uint32_t get_region() { return BANK1_MD_TRACKS_START; }
  bool is_external() { return get_region() != BANK1_MD_TRACKS_START; }
  /* Calibrate data members on slot copy */
  virtual void on_copy(int16_t s_col, int16_t d_col, bool destination_same) { }
  virtual uint8_t get_model() { return EMPTY_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return DEVICE_NULL; }

};

#endif /* GRIDTRACK_H__ */
