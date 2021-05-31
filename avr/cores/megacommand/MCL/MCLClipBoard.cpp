/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#include "MCL_impl.h"

#define FILENAME_CLIPBOARD "clipb.tmp"

// Sequencer CLIPBOARD tracks are stored at the end of the GRID + 1.

#define CLIPBOARD_FILE_SIZE                                                    \
  (uint32_t) GRID_SLOT_BYTES +                                                 \
      (uint32_t)GRID_SLOT_BYTES *(uint32_t)(GRID_LENGTH + 1) *                 \
          (uint32_t)(GRID_WIDTH + 1)

bool MCLClipBoard::init() { return true; }

bool MCLClipBoard::open() {
  DEBUG_PRINT_FN();

  SD.chdir("/");
  char *str = FILENAME_CLIPBOARD;
  char grid_filename[sizeof(FILENAME_CLIPBOARD) + 2];
  strcpy(grid_filename, str);
  uint8_t l = strlen(grid_filename);
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grid_filename[l] = '.';
    grid_filename[l + 1] = i + '0';
    grid_filename[l + 2] = '\0';
    if (!SD.exists(grid_filename)) {
      DEBUG_PRINTLN(F("Creating clipboard"));
      if (!grids[i].new_file(grid_filename)) {
        gfx.alert("ERROR", "SD ERROR");
        return false;
      }
    } else {
      if (!grids[i].open_file(grid_filename)) {
        DEBUG_PRINTLN(F("Could not open clipboard"));
        return false;
      }
      else {
        DEBUG_PRINTLN(F("Opened Clipboard"));
      }
    }
  }
  return true;
}

bool MCLClipBoard::close() {
  bool ret = true;
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    ret &= grids[n].file.close();
  }
  return ret;
}

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
  ExtTrack *ext_track = (ExtTrack *)(&temp_track);
  uint8_t grid = 0;

  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }

  if (track < NUM_MD_TRACKS) {
    memcpy(md_track->seq_data.data(), mcl_seq.md_tracks[track].data(),
           sizeof(md_track->seq_data));
    md_track->get_machine_from_kit(track);
    md_track->link.length = mcl_seq.md_tracks[track].length;
    md_track->link.speed = mcl_seq.md_tracks[track].speed;
    ret = grids[grid].write(&temp_track, sizeof(MDTrack), track, GRID_LENGTH);
  }
  else {
    uint8_t n = track - NUM_MD_TRACKS;
    memcpy(ext_track->seq_data.data(), mcl_seq.ext_tracks[n].data(),
           sizeof(ext_track->seq_data));
    ext_track->link.length = mcl_seq.ext_tracks[n].length;
    ext_track->link.speed = mcl_seq.ext_tracks[n].speed;
    ret = grids[grid].write(&temp_track, sizeof(ExtTrack), track, GRID_LENGTH);
  }
  close();
  if (!ret) {
    DEBUG_PRINTLN(F("failed write"));
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
  ExtTrack *ext_track = (ExtTrack *)(&temp_track);

  uint8_t grid = 0;
  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }
  if (source_track < NUM_MD_TRACKS) {
    ret = grids[grid].read(&temp_track, sizeof(MDTrack), source_track, GRID_LENGTH);
  } else {
    ret = grids[grid].read(&temp_track, sizeof(ExtTrack), source_track,
                    GRID_LENGTH);
  }
  if (!ret) {
    DEBUG_PRINTLN(F("failed read"));
    close();
    return false;
  }
  if (source_track < NUM_MD_TRACKS) {
    DEBUG_PRINTLN(F("loading seq track"));
    memcpy(mcl_seq.md_tracks[track].data(), md_track->seq_data.data(),
           sizeof(md_track->seq_data));

    mcl_seq.md_tracks[track].set_length(md_track->link.length);
    mcl_seq.md_tracks[track].set_speed(md_track->link.speed);

    if (md_track->machine.trigGroup == source_track) {
      md_track->machine.trigGroup = 255;
    }
    if (md_track->machine.muteGroup == source_track) {
      md_track->machine.muteGroup = 255;
    }
    if (md_track->machine.lfo.destinationTrack == source_track) {
      md_track->machine.lfo.destinationTrack = track;
    }
    DEBUG_PRINTLN(F("sending seq track"));
    bool send_machine = true;
    bool send_level = true;
    MD.sendMachine(track, &(md_track->machine), send_level, send_machine);
  }
  else {
    memcpy(mcl_seq.ext_tracks[track - NUM_MD_TRACKS].data(), ext_track->seq_data.data(),
           sizeof(ext_track->seq_data));
    mcl_seq.ext_tracks[track - NUM_MD_TRACKS].length = ext_track->link.length;
    mcl_seq.ext_tracks[track - NUM_MD_TRACKS].speed = ext_track->link.speed;
  }
  close();
  return true;
}

bool MCLClipBoard::copy(uint16_t col, uint16_t row, uint16_t w, uint16_t h, uint8_t grid) {
  DEBUG_PRINT_FN();
  uint8_t old_grid = proj.get_grid();
  t_col = col;
  t_row = row;
  t_w = w;
  t_h = h;
  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }
  EmptyTrack temp_track;
  int32_t offset;
  bool ret;
  GridRowHeader header;

  for (int y = 0; y < h; y++) {
    proj.select_grid(grid);
    proj.read_grid_row_header(&header, y + row);
    ret = grids[grid].write_row_header(&header, y + row);
    DEBUG_PRINTLN(header.name);
    for (int x = 0; x < w; x++) {
      ret = proj.read_grid(&temp_track, sizeof(temp_track), x + col, y + row);
      DEBUG_DUMP(temp_track.active);
      if (ret) {
        ret = grids[grid].write(&temp_track, sizeof(temp_track), x + col, y + row);
      }
    }
  }
  close();
  proj.select_grid(old_grid);
}
bool MCLClipBoard::paste(uint16_t col, uint16_t row, uint8_t grid) {
  DEBUG_PRINT_FN();
  uint8_t old_grid = proj.get_grid();
  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
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

  uint8_t track_idx, dev_idx;
  for (int y = 0; y < t_h && y + row < GRID_LENGTH; y++) {
    proj.select_grid(grid);
    proj.read_grid_row_header(&header, y + row);
    if ((strlen(header.name) == 0) || (!header.active) ||
        (t_w == GRID_WIDTH && col == 0)) {
      grids[grid].read_row_header(&header_copy, y + t_row);
      header.active = true;
      strncpy(&(header.name[0]), &(header_copy.name[0]), sizeof(header.name));
    }
    for (int x = 0; x < t_w && x + col < GRID_WIDTH; x++) {

      // track now has full data and correct type
      uint8_t s_col = x + t_col;
      uint8_t d_col = x + col;

      uint8_t slot_n = grid * GRID_WIDTH + d_col;

      grids[grid].read(&empty_track, sizeof(EmptyTrack), s_col, y + t_row);

      DeviceTrack *ptrack = ((DeviceTrack*) &empty_track)->init_track_type(empty_track.active);
      DEBUG_DUMP(ptrack->active);

      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(slot_n, &track_idx, &dev_idx);

      if (gdt != nullptr && gdt->track_type != ptrack->active)
        //Don't allow paste in to unsupported slots
        continue;

      int16_t link_row_offset = ptrack->link.row - t_row;

      uint8_t new_link_row = row + link_row_offset;
      if (new_link_row >= GRID_LENGTH) {
        new_link_row = y + row;
      } else if (new_link_row < 0) {
        new_link_row = y + row;
      }
      ptrack->link.row = new_link_row;
      header.update_model(d_col, ptrack->get_model(),
                          ptrack->get_device_type());
      ptrack->on_copy(s_col, d_col, destination_same);
      ptrack->store_in_grid(d_col, y + row);
    }
    proj.write_grid_row_header(&header, y + row);
  }
  close();
  proj.select_grid(old_grid);
}

MCLClipBoard mcl_clipboard;
