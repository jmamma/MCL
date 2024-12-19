/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#include "MCLClipBoard.h"
#include "MCLSd.h"
#include "Project.h"
#include "EmptyTrack.h"
#include "DeviceTrack.h"
#include "MDTrack.h"
#include "MCLGUI.h"
#include "MCLActions.h"
#include "MidiActivePeering.h"
#include "SeqPages.h"

// Sequencer CLIPBOARD tracks are stored at the end of the GRID + 1.

#define CLIPBOARD_FILE_SIZE                                                    \
  (uint32_t) GRID_SLOT_BYTES +                                                 \
      (uint32_t)GRID_SLOT_BYTES *(uint32_t)(GRID_LENGTH + 1) *                 \
          (uint32_t)(GRID_WIDTH + 1)

bool MCLClipBoard::init() { return true; }

bool MCLClipBoard::open() {
  DEBUG_PRINT_FN();

  SD.chdir("/");
  char str[] = FILENAME_CLIPBOARD;
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
      } else {
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
    copy_sequencer_track(MDFX_TRACK_NUM + NUM_MD_TRACKS);
  } else {
    for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
      if (!copy_sequencer_track(n + NUM_MD_TRACKS)) {
        return false;
      }
    }
  }
  return true;
}

void MCLClipBoard::copy_scene(PerfScene *s1) {
  memcpy(&scene, s1, sizeof(scene));
  copy_scene_active = 1;
}

bool MCLClipBoard::paste_scene(PerfScene *s1) {
  if (copy_scene_active) {
    memcpy(s1, &scene, sizeof(scene));
  }
  return copy_scene_active;
}

bool MCLClipBoard::copy_sequencer_track(uint8_t track) {
  DEBUG_PRINT_FN();
  bool ret = false;
  EmptyTrack empty_track;

  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }

  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(track);
  if (gdt == nullptr) { return false; }

  uint8_t grid_idx = mcl_actions.get_grid_idx(track);
  uint8_t track_idx = mcl_actions.get_track_idx(track);

  auto device_track =
            ((DeviceTrack *)&empty_track)->init_track_type(gdt->track_type);
  bool merge = false;
  bool online = true;
  if (device_track == nullptr) { goto end; }

  ret = device_track->store_in_grid(track_idx, GRID_LENGTH, gdt->seq_track, merge,
                                     online, &grids[grid_idx]);

  end:
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
      paste_sequencer_track(MDFX_TRACK_NUM + NUM_MD_TRACKS, MDFX_TRACK_NUM + NUM_MD_TRACKS);
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


  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(source_track);
  uint8_t source_track_idx = mcl_actions.get_track_idx(source_track);

  if (gdt == nullptr) { return false; }

  gdt = mcl_actions.get_grid_dev_track(track);
  uint8_t track_idx = mcl_actions.get_track_idx(track);

  if (gdt == nullptr) { return false; }

  uint8_t grid_idx = mcl_actions.get_grid_idx(track);

  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }

  auto *device_track =
      temp_track.load_from_grid_512(source_track_idx, GRID_LENGTH, &grids[grid_idx]);

  if (device_track == nullptr) {
    close();
    return false;
  }

  DEBUG_PRINTLN("getting ready to paste");
  device_track->paste_track(source_track_idx, track_idx, gdt->seq_track);

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  if (devs[0] == &MD && track_idx == last_md_track) {
    if (mcl.currentPage() == SEQ_STEP_PAGE) {
      seq_step_page.config();
    }
    if (mcl.currentPage() == SEQ_PTC_PAGE) {
      seq_ptc_page.config();
    }
  }
  close();
  return true;
}

bool MCLClipBoard::copy(uint8_t col, uint16_t row, uint8_t w, uint16_t h) {
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

  DEBUG_PRINTLN("copy");
  DEBUG_PRINTLN(col);
  DEBUG_PRINTLN(row);
  DEBUG_PRINTLN(w);

  uint8_t grid = col / 16;
  for (int y = 0; y < h; y++) {
    proj.select_grid(grid);
    proj.read_grid_row_header(&header, y + row);
    ret = grids[grid].write_row_header(&header, y + row);
    DEBUG_PRINTLN(header.name);
    if (h > 4) {
      mcl_gui.draw_progress("", y, h);
    }
    for (uint8_t x = 0; x < w; x++) {
      uint8_t s_col = x + col;
      if (x + col >= 16) { s_col -= 16; grid = 1; }
      proj.select_grid(grid);
      DEBUG_PRINT("Copy: "); DEBUG_PRINT(s_col); DEBUG_PRINT(" "); DEBUG_PRINT(y + row); DEBUG_PRINT(" "); DEBUG_PRINTLN(grid);
      auto *ptrack = temp_track.load_from_grid_512(s_col, y + row);
      DEBUG_DUMP(temp_track.active);
      if (ptrack != nullptr) {
        bool merge = false;
        bool online = false;
        ptrack->store_in_grid(s_col, y + row, nullptr, merge, online, grids + grid);
      }
      else { DEBUG_PRINTLN("ptrack null"); }
    }
  }
  close();
  proj.select_grid(old_grid);
  return true;
}
bool MCLClipBoard::paste(uint8_t col, uint16_t row) {
  DEBUG_PRINT_FN();
  uint8_t old_grid = proj.get_grid();
  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }
  DEBUG_PRINTLN("paste here");
  bool destination_same = (col == t_col || t_w == 1);

  // setup buffer frame
  EmptyTrack empty_track;

  GridRowHeader headers[2];

  GridRowHeader header_copy;

  uint8_t grid = col / 16;

  for (int y = 0; y < t_h && y + row < GRID_LENGTH; y++) {
    proj.select_grid(0);
    proj.read_grid_row_header(headers, y + row);
    proj.select_grid(1);
    proj.read_grid_row_header(headers + 1, y + row);
    if ((!headers[0].active) || (strlen(headers[0].name) == 0) ||
        (t_w == GRID_WIDTH && col == 0)) {
      grids[grid].read_row_header(&header_copy, y + t_row);
      headers[0].active = true;
      headers[1].active = true;
      if (header_copy.active) {
        strncpy(&(headers[0].name[0]), &(header_copy.name[0]), sizeof(headers[0].name));
        strncpy(&(headers[1].name[0]), &(header_copy.name[0]), sizeof(headers[0].name));
      } else {
        headers[0].name[0] = '\0';
        headers[1].name[0] = '\0';
      }
    }
    if (t_h > 8) {
      mcl_gui.draw_progress("", y, t_h);
    }
    for (uint8_t x = 0; x < t_w && x + col < GRID_WIDTH * 2; x++) {

      uint8_t s_col = x + t_col;
      uint8_t d_col = x + col;

      uint8_t slot_n = d_col;

      grid = s_col / 16;

      if (s_col >= GRID_WIDTH) { s_col -= GRID_WIDTH; }

      auto *ptrack = empty_track.load_from_grid_512(s_col, y + t_row, grids + grid);
      if (ptrack == nullptr) { continue; }


      DEBUG_PRINTLN(slot_n);

      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(slot_n);
      uint8_t track_idx = mcl_actions.get_track_idx(slot_n);

      if ((gdt == nullptr || (gdt->track_type != ptrack->active && ptrack->get_parent_model() != gdt->track_type)) &&
          (ptrack->active != EMPTY_TRACK_TYPE)) {
        DEBUG_PRINTLN("track not supported");
        // Don't allow paste in to unsupported slots
        continue;
      }
      int16_t link_row_offset = ptrack->link.row - t_row;

      uint8_t new_link_row = row + link_row_offset;
      if (new_link_row >= GRID_LENGTH) {
        new_link_row = y + row;
      } else if (new_link_row < 0) {
        new_link_row = y + row;
      }
      grid = d_col / 16;
      if (d_col >= GRID_WIDTH) { d_col -= GRID_WIDTH; }

      DEBUG_PRINT("PASTE: "); DEBUG_PRINT(s_col); DEBUG_PRINT("->"); DEBUG_PRINT(d_col); DEBUG_PRINT(" "); DEBUG_PRINTLN(grid);
      proj.select_grid(grid);
      ptrack->link.row = new_link_row;
      headers[grid].update_model(d_col, ptrack->get_model(), ptrack->active);
      ptrack->on_copy(s_col, d_col, destination_same);
      ptrack->store_in_grid(d_col, y + row);
    }
    proj.select_grid(0);
    proj.write_grid_row_header(headers, y + row);
    proj.select_grid(1);
    proj.write_grid_row_header(headers + 1, y + row);
  }
  close();
  proj.select_grid(old_grid);
  return true;
}

MCLClipBoard mcl_clipboard;
