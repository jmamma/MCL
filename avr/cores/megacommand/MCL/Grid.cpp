#include "Grid.h"
#include "MCL.h"

void Grid::setup() {}
/*
char* Grid::get_slot_kit(int column, int row, bool load, bool scroll) {

  A4Track track_buf;
  if (grid_page.grid_models[column] == EMPTY_TRACK_TYPE) {
    return "----";
  }


  uint8_t char_position = 0;
  //if ((slowclock % 50) == 0) { row_name_offset++; }
  if (row_name_offset > 15) {
    row_name_offset = 0;
  }
  if (scroll) {

    for (uint8_t c = 0; c < 4; c++) {

      if (c + (uint8_t)row_name_offset > 15) {
        char_position =  c + (uint8_t)row_name_offset - 16;
      }
      else {
        char_position =  c + (uint8_t)row_name_offset;
      }
      //  char some_string[] = "hello my baby";
      //row_name[c] = some_string[char_position];
      if (char_position < 5) {
        row_name[c] = ' ';
      }
      else {
        row_name[c] = grid_page.currentkitName[char_position - 5];
      }
    }
    row_name[4] = '\0';

  }
  else {
    for (uint8_t a = 0; a < 16; a++) {
      row_name[a] = grid_page.currentkitName[a];
    }
    row_name[16] = '\0';

  }

  return row_name;

}
*/
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
    md_track->load_track_from_grid(s_col, s_row);
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
    a4_track->load_track_from_grid(s_col, s_row);
    a4_track->store_track_in_grid(d_col, d_row);
  }
}

uint8_t Grid::get_slot_model(int column, int row, bool load) {
  EmptyTrack temp_track;
  MDTrack *md_track = (MDTrack *)&temp_track;
  A4Track *a4_track = (A4Track *)&temp_track;
  ExtTrack *ext_track = (ExtTrack *)&temp_track;
  int32_t len = sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);
  if (column < 16) {
    if (load == true) {
      if (!md_track->load_track_from_grid(column, row, len)) {
        return NULL;
      }
    }
    if (md_track->active == EMPTY_TRACK_TYPE) {
      return NULL;
    } else {
      return md_track->machine.model;
    }
  }

  else {
    if (load == true) {
      if (!a4_track->load_track_from_grid(column, row, 50)) {
        return NULL;
      }
    }
    return md_track->active;
  }
}

int32_t Grid::get_slot_offset(int16_t column, int16_t row) {
  int32_t offset = (int32_t)GRID_SLOT_BYTES +
                   (int32_t)((column + 1) + (row * (GRID_WIDTH + 1))) *
                       (int32_t)GRID_SLOT_BYTES;
  return offset;
}

int32_t Grid::get_header_offset(int16_t row) {
  int32_t offset =
      (int32_t)GRID_SLOT_BYTES +
      (int32_t)(0 + (row * (GRID_WIDTH + 1))) * (int32_t)GRID_SLOT_BYTES;
  return offset;
}

bool Grid::clear_slot(int16_t column, int16_t row, bool update_header) {

  bool ret;
  int b;
  GridTrack temp_track;

  if (update_header) {
    GridRowHeader row_header;
    row_header.read(row);
    row_header.update_model(column, EMPTY_TRACK_TYPE, DEVICE_NULL);
    row_header.write(row);
  }

  temp_track.active = EMPTY_TRACK_TYPE;
  temp_track.chain.row = row;
  temp_track.chain.loops = 0;

  int32_t offset = get_slot_offset(column, row);

  ret = proj.file.seekSet(offset);

  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("Clear grid failed: ");
    DEBUG_PRINTLN(row);
    DEBUG_PRINTLN(column);
    return false;
  }
  // DEBUG_PRINTLN("Writing");
  // DEBUG_PRINTLN(sizeof(temptrack.active));

  ret = mcl_sd.write_data((uint8_t *)&(temp_track), sizeof(temp_track),
                          &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("Write failed");
    return false;
  }
  return true;
}

bool Grid::clear_row(int16_t row) {
  bool update_header = false;
  GridRowHeader row_header;
  row_header.init();
  for (int x = 0; x < GRID_WIDTH; x++) {
    clear_slot(x, row, update_header);
  }
  return row_header.write(row);
}

Grid grid;
