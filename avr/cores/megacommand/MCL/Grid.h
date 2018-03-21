/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRID_H__
#define GRID_H__

#define GRID_LENGTH 130
#define GRID_WIDTH 22
#define GRID_SLOT_BYTES 4096

#define A4_TRACK_TYPE 2
#define MD_TRACK_TYPE 1
#define EXT_TRACK_TYPE 3
#define EMPTY_TRACK_TYPE 0

class Grid {
public:
  void setup();
  bool clear_slot(uint16_t i);
  bool clear_row(uint16_t i);
  void uint32_t get_slot_model(int column, int row, bool load, A4Track *track_buf);
  char *get_slot_kit(int column, int row, bool load, bool scroll);
};
extern Grid grid;
extern GridPage grid_page;
extern GridSavePage grid_save_page;
extern GridWritePage grid_write_page;

#endif /* GRID_H__ */
