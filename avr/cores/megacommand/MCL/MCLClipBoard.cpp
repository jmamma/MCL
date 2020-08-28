/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#include "MCL.h"
#include "MCLClipBoard.h"
#include "Shared.h"

#define FILENAME_CLIPBOARD "/clipboard.tmp"

// Sequencer CLIPBOARD tracks are stored at the end of the GRID + 1.

#define CLIPBOARD_FILE_SIZE                                                    \
  (uint32_t) GRID_SLOT_BYTES +                                                 \
      (uint32_t)GRID_SLOT_BYTES *(uint32_t)(GRID_LENGTH + 1) *                 \
          (uint32_t)(GRID_WIDTH + 1)

bool MCLClipBoard::init() {
  DEBUG_PRINTLN("Creating clipboard");
  return grid.new_file(FILENAME_CLIPBOARD);
}

bool MCLClipBoard::open() {
  DEBUG_PRINT_FN();
  if (!grid.file.open(FILENAME_CLIPBOARD, O_RDWR)) {
    init();
    return grid.file.open(FILENAME_CLIPBOARD, O_RDWR);
  }
  return true;
  /*
    if (file.fileSize() < CLIPBOARD_FILE_SIZE) {
      file.close();
      SD.remove(FILENAME_CLIPBOARD);
      DEBUG_PRINTLN("clipboard file size too small deleting");
      init();
      return file.open(FILENAME_CLIPBOARD, O_RDWR);
    } */
}

bool MCLClipBoard::close() { return grid.file.close(); }

bool MCLClipBoard::copy_sequencer(uint8_t offset) {
  if (offset < NUM_MD_TRACKS) {
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
      if (!copy_sequencer_track(n)) {
        return false;
      }
    }
  } else {
    for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
      if (!copy_sequencer_track(n + NUM_MD_TRACKS)) {
        return false;
      }
    }
  }
  return true;
}

bool MCLClipBoard::copy_sequencer_track(uint8_t track) {
  DEBUG_PRINT_FN();
  bool ret;
  EmptyTrack temp_track;

  MDTrack *md_track = (MDTrack *)(&temp_track);
  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return false;
  }

  if (track < NUM_MD_TRACKS) {
    memcpy(md_track->seq_data.data(), mcl_seq.md_tracks[track].data(),
           sizeof(md_track->seq_data));
    md_track->get_machine_from_kit(track);
    ret = grid.write(&temp_track, sizeof(MDTrack), track, GRID_LENGTH);
  }
#ifdef EXT_TRACKS
  else {
    memcpy(&temp_track, &mcl_seq.ext_tracks[track - NUM_MD_TRACKS],
           sizeof(ExtSeqTrackData));
    ret = grid.write(&temp_track, sizeof(ExtSeqTrack), track, GRID_LENGTH);
  }
#endif
  close();
  if (!ret) {
    DEBUG_PRINTLN("failed write");
  }
  return ret;
}

bool MCLClipBoard::paste_sequencer(uint8_t offset) {
  if (offset < NUM_MD_TRACKS) {
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
      if (!paste_sequencer_track(n, n)) {
        return false;
      }
    }
  } else {
    for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
      if (!paste_sequencer_track(n + offset, n + offset)) {
        return false;
      }
    }
  }
  return true;
}

bool MCLClipBoard::paste_sequencer_track(uint8_t source_track, uint8_t track) {
  DEBUG_PRINT_FN();
  bool ret;

  EmptyTrack temp_track;
  MDTrack *md_track = (MDTrack *)(&temp_track);

  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return false;
  }
    if (source_track < NUM_MD_TRACKS) {
      ret = grid.read(&temp_track, sizeof(MDTrack), source_track, GRID_LENGTH);
    } else {
      ret = grid.read(&temp_track, sizeof(ExtSeqTrackData), source_track, GRID_LENGTH);
    }
    if (!ret) {
      DEBUG_PRINTLN("failed read");
      close();
      return false;
    }
  if (source_track < NUM_MD_TRACKS) {
    DEBUG_PRINTLN("loading seq track");
    memcpy(mcl_seq.md_tracks[track].data(), md_track->seq_data.data(),
           sizeof(md_track->seq_data));
    if (md_track->machine.trigGroup == source_track) {
      md_track->machine.trigGroup = 255;
    }
    if (md_track->machine.muteGroup == source_track) {
      md_track->machine.muteGroup = 255;
    }
    if (md_track->machine.lfo.destinationTrack == source_track) {
      md_track->machine.lfo.destinationTrack = track;
    }
    DEBUG_PRINTLN("sending seq track");
    md_track->place_track_in_kit(track, &(MD.kit), true);
    MD.setMachine(track, &(md_track->machine));
  }
#ifdef EXT_TRACKS
  else if (track >= NUM_MD_TRACKS) {
    memcpy(&mcl_seq.ext_tracks[track - NUM_MD_TRACKS], &(temp_track),
           sizeof(ExtSeqTrackData));
  }
#endif
  close();
  return true;
}

bool MCLClipBoard::copy(uint16_t col, uint16_t row, uint16_t w, uint16_t h) {
  DEBUG_PRINT_FN();
  t_col = col;
  t_row = row;
  t_w = w;
  t_h = h;
  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return false;
  }
  EmptyTrack temp_track;
  int32_t offset;
  bool ret;
  GridRowHeader header;

  for (int y = 0; y < h; y++) {
    proj.read_grid_row_header(&header, y + row);
    ret = grid.write_row_header(&header, y + row);
    DEBUG_PRINTLN(header.name);
    for (int x = 0; x < w; x++) {
      ret = proj.read_grid(&temp_track, sizeof(temp_track), x + col, y + row);
      if (ret) {
        ret = grid.write(&temp_track, sizeof(temp_track), x, y);
      }
    }
  }
  close();
}
bool MCLClipBoard::paste(uint16_t col, uint16_t row) {
  DEBUG_PRINT_FN();
  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return false;
  }

  bool destination_same = (col == t_col);
  if (t_w == 1) {
    destination_same = true;
  }

  // setup buffer frame
  EmptyTrack empty_track;

  GridRowHeader header;
  GridRowHeader header_copy;
  for (int y = 0; y < t_h && y + row < GRID_LENGTH; y++) {
    proj.read_grid_row_header(&header, y + row);

    if ((strlen(header.name) == 0) || (!header.active) ||
        (t_w == GRID_WIDTH && col == 0)) {
      grid.read_row_header(&header_copy, y + t_row);
      header.active = true;
      strcpy(&(header.name[0]), &(header_copy.name[0]));
    }
    for (int x = 0; x < t_w && x + col < GRID_WIDTH; x++) {

      empty_track.load_from_grid(x, y);

      // track now has full data and correct type
      uint8_t s_col = x + t_col;
      uint8_t d_col = x + col;

      int16_t chain_row_offset = empty_track.chain.row - t_row;

      uint8_t new_chain_row = row + chain_row_offset;
      if (new_chain_row >= GRID_LENGTH) { new_chain_row = y + row; }
      else if (new_chain_row < 0) { new_chain_row = y + row; }
      empty_track.chain.row = new_chain_row;
      header.update_model(x + col, empty_track.get_model(), empty_track.get_device_type());
      empty_track.on_copy(s_col, d_col, destination_same);
      empty_track.store_in_grid(x + col, y + row);
    }
    proj.write_grid_row_header(&header, y + row);
  }
  close();
}

MCLClipBoard mcl_clipboard;
