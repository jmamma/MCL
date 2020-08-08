/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef GRIDTRACK_H__
#define GRIDTRACK_H__

#define A4_TRACK_TYPE_270 2
#define MD_TRACK_TYPE_270 1
#define EXT_TRACK_TYPE_270 3

#define MD_TRACK_TYPE 4
#define A4_TRACK_TYPE 5
#define EXT_TRACK_TYPE 6

#define EMPTY_TRACK_TYPE 0

#include "GridChain.h"
#include "SeqTrack.h"

class GridTrack_270 {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  char trackName[17];
  GridChain_270 chain;
};

class GridTrack {
public:
  uint8_t active = EMPTY_TRACK_TYPE;
  GridChain chain;
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);

  bool is_active() { return (active != EMPTY_TRACK_TYPE) && (active != 255); }

  uint16_t get_track_size();
  uint32_t get_region() {
    switch (active) {
    case default:
    case MD_TRACK_TYPE:
      return BANK1_MD_TRACKS_START;
      break;
    case EXT_TRACK_TYPE:
    case A4_TRACK_TYPE:
      return BANK1_A4_TRACKS_START;
      break;
    }
  }
  bool load_from_grid(uint8_t column, uint16_t row);
  virtual bool store_in_grid(uint8_t column, uint16_t row);

  bool store_in_mem(uint8_t column, uint32_t region = p_addr_base) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(ptr, this, get_track_size());
    return true;
  }

  bool load_from_mem(uint8_t column) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(this, ptr, get_track_size());
    return true;
  }
  /* Load track from Grid in to sequencer, place in payload to be transmitted to
   * device*/
  virtual void load_immediate(){

  };
  virtual bool is() { return false; }
  void init() {
    chain.length = 16;
    chain.speed = SEQ_SPEED_1X;
  }
};

#endif /* GRIDTRACK_H__ */
