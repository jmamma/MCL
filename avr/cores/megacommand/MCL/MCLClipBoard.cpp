/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#include "MCL.h"
#include "MCLClipBoard.h"
#include "Shared.h"

#define FILENAME_CLIPBOARD "clipboard.tmp"

bool MCLClipBoard::init() {
  DEBUG_PRINTLN("Creating clipboard");
  bool ret = file.createContiguous(FILENAME_CLIPBOARD,
                                   (uint32_t)GRID_SLOT_BYTES +
                                       (uint32_t)GRID_SLOT_BYTES *
                                           (uint32_t)GRID_LENGTH *
                                           (uint32_t)(GRID_WIDTH + 1));
  if (ret) {
    file.close();
  } else {
    DEBUG_PRINTLN("failed to create contiguous file");
    return false;
  }
}

bool MCLClipBoard::open() {
  if (!file.open(FILENAME_CLIPBOARD, O_RDWR)) {
    init();
    return file.open(FILENAME_CLIPBOARD, O_RDWR);
  }
}
bool MCLClipBoard::close() { return file.close(); }

bool MCLClipBoard::copy(int col, int row, int w, int h) {
  t_col = col;
  t_row = row;
  t_w = w;
  t_h = h;
  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return;
  }
  EmptyTrack temp_track;
  int32_t offset;
  bool ret;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
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
bool MCLClipBoard::paste(int col, int row) {
  if (!open()) {
    DEBUG_PRINTLN("error could not open clipboard");
    return;
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
  for (int y = 0; y < t_h; y++) {
    if (y + row < GRID_LENGTH - 1) {
      header.read(y + row);
      for (int x = 0; x < t_w; x++) {

        offset = grid.get_slot_offset(x, y);
        ret = file.seekSet(offset);
        ret = mcl_sd.read_data(&temp_track, sizeof(temp_track), &file);
        uint8_t s_col = x + t_col;
        uint8_t d_col = x + col;
        if (x + col < GRID_WIDTH - 1) {
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
              header.update_model(x + col, md_track->machine.model,
                                  MD_TRACK_TYPE);
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
              offset = grid.get_slot_offset(x + col, y + row);
              ret = proj.file.seekSet(offset);
              ret = mcl_sd.write_data(md_track, sizeof(MDTrack), &proj.file);
            }
            break;
          default:
            break;
          }
        }
      }
      header.write(y + row);
    }
  }
  close();
}

MCLClipBoard mcl_clipboard;
