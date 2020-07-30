/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRID_H__
#define GRID_H__

#include "GridPages.h"
#include "A4Track.h"

#define GRID_LENGTH 128
#define GRID_WIDTH 20
#define GRID_SLOT_BYTES 4096

#define A4_TRACK_TYPE 2
#define MD_TRACK_TYPE 1
#define EXT_TRACK_TYPE 3
#define EMPTY_TRACK_TYPE 0

class Grid {
public:
  uint8_t get_slot_model(int column, int row, bool load);

  void setup();
  int32_t get_slot_offset(int16_t column, int16_t row);
  int32_t get_header_offset(int16_t row);
  bool copy_slot(int16_t s_col, int16_t s_row, int16_t d_col, int16_t d_row, bool destination_same);
  bool clear_slot(int16_t column, int16_t row, bool update_header = true);
  bool clear_row(int16_t row);
  bool clear_model(int16_t column, uint16_t row);
  //  char *get_slot_kit(int column, int row, bool load, bool scroll);
};

extern Grid grid;

#endif /* GRID_H__ */
