#include "Grid.h"
#include "MCL.h"

void Grid::setup() {}

bool Grid::write_header() {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Writing grid header");

  version = GRID_VERSION;
  hash = 0;

  ret = file.seekSet(0);

  if (!ret) {

    DEBUG_PRINTLN("Seek failed");

    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(GridHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN("Write header failed");
    return false;
  }
  DEBUG_PRINTLN("Write header success");
  return true;
}


bool Grid::new_file(const char *gridname) {
  file.close();

  DEBUG_PRINTLN("Attempting to create grid file");
  DEBUG_PRINTLN(gridname);
  bool ret;
  ret = file.createContiguous(gridname, (uint32_t)GRID_SLOT_BYTES +
                                               (uint32_t)GRID_SLOT_BYTES *
                                                   (uint32_t)GRID_LENGTH *
                                                   (uint32_t)(GRID_WIDTH + 1));

  if (!ret) {
    file.close();
    DEBUG_PRINTLN("Could not extend file");
    return false;
  }
  DEBUG_PRINTLN("extension succeeded, trying to close");
  file.close();

  ret = file.open(gridname, O_RDWR);

  if (!ret) {
    file.close();
    DEBUG_PRINTLN("Could not open file");
    return false;
  }


}

bool Grid::new_grid(const char *gridname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Creating new grid");
  if (!new_file(gridname)) { return false; }

  DEBUG_PRINTLN("Initializing grid.. please wait");
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
      DEBUG_PRINTLN("coud not clear row");
      return false;
    }
  }
  clearLed2();
  ret = file.seekSet(0);

  if (!ret) {
    DEBUG_PRINTLN("Could not seek");
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
  DEBUG_PRINT(" ");
  DEBUG_PRINT(d_col);
  DEBUG_PRINTLN(" ");
  if (s_col < 16 && d_col > 15) {
    return false;
  }
  if (s_col > 15 && d_col < 16) {
    return false;
  }
  EmptyTrack temp_track;
  MDTrack *md_track = (MDTrack *)&temp_track;
  A4Track *a4_track = (A4Track *)&temp_track;
  ExtTrack *ext_track = (ExtTrack *)&temp_track;

  if (s_col < 16) {
    md_track->load_from_grid(s_col, s_row);
    // bit of a hack to keep lfos modulating the same track.
    if (destination_same) {
      if (md_track->machine.trigGroup == s_col) {
        md_track->machine.trigGroup = 255;
      }
      if (md_track->machine.muteGroup == s_col) {
        md_track->machine.muteGroup = 255;
      }
      if (md_track->machine.lfo.destinationTrack == s_col) {
        md_track->machine.lfo.destinationTrack = d_col;
      }
    } else {
      int lfo_dest = md_track->machine.lfo.destinationTrack - s_col;
      int trig_dest = md_track->machine.trigGroup - s_col;
      int mute_dest = md_track->machine.muteGroup - s_col;
      if (range_check(d_col + lfo_dest, 0, 15)) {
        md_track->machine.lfo.destinationTrack = d_col + lfo_dest;
      } else {
        md_track->machine.lfo.destinationTrack = 255;
      }
      if (range_check(d_col + trig_dest, 0, 15)) {
        md_track->machine.trigGroup = d_col + trig_dest;
      } else {
        md_track->machine.trigGroup = 255;
      }
      if (range_check(d_col + mute_dest, 0, 15)) {
        md_track->machine.muteGroup = d_col + mute_dest;
      } else {
        md_track->machine.muteGroup = 255;
      }
    }
    md_track->store_track_in_grid(d_col, d_row);
  } else {
    a4_track->load_from_grid(s_col, s_row);
    a4_track->store_track_in_grid(d_col, d_row);
  }
}

uint8_t Grid::get_slot_model(int column, int row, bool load) {
  GridTrack temp_track;
  temp_track.load_from_grid(column, row);
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
  temp_track.chain.row = row;
  temp_track.chain.loops = 0;

  int32_t offset = get_slot_offset(column, row);

  ret = file.seekSet(offset);

  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("Clear grid failed: ");
    DEBUG_DUMP(row);
    DEBUG_DUMP(column);
    return false;
  }
  // DEBUG_PRINTLN("Writing");
  // DEBUG_DUMP(sizeof(temptrack.active));

  ret = mcl_sd.write_data((uint8_t *)&(temp_track), sizeof(temp_track),
                          &file);
  if (!ret) {
    DEBUG_PRINTLN("Write failed");
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


