#include "MCL_impl.h"

void Grid::setup() {}

bool Grid::write_header() {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Writing grid header"));

  version = GRID_VERSION;
  hash = 0;

  ret = file.seekSet(0);

  if (!ret) {

    DEBUG_PRINTLN(F("Seek failed"));

    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(GridHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN(F("Write header failed"));
    return false;
  }
  DEBUG_PRINTLN(F("Write header success"));
  return true;
}

bool Grid::new_file(const char *gridname) {
  file.close();

  DEBUG_PRINTLN(F("Attempting to create grid file"));
  DEBUG_PRINTLN(gridname);
  bool ret;
  // GRID_WIDTH + 1 (because first slot is header)
  // GRID_LENGTH + 2 (first row reserved for header information
  //                 (last row is reserved for tmp space, used by clipboard);
  ret = file.createContiguous(gridname, (uint32_t)GRID_SLOT_BYTES *
                                            (uint32_t)(GRID_LENGTH + 2) *
                                            (uint32_t)(GRID_WIDTH + 1));

  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not extend file"));
    return false;
  }
  DEBUG_PRINTLN(F("extension succeeded, trying to close"));
  file.close();

  ret = file.open(gridname, O_RDWR);

  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not open file"));
    return false;
  }

  return true;
}

bool Grid::new_grid(const char *gridname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Creating new grid"));
  if (!new_file(gridname)) {
    return false;
  }

  DEBUG_PRINTLN(F("Initializing grid.. please wait"));
#ifdef OLED_DISPLAY
  oled_display.drawRect(15, 23, 98, 6, WHITE);
#endif
  // Initialise the project file by filling the grid with blank data.
  uint8_t ledstatus;
  for (int32_t i = 0; i < GRID_LENGTH; i++) {

#ifdef OLED_DISPLAY
    mcl_gui.draw_progress("INITIALIZING", i, GRID_LENGTH);
#endif
    if (i % 2 == 0) {
      if (ledstatus == 0) {
        setLed2();
        ledstatus = 1;
      } else {
        clearLed2();
        ledstatus = 0;
      }
    }

    ret = clear_row(i);
    if (!ret) {
      DEBUG_PRINTLN(F("coud not clear row"));
      return false;
    }
  }
  clearLed2();
  ret = file.seekSet(0);

  if (!ret) {
    DEBUG_PRINTLN(F("Could not seek"));
    return false;
  }

  if (!write_header()) {
    return false;
  }

  file.close();
  return true;
}

bool Grid::copy_slot(int16_t s_col, int16_t s_row, int16_t d_col, int16_t d_row,
                     bool destination_same) {
  DEBUG_PRINT_FN();
  DEBUG_PRINT(s_col);
  DEBUG_PRINT(F(" "));
  DEBUG_PRINT(d_col);
  DEBUG_PRINTLN(F(" "));
  if (s_col < 16 && d_col > 15) {
    return false;
  }
  if (s_col > 15 && d_col < 16) {
    return false;
  }
  // setup a buffer frame for the tracks.
  //
  EmptyTrack empty_track;
  // TODO grid id?
  auto *track = empty_track.load_from_grid(s_col, s_row);
  // at this point, the vtable of ptrack should be repaired
  track->on_copy(s_col, d_col, destination_same);
  track->store_in_grid(d_col, d_row);
}

uint8_t Grid::get_slot_model(int column, int row, bool load) {
  GridTrack temp_track;
  temp_track.load_from_grid(column, row);
  // XXX why active, not the actual model?
  return temp_track.active;
}

bool Grid::clear_slot(int16_t column, int16_t row, bool update_header) {

  bool ret;
  int b;
  GridTrack temp_track;

  if (update_header) {
    GridRowHeader row_header;
    read_row_header(&row_header, row);
    row_header.update_model(column, 0, EMPTY_TRACK_TYPE);
    write_row_header(&row_header, row);
  }

  temp_track.active = EMPTY_TRACK_TYPE;
  temp_track.link.init(row);

  int32_t offset = get_slot_offset(column, row);

  ret = file.seekSet(offset);

  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN(F("Clear grid failed: "));
    DEBUG_DUMP(row);
    DEBUG_DUMP(column);
    return false;
  }
  // DEBUG_PRINTLN("Writing");
  // DEBUG_DUMP(sizeof(temptrack.active));

  ret = mcl_sd.write_data((uint8_t *)&(temp_track), sizeof(temp_track), &file);
  if (!ret) {
    DEBUG_PRINTLN(F("Write failed"));
    return false;
  }
  return true;
}

__attribute__((noinline)) bool Grid::clear_row(int16_t row) {
  GridRowHeader row_header;
  row_header.init();
  for (int x = 0; x < GRID_WIDTH; x++) {
    clear_slot(x, row, false);
  }
  return write_row_header(&row_header, row);
}

int32_t Grid_270::get_slot_offset(int16_t column, int16_t row) {
  int32_t offset = (int32_t)GRID_SLOT_BYTES_270 +
                   (int32_t)((column + 1) + (row * (GRID_WIDTH_270 + 1))) *
                       (int32_t)GRID_SLOT_BYTES_270;
  return offset;
}

int32_t Grid_270::get_row_header_offset(int16_t row) {
  int32_t offset =
      (int32_t)GRID_SLOT_BYTES_270 +
      (int32_t)(0 + (row * (GRID_WIDTH_270 + 1))) * (int32_t)GRID_SLOT_BYTES_270;
  return offset;
}


