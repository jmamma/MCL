/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#include "MCLClipBoard.h"
#include "MCLSd.h"
#include "Project.h"
#include "EmptyTrack.h"
#include "DeviceTrack.h"
#include "MDTrack.h"
#include "MCLGUI.h"
#include "MCLActions.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

// Sequencer CLIPBOARD tracks are stored at the end of the GRID + 1.

#define CLIPBOARD_FILE_SIZE                                                    \
  (uint32_t) GRID_SLOT_BYTES +                                                 \
      (uint32_t)GRID_SLOT_BYTES *(uint32_t)(GRID_LENGTH + 1) *                 \
          (uint32_t)(GRID_WIDTH + 1)

namespace {

bool clipboard_track_supported(DeviceTrack *track, GridDeviceTrack *gdt) {
  if (track == nullptr) {
    return false;
  }
  if (track->active == EMPTY_TRACK_TYPE) {
    return true;
  }
  if (gdt == nullptr) {
    return false;
  }
  return track->can_materialize_as(gdt->track_type);
}

DeviceTrack *materialize_clipboard_track(DeviceTrack *track,
                                         GridDeviceTrack *gdt,
                                         GridColumn track_idx) {
  if (!clipboard_track_supported(track, gdt)) {
    return nullptr;
  }
  if (track->active == EMPTY_TRACK_TYPE) {
    return track;
  }
  return track->materialize_as(gdt->track_type, track_idx, gdt->seq_track);
}

bool clipboard_slot_is_type(GridSlot slot, uint8_t track_type) {
  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(slot);
  return gdt != nullptr && gdt->track_type == track_type;
}

} // namespace

bool MCLClipBoard::init() { return true; }

bool MCLClipBoard::open() {
  DEBUG_PRINT_FN();

#ifndef __AVR__
  SD.chdir(mcl_sd.mcl_root[0] == '\0' ? "/" : mcl_sd.mcl_root);
#else
  SD.chdir("/");
#endif
  char grid_filename[] = FILENAME_CLIPBOARD ".0";
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grid_filename[sizeof(FILENAME_CLIPBOARD)] = i + '0';
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

bool MCLClipBoard::copy_sequencer(GridSlot offset) {
  bool is_md = (offset < NUM_MD_TRACKS);
  GridSlot start_track = is_md ? 0 : NUM_MD_TRACKS;
  uint8_t num_tracks = is_md ? NUM_MD_TRACKS : NUM_EXT_TRACKS;

  for (uint8_t n = 0; n < num_tracks; n++) {
    if (!copy_sequencer_track(start_track + n)) {
      return false;
    }
  }

  const GridSlot md_fx_slot = MDFX_TRACK_NUM + NUM_MD_TRACKS;
  if (is_md && clipboard_slot_is_type(md_fx_slot, MDFX_TRACK_TYPE) &&
      !copy_sequencer_track(md_fx_slot)) {
    return false;
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

bool MCLClipBoard::copy_sequencer_track(GridSlot track) {
  DEBUG_PRINT_FN();
  bool ret = false;
  EmptyTrack empty_track;

  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(track);
  if (gdt == nullptr) { return false; }

  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }

  GridIndex grid_idx = track >> 4;
  GridColumn track_idx = track & 0xF;

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

bool MCLClipBoard::paste_sequencer(GridSlot offset) {
  if (offset < NUM_MD_TRACKS) {
    for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
      if (!paste_sequencer_track(n, n)) {
        return false;
      }
    }
    const GridSlot md_fx_slot = MDFX_TRACK_NUM + NUM_MD_TRACKS;
    if (clipboard_slot_is_type(md_fx_slot, MDFX_TRACK_TYPE) &&
        !paste_sequencer_track(md_fx_slot, md_fx_slot)) {
      return false;
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

bool MCLClipBoard::paste_sequencer_track(GridSlot source_track, GridSlot track) {
  DEBUG_PRINT_FN();

  EmptyTrack temp_track;
  GridColumn source_track_idx = source_track & 0xF;

  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(track);
  GridColumn track_idx = track & 0xF;

  if (gdt == nullptr) { return false; }

  GridIndex source_grid_idx = source_track >> 4;

  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }

  auto *device_track =
      temp_track.load_from_grid_512(source_track_idx, GRID_LENGTH, &grids[source_grid_idx]);

  if (device_track == nullptr) {
    close();
    return false;
  }
  device_track = materialize_clipboard_track(device_track, gdt, track_idx);
  if (device_track == nullptr) {
    close();
    return false;
  }

  DEBUG_PRINTLN("getting ready to paste");
  device_track->paste_track(source_track_idx, track_idx, gdt->seq_track);

  if (SeqTrackUtil::is_md_device(device_manager.primary_device()) &&
      track_idx == last_primary_track) {
    if (mcl.currentPage() == SEQ_STEP_PAGE) {
      seq_step_page.config();
    }
    if (mcl.currentPage() == SEQ_PTC_PAGE) {
      seq_ptc_page.config();
    }
  }
  return close();
}

bool MCLClipBoard::copy(GridSlot col, GridRow row, GridSpan w, GridSpan h) {
  DEBUG_PRINT_FN();
  t_col = col;
  t_row = row;
  t_w = w;
  t_h = h;
  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }
  EmptyTrack temp_track;
  bool ret = true;
  GridRowHeader header;

  DEBUG_PRINTLN("copy");
  DEBUG_PRINTLN(col);
  DEBUG_PRINTLN(row);
  DEBUG_PRINTLN(w);

  for (uint8_t y = 0; y < h; y++) {
    GridRow dst_row = y + row;
    if (h > 4) {
      mcl_gui.draw_progress("", y, h);
    }
    GridIndex last_hdr_grid = 255;
    for (uint8_t x = 0; x < w; x++) {
      GridSlot full_col = x + col;
      GridIndex cur_grid = full_col >> 4;
      GridColumn s_col = full_col & 0xF;
      if (cur_grid != last_hdr_grid) {
        proj.read_grid_row_header(&header, dst_row, cur_grid);
        if (!grids[cur_grid].write_row_header(&header, dst_row)) {
          ret = false;
        }
        DEBUG_PRINTLN(header.name);
        last_hdr_grid = cur_grid;
      }
      DEBUG_PRINT("Copy: "); DEBUG_PRINT(full_col); DEBUG_PRINT(" "); DEBUG_PRINT(dst_row); DEBUG_PRINT(" "); DEBUG_PRINTLN(cur_grid);
      auto *ptrack = temp_track.load_from_grid_512(full_col, dst_row);
      DEBUG_DUMP(temp_track.active);
      if (ptrack != nullptr) {
        bool merge = false;
        bool online = false;
        if (!ptrack->store_in_grid(s_col, dst_row, nullptr, merge, online,
                                   grids + cur_grid)) {
          ret = false;
        }
      }
      else {
        DEBUG_PRINTLN("ptrack null");
        ret = false;
      }
    }
  }
  return close() && ret;
}
bool MCLClipBoard::paste(GridSlot col, GridRow row) {
  DEBUG_PRINT_FN();
  if (!open()) {
    DEBUG_PRINTLN(F("error could not open clipboard"));
    return false;
  }
  DEBUG_PRINTLN("paste here");
  bool ret = true;
  bool destination_same = (col == t_col || t_w == 1);

  // setup buffer frame
  EmptyTrack empty_track;

  GridRowHeader headers[2];

  GridRowHeader header_copy;

  GridIndex src_grid = t_col >> 4;

  for (uint8_t y = 0; y < t_h && y + row < GRID_LENGTH; y++) {
    GridRow dst_row = y + row;
    proj.read_grid_row_header(headers,     dst_row, 0);
    proj.read_grid_row_header(headers + 1, dst_row, 1);
    if ((!headers[0].active) || (strlen(headers[0].name) == 0) ||
        (t_w == GRID_WIDTH && col == 0)) {
      grids[src_grid].read_row_header(&header_copy, y + t_row);
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

      GridSlot full_s_col = x + t_col;
      GridSlot d_col = x + col;         // full logical dest col (0–31)

      GridSlot slot_n = d_col;

      GridIndex s_grid = full_s_col >> 4;
      GridColumn s_col  = full_s_col & 0xF;

      auto *ptrack = empty_track.load_from_grid_512(s_col, y + t_row, grids + s_grid);
      if (ptrack == nullptr) {
        ret = false;
        continue;
      }


      DEBUG_PRINTLN(slot_n);

      GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(slot_n);
      GridColumn track_idx = d_col & 0xF;

      ptrack = materialize_clipboard_track(ptrack, gdt, track_idx);
      if (ptrack == nullptr) {
        DEBUG_PRINTLN("track not supported");
        // Don't allow paste in to unsupported slots
        continue;
      }
      int16_t link_row_offset = ptrack->link.row - t_row;

      int16_t new_link_row = row + link_row_offset;
      if (new_link_row >= (int16_t)GRID_LENGTH) {
        new_link_row = dst_row;
      } else if (new_link_row < 0) {
        new_link_row = dst_row;
      }
      GridIndex d_grid = d_col >> 4;

      DEBUG_PRINT("PASTE: "); DEBUG_PRINT(s_col); DEBUG_PRINT("->"); DEBUG_PRINT(d_col); DEBUG_PRINT(" "); DEBUG_PRINTLN(d_grid);
      ptrack->link.row = (uint8_t)new_link_row;
      ptrack->on_copy(s_col, track_idx, destination_same);
      if (ptrack->store_in_grid(d_col, dst_row)) {
        headers[d_grid].update_model(track_idx, ptrack->get_model(),
                                     ptrack->active);
      } else {
        ret = false;
      }
    }
    if (!proj.write_grid_row_header(headers,     dst_row, 0)) ret = false;
    if (!proj.write_grid_row_header(headers + 1, dst_row, 1)) ret = false;
  }
  return close() && ret;
}

MCLClipBoard mcl_clipboard;
