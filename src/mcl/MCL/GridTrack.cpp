#include "GridTrack.h"
#include "Project.h"
#include "MCLActions.h"
#include "platform.h"
#if !defined(__AVR__)
#include "../Drivers/Generic/Sequencer/StepSeqDefines.h"
#endif

bool GridTrack::write_grid(void *data, size_t len, GridSlot column, GridRow row, Grid *grid) {
  stamp_storage_version(len);
  if (grid == nullptr) {
    return proj.write_grid(data, len, column, row);
  }
  else {
    return grid->write(data, len, column, row);
  }
}

void GridTrack::stamp_storage_version(size_t len) {
  version = 0;
  storage_size = 0;
  uint16_t project_version = proj.version;
  if (project_version >= PROJ_VERSION_TRACK_STORAGE_VERSION) {
    version = storage_version();
    if (project_version >= PROJ_VERSION_DYNAMIC_TRACK_STORAGE &&
        len <= UINT16_MAX) {
      storage_size = (uint16_t)len;
    }
  }
}

bool GridTrack::storage_version_at_least(uint8_t min_version) const {
  return proj.version >= PROJ_VERSION_TRACK_STORAGE_VERSION &&
         version >= min_version;
}

uint16_t GridTrack::stored_track_size(uint16_t current_size) const {
  if (proj.version >= PROJ_VERSION_DYNAMIC_TRACK_STORAGE &&
      storage_size > 0 && storage_size <= GRID_SLOT_BYTES) {
    return storage_size < current_size ? storage_size : current_size;
  }
  return current_size;
}

uint16_t GridTrack::cached_track_size(uint16_t current_size) const {
  uint16_t bytes = stored_track_size(current_size);
  uint16_t header_size = _sizeof();
  return bytes < header_size ? header_size : bytes;
}

bool GridTrack::store_in_mem(GridSlot column) {
  uintptr_t pos = get_region() +
                  static_cast<uintptr_t>(get_region_size() *
                                         static_cast<uint32_t>(column));
  volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
  memcpy_bank1(ptr, _this(), cached_track_size(get_track_size()));
  return true;
}

bool GridTrack::load_from_mem(GridSlot column, size_t size) {
  uintptr_t pos = get_region() +
                  static_cast<uintptr_t>(get_region_size() *
                                         static_cast<uint32_t>(column));
  volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);

  if (size) {
    memcpy_bank1(_this(), ptr, size);
    return true;
  }

  uint8_t expected_active = active;
  uint16_t header_size = _sizeof();
  memcpy_bank1(_this(), ptr, header_size);

  if (active != expected_active) {
    return true;
  }

  uint16_t current_size = get_track_size();
  uint16_t bytes = cached_track_size(current_size);
  if (bytes < current_size) {
    init_defaults();
  }
  memcpy_bank1(_this(), ptr, bytes);
  return true;
}

void GridTrack::repair_loaded_header() {
  uint8_t tmp_version = version;
  uint16_t tmp_storage_size = storage_size;
  uint8_t tmp_flags = flags;
  uint8_t tmp_active = active;
  ::new (this) GridTrack;
  version = tmp_version;
  storage_size = tmp_storage_size;
  flags = tmp_flags;
  active = tmp_active;

  if (active == 255) {
    init();
  }
}

void GridTrack::load_link_data(SeqTrack *seq_track) {
  seq_track->speed = link.speed_value();
  seq_track->length = link.length;
}

#if !defined(__AVR__)
uint8_t GridTrack::transition_countdown_resolution() {
  return STEPSEQ_LEGACY_SEQ_INTERPOLATION;
}
#endif

void GridTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                GridSlot slotnumber) {
  uint8_t n = slotnumber;
  if (seq_track == nullptr) { return; }
#if defined(__AVR__)
  uint32_t target = (uint32_t)mcl_actions.next_transition * 12u +
                    mcl_actions.transition_offsets[n];
  if (target > 0) {
    target--;
  }
  uint32_t count_down =
      MidiClock.clock_diff_div192(MidiClock.div192th_counter, target);
  if (count_down > (0x10000UL * 12u - 4096u)) {
    count_down = 1;
  }
#else
  const uint32_t now = MidiClock.div192th_counter;
  uint32_t target =
      MidiClock.div16th_to_div192(mcl_actions.next_transition,
                                  mcl_actions.transition_offsets[n]);
  if (target > 0) {
    target--;
  }
  uint32_t count_down = MidiClock.clock_diff_div192(now, target);
  if (target < now && (now - target) < 4096u) {
    count_down = 1;
  }
  const uint8_t countdown_resolution = transition_countdown_resolution();
  if (countdown_resolution > 0 &&
      MidiClock.clock_interpolation > countdown_resolution) {
    const uint8_t divider = MidiClock.clock_interpolation / countdown_resolution;
    if (divider > 1) {
      count_down = (count_down + divider - 1) / divider;
    }
  }
#endif
  USE_LOCK();
  SET_LOCK();
  seq_track->count_down = count_down;
  CLEAR_LOCK();

}
bool GridTrack::load_from_grid_512(GridSlot column, GridRow row, Grid *grid) {

  if (grid) {
    if (!grid->read(_this(), 512, column, row)) {
      DEBUG_PRINTLN(F("read failed"));
      return false;
    }
  }
  else {
    if (!proj.read_grid(_this(), 512, column, row)) {
      DEBUG_PRINTLN(F("read failed"));
      return false;
    }
  }
  repair_loaded_header();
  return true;
}

bool GridTrack::load_from_grid(GridSlot column, GridRow row) {

  if (!proj.read_grid(_this(), _sizeof(), column, row)) {
    DEBUG_PRINTLN(F("read failed"));
    return false;
  }

  repair_loaded_header();
  return true;
}

// merge and online are ignored here.
bool GridTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track, uint8_t merge,
                              bool online, Grid *grid) {

  DEBUG_PRINT_FN();

  bool ret = grid == nullptr ? proj.write_grid(_this(), _sizeof(), column, row)
                             : grid->write(_this(), _sizeof(), column, row);
  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }

  return true;
}
