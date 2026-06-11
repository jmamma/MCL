#pragma once

#include "Sequencer/MCLSeq.h"

#include "DeviceManager.h"
#include "MCLSysConfig.h"

static inline bool seq_grid_x_runs_md_tracks() {
#ifdef PLATFORM_TBD
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_MD;
#else
  return true;
#endif
}

#if defined(PLATFORM_TBD)
static inline bool seq_grid_x_runs_tbd_tracks() {
  return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}
#endif

#if !defined(__AVR__)
static inline bool seq_grid_y_runs_midi_tracks() {
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_OFF) {
    return false;
  }
#if defined(PLATFORM_TBD)
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    return true;
  }
#endif
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    return true;
  }
  MidiDevice *secondary = device_manager.secondary_device();
  return secondary != nullptr &&
         (secondary->id == DEVICE_A4 || secondary->id == DEVICE_MNM);
}
#endif

static inline bool seq_grid_y_runs_legacy_ext_tracks() {
#if defined(__AVR__)
  return true;
#else
  return mcl_cfg.grid_y_device != GRID_Y_DEVICE_OFF &&
         !seq_grid_y_runs_midi_tracks();
#endif
}
