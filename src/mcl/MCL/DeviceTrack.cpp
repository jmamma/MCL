#include "DeviceTrack.h"
#include "Project.h"
#include "Track.h"
#include "platform.h"

namespace {

DeviceTrack *load_grid_512(DeviceTrack *track, GridSlot column, GridRow row,
                           Grid *grid, bool *loaded_header) {
  if (loaded_header != nullptr) {
    *loaded_header = false;
  }
  if (!track->GridTrack::load_from_grid_512(column, row, grid)) {
    return nullptr;
  }

  if (loaded_header != nullptr) {
    *loaded_header = true;
  }

  auto ptrack = track->init_loaded_track_type(track->active);
  if (track->active != EMPTY_TRACK_TYPE) {
    uint16_t len = ptrack->get_track_size();
    if (len > 512) {
      len -= 512;
      uint8_t *dst = static_cast<uint8_t *>(track->_this()) + 512;
      Grid *source_grid = grid;
      if (source_grid == nullptr) {
        source_grid = &proj.grids[column >> 4];
      }
      if (!source_grid->read(dst, len)) {
        DEBUG_PRINTLN(F("read failed"));
        return nullptr;
      }
    }
  }
  return ptrack;
}

} // namespace

// Uncomment to show class sizes at compile time
template<int N> struct ShowSize;
/*
ShowSize<sizeof(EmptyTrack)> show_empty_size;
ShowSize<sizeof(MDTrack)> show_md_size;
ShowSize<sizeof(A4Track)> show_a4_size;
ShowSize<sizeof(ExtTrack)> show_ext_size;
ShowSize<sizeof(MDFXTrack)> show_mdfx_size;
ShowSize<sizeof(MDTempoTrack)> show_mdtempo_size;
ShowSize<sizeof(MDRouteTrack)> show_mdroute_size;
ShowSize<sizeof(MNMTrack)> show_mnm_size;
ShowSize<sizeof(PerfTrack)> show_perf_size;
ShowSize<sizeof(GridRowHeader)> show_perf_size;
*/

DeviceTrack *DeviceTrack::init_track_type(uint8_t track_type) {
  switch (track_type) {
  default:
  case EMPTY_TRACK_TYPE:
    ::new (this) EmptyTrack;
    break;
  case MD_TRACK_TYPE:
    ::new (this) MDTrack;
    break;
#if !defined(__AVR__)
  case MDSPSX_TRACK_TYPE:
    ::new (this) SPSXTrack;
    break;
#endif
  case A4_TRACK_TYPE:
    ::new (this) A4Track;
    break;
  case EXT_TRACK_TYPE:
    ::new (this) ExtTrack;
    break;
  case MDFX_TRACK_TYPE:
    ::new (this) MDFXTrack;
    break;
  case MDTEMPO_TRACK_TYPE:
    ::new (this) MDTempoTrack;
    break;
  case MD_ROUTE_TRACK_TYPE:
    ::new (this) MDRouteTrack;
    break;
  case MNM_TRACK_TYPE:
    ::new (this) MNMTrack;
    break;
#if !defined(__AVR__)
  case MIDI_TRACK_TYPE:
    ::new (this) MidiTrack;
    break;
  case A4_MIDI_TRACK_TYPE:
    ::new (this) A4MidiTrack;
    break;
  case MNM_MIDI_TRACK_TYPE:
    ::new (this) MNMMidiTrack;
    break;
#endif
#ifdef PLATFORM_TBD
  case TBD_TRACK_TYPE:
    ::new (this) TBDTrack;
    break;
  case TBD_MIDI_TRACK_TYPE:
    ::new (this) TBDMidiTrack;
    break;
#endif
  case GRIDCHAIN_TRACK_TYPE:
    ::new (this) GridChainTrack;
    break;
  case PERF_TRACK_TYPE:
    ::new (this) PerfTrack;
    break;
  }
  return this;
}

#if !defined(__AVR__)
bool DeviceTrack::can_materialize_as(uint8_t track_type) {
  return active == track_type;
}
#endif

DeviceTrack *DeviceTrack::load_from_mem(GridSlot col, uint8_t track_type,
                                        size_t size) {
  DeviceTrack *that = init_track_type(track_type);
  if (!that->GridTrack::load_from_mem(col, size)) {
    return nullptr;
  }
  if (that->active == track_type) {
    return that;
  }

  uint8_t source_active = that->active;
  if (size) {
    return nullptr;
  }

  auto p = init_loaded_track_type(source_active);
  if (!p->GridTrack::load_from_mem(col) || p->active != source_active) {
    return nullptr;
  }
  return p->materialize_as(track_type, col, nullptr);
}

DeviceTrack *DeviceTrack::materialize_as(uint8_t track_type,
                                         uint8_t tracknumber,
                                         SeqTrack *seq_track) {
  (void)tracknumber;
  (void)seq_track;
  if (active == track_type) {
    return this;
  }
  uint16_t source_offset;
  uint16_t target_offset;
  uint16_t len;
  if (materialized_storage_range(track_type, source_offset, target_offset,
                                 len)) {
    return materialize_storage_range(track_type, source_offset, target_offset,
                                     len);
  }
  return nullptr;
}

DeviceTrack *DeviceTrack::load_from_grid_512(GridSlot column, GridRow row,
                                             Grid *grid) {
  return load_grid_512(this, column, row, grid, nullptr);
}

DeviceTrack *DeviceTrack::materialize_storage_range(uint8_t track_type,
                                                    uint16_t source_offset,
                                                    uint16_t target_offset,
                                                    uint16_t len) {
  uint8_t *base = static_cast<uint8_t *>(_this());
  DeviceTrack *target = init_materialized_track_type(track_type);
  memmove(static_cast<uint8_t *>(target->_this()) + target_offset,
          base + source_offset, len);
  return target;
}

DeviceTrack *DeviceTrack::load_from_grid(GridSlot column, GridRow row) {
  if (!GridTrack::load_from_grid(column, row)) {
    return nullptr;
  }

  // header read successfully. now reconstruct the object.
  auto ptrack = init_loaded_track_type(active);

  // virtual functions are ready

  if (active != EMPTY_TRACK_TYPE) {
    uint16_t len = ptrack->get_track_size();
    if (!proj.read_grid((uint8_t*)_this(), len, column, row)) {
      DEBUG_PRINTLN(F("read failed"));
      return nullptr;
    }
  }

  return ptrack;
}

bool DeviceTrackChunk::load_from_mem_chunk(GridSlot column, uint8_t chunk) {
  size_t chunk_size = sizeof(seq_data_chunk);

  // 1. Safely calculate the byte offset of the seq_data_chunk member.
  uintptr_t offset = reinterpret_cast<uintptr_t>(&seq_data_chunk[0]) -
                     reinterpret_cast<uintptr_t>(_this());

  // 2. Calculate the final address using uintptr_t for the variable and all
  // parts of the calculation.
  uintptr_t pos = get_region() +
                  static_cast<uintptr_t>(get_region_size() * static_cast<uint32_t>(column)) + offset +
                  (chunk_size * chunk);
  // 3. Convert the final integer address back to a pointer for the memory copy.
  volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
  memcpy_bank1(seq_data_chunk, ptr, chunk_size);
  return true;
}

bool DeviceTrackChunk::load_chunk(volatile void *ptr, uint8_t chunk) {
  size_t chunk_size = sizeof(seq_data_chunk);
  if (chunk == get_chunk_count() - 1) {
    chunk_size = get_seq_data_size() - sizeof(seq_data_chunk) * chunk;
  }
  memcpy((uint8_t *)ptr + sizeof(seq_data_chunk) * chunk, seq_data_chunk,
         chunk_size);
  return true;
}

bool DeviceTrackChunk::load_link_from_mem(GridSlot column) {
  // 1. Safely calculate the byte offset of the 'link' member.
  uintptr_t offset = reinterpret_cast<uintptr_t>(&this->link) -
                     reinterpret_cast<uintptr_t>(_this());
  // 2. Calculate the final address using uintptr_t.
  uintptr_t pos = get_region() +
                  static_cast<uintptr_t>(get_region_size() * static_cast<uint32_t>(column)) + offset;
  // 3. Convert the final integer address back to a pointer.
  volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
  memcpy_bank1(&this->link, ptr, sizeof(GridLink));
  return true;
}
