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
#include "MCLMemory.h"
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

  ///  bool data:
  ///  when false, only load the GridTrack metadata (active and chain) and reconstruct the correct type
  ///  when true, load the full track including sound and sequence data
  bool load_from_grid(uint8_t column, uint16_t row, bool data = false);
  ///  bool data:
  ///  when false, only save the GridTrack metadata (type and chain)
  ///  when true, save the full track including sound and sequence data
  bool store_in_grid(uint8_t column, uint16_t row, bool data = false);

  ///  caller guarantees that the type is reconstructed correctly
  bool store_in_mem(uint8_t column) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(ptr, &(this->active), get_track_size());
    return true;
  }

  ///  caller guarantees that the type is reconstructed correctly
  bool load_from_mem(uint8_t column) {
    uint32_t pos = get_region() + get_track_size() * (uint32_t)(column);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(&(this->active), ptr, get_track_size());
    return true;
  }

  void init() {
    chain.length = 16;
    chain.speed = SEQ_SPEED_1X;
  }


  /* Load track from Grid in to sequencer, place in payload to be transmitted to device*/
  virtual void load_immediate(uint8_t track_number) {}
  virtual uint16_t get_track_size() { return sizeof(GridTrack); }
  virtual uint32_t get_region() { return BANK1_MD_TRACKS_START; }
  /* Calibrate data members on slot copy */
  virtual void on_copy(int16_t s_col, int16_t d_col, bool destination_same) { } 
  virtual uint8_t get_model() { return EMPTY_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return DEVICE_NULL; }

};

#endif /* GRIDTRACK_H__ */
