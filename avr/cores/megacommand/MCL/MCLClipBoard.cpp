/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#include "MCL.h"
#include "MCLClipBoard.h"
#include "Shared.h"

#define FILENAME_CLIPBOARD "clipboard.tmp"

// Sequencer CLIPBOARD tracks are stored at the end of the GRID + 1.

#define CLIPBOARD_FILE_SIZE                                                    \
  (uint32_t) GRID_SLOT_BYTES +                                                 \
      (uint32_t)GRID_SLOT_BYTES *(uint32_t)(GRID_LENGTH + 1) *                 \
          (uint32_t)(GRID_WIDTH + 1)

bool MCLClipBoard::init() {
  DEBUG_PRINTLN("Creating clipboard");
  bool ret = file.createContiguous(FILENAME_CLIPBOARD, CLIPBOARD_FILE_SIZE);
  if (ret) {
    file.close();
  } else {
    DEBUG_PRINTLN("failed to create contiguous file");
    return false;
  }
}

bool MCLClipBoard::open() {
  DEBUG_PRINT_FN();
  if (!file.open(FILENAME_CLIPBOARD, O_RDWR)) {
    init();
    return file.open(FILENAME_CLIPBOARD, O_RDWR);
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

bool MCLClipBoard::close() { return file.close(); }

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
  int32_t offset = grid.get_slot_offset(track, GRID_LENGTH);
  ret = file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("failed seek");
    close();
    return false;
  }
  if (track < NUM_MD_TRACKS) {
    memcpy(&(md_track->seq_data), &mcl_seq.md_tracks[track],
           sizeof(md_track->seq_data));
    md_track->get_machine_from_kit(track, track);
    ret = mcl_sd.write_data(&temp_track, sizeof(MDTrackLight), &file);
  }
#ifdef EXT_TRACKS
  else {
    memcpy(&temp_track, &mcl_seq.ext_tracks[track - NUM_MD_TRACKS],
           sizeof(ExtSeqTrackData));
    ret = mcl_sd.write_data(&temp_track, sizeof(ExtSeqTrack), &file);
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
  int32_t offset = grid.get_slot_offset(source_track, GRID_LENGTH);
  ret = file.seekSet(offset);
  if (ret) {
    if (source_track < NUM_MD_TRACKS) {
      ret = mcl_sd.read_data(&temp_track, sizeof(MDTrackLight), &file);
    } else {
      ret = mcl_sd.read_data(&temp_track, sizeof(ExtSeqTrackData), &file);
    }
    if (!ret) {
      DEBUG_PRINTLN("failed read");
      close();
      return false;
    }
  } else {
    close();
    DEBUG_PRINTLN("failed seek");
    return false;
  }
  if (source_track < NUM_MD_TRACKS) {
    DEBUG_PRINTLN("loading seq track");
    memcpy(&mcl_seq.md_tracks[track], &(md_track->seq_data),
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
    md_track->place_track_in_kit(track, track, &(MD.kit), true);
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

bool MCLClipBoard::copy(uint8_t col, uint8_t row, uint8_t w, uint8_t h) {
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

  for (uint8_t y = 0; y < h; y++) {
    header.read(y + row);
    offset = grid.get_header_offset(y + row);
    ret = file.seekSet(offset);
    ret = mcl_sd.write_data((uint8_t *)(&header), sizeof(GridRowHeader), &file);
    DEBUG_PRINTLN(header.name);
    for (uint8_t x = 0; x < w; x++) {
      offset = grid.get_slot_offset(x + col, y + row);
      ret = proj.file.seekSet(offset);
      ret = mcl_sd.read_data(&temp_track, sizeof(temp_track), &proj.file);
      if (ret) {

        offset = grid.get_slot_offset(x, y);
        ret = file.seekSet(offset);
        ret = mcl_sd.write_data(&temp_track, sizeof(temp_track), &file);
      }
    }
  }
  close();
}
bool MCLClipBoard::paste(uint8_t col, uint8_t row) {
  DEBUG_PRINT_FN();
  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return false;
  }

  bool destination_same = (col == t_col);
  if (t_w == 1) {
    destination_same = true;
  }
  EmptyTrack temp_track;
  MDTrack *md_track = (MDTrack *)&temp_track;
  A4Track *a4_track = (A4Track *)&temp_track;
  ExtTrack *ext_track = (ExtTrack *)&temp_track;

  int32_t offset;
  bool ret;

  GridRowHeader header;
  GridRowHeader header_copy;
  for (uint8_t y = 0; y < t_h && y + row < GRID_LENGTH; y++) {
    header.read(y + row);

    if ((strlen(header.name) == 0) || (!header.active) ||
        (t_w == GRID_WIDTH && col == 0)) {
      offset = grid.get_header_offset(y + t_row);
      ret = file.seekSet(offset);
      ret = mcl_sd.read_data((uint8_t *)(&header_copy), sizeof(GridRowHeader),
                             &file);
      header.active = true;
      strcpy(&(header.name[0]), &(header_copy.name[0]));
    }
    for (uint8_t x = 0; x < t_w && x + col < GRID_WIDTH; x++) {

      offset = grid.get_slot_offset(x, y);
      ret = file.seekSet(offset);
      ret = mcl_sd.read_data(&temp_track, sizeof(temp_track), &file);
      uint8_t s_col = x + t_col;
      uint8_t d_col = x + col;
      switch (temp_track.active) {
      case EMPTY_TRACK_TYPE:
        header.update_model(x + col, EMPTY_TRACK_TYPE, DEVICE_NULL);
        break;

      case EXT_TRACK_TYPE:
        if (x + col >= NUM_MD_TRACKS) {
          header.update_model(x + col, x + col, EXT_TRACK_TYPE);
          offset = grid.get_slot_offset(x + col, y + row);
          ret = proj.file.seekSet(offset);
          ret = mcl_sd.write_data(ext_track, sizeof(ExtTrack), &proj.file);
        }
        break;

      case A4_TRACK_TYPE:
        if (x + col >= NUM_MD_TRACKS) {
          header.update_model(x + col, x + col, A4_TRACK_TYPE);
          offset = grid.get_slot_offset(x + col, y + row);
          ret = proj.file.seekSet(offset);
          ret = mcl_sd.write_data(a4_track, sizeof(A4Track), &proj.file);
        }
        break;

      case MD_TRACK_TYPE:
        if (x + col < NUM_MD_TRACKS) {
          header.update_model(x + col, md_track->machine.model, MD_TRACK_TYPE);
          if ((destination_same)) {
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
            uint8_t lfo_dest = md_track->machine.lfo.destinationTrack - s_col;
            uint8_t trig_dest = md_track->machine.trigGroup - s_col;
            uint8_t mute_dest = md_track->machine.muteGroup - s_col;
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
          offset = grid.get_slot_offset(x + col, y + row);
          ret = proj.file.seekSet(offset);
          ret = mcl_sd.write_data(md_track, sizeof(MDTrack), &proj.file);
        }
        break;
      default:
        break;
      }
    }
    header.write(y + row);
  }
  close();
}

MCLClipBoard mcl_clipboard;
