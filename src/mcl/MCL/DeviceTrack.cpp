#include "DeviceTrack.h"
#include "Project.h"
#include "Track.h"
#include "platform.h"

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
ShowSize<sizeof(MDLFOTrack)> show_mdlfo_size;
ShowSize<sizeof(MNMTrack)> show_mnm_size;
ShowSize<sizeof(PerfTrack)> show_perf_size;
ShowSize<sizeof(GridRowHeader)> show_perf_size;
*/

DeviceTrack *DeviceTrack::init_track_type(uint8_t track_type) {
  active = track_type;
  switch (track_type) {
  default:
  case EMPTY_TRACK_TYPE:
    ::new (this) EmptyTrack;
    break;
  case MD_TRACK_TYPE:
    ::new (this) MDTrack;
    break;
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
  case MDROUTE_TRACK_TYPE:
    ::new (this) MDRouteTrack;
    break;
  case MDLFO_TRACK_TYPE:
    ::new (this) MDLFOTrack;
    break;
  case MNM_TRACK_TYPE:
    ::new (this) MNMTrack;
    break;
    //  case GRIDCHAIN_TRACK_TYPE:
    //    ::new (this) GridChainTrack;
    //    break;
  case PERF_TRACK_TYPE:
    ::new (this) PerfTrack;
    break;
  }
  return this;
}

DeviceTrack *DeviceTrack::load_from_grid_512(uint8_t column, uint16_t row,
                                             Grid *grid) {
  if (!GridTrack::load_from_grid_512(column, row, grid)) {
    return nullptr;
  }

  // header read successfully. now reconstruct the object.
  auto ptrack = init_track_type(active);

  if (ptrack == nullptr) {
    DEBUG_PRINTLN("unrecognized track type");
    return nullptr;
  }

  // virtual functions are ready

  DEBUG_PRINTLN("device load from 512");
  DEBUG_PRINTLN(active != EMPTY_TRACK_TYPE);
  DEBUG_PRINTLN(ptrack->get_track_size());
  if (active != EMPTY_TRACK_TYPE) {
    if (ptrack->get_track_size() < 512) {
      return ptrack;
    }
    size_t len = ptrack->get_track_size() - 512;
    if (grid) {
      if (!grid->read((uint8_t*)_this() + 512, len)) {
        DEBUG_PRINTLN(F("read failed"));
        return nullptr;
      }
    } else {
      if (!proj.read_grid((uint8_t*)_this() + 512, len)) {
        DEBUG_PRINTLN(F("read failed"));
        return nullptr;
      }
    }
  }
  return ptrack;
}

DeviceTrack *DeviceTrack::load_from_grid(uint8_t column, uint16_t row) {
  if (!GridTrack::load_from_grid(column, row)) {
    return nullptr;
  }

  // header read successfully. now reconstruct the object.
  auto ptrack = init_track_type(active);

  if (ptrack == nullptr) {
    DEBUG_PRINTLN("unrecognized track type");
    return nullptr;
  }

  // virtual functions are ready

  if (active != EMPTY_TRACK_TYPE) {
    uint16_t len = ptrack->get_track_size();

    if (!proj.read_grid((uint8_t*)_this(), len, column, row)) {
      DEBUG_PRINTLN(F("read failed"));
      return nullptr;
    }
  }

  auto ptrack2 = ptrack->init_track_type(active);
  if (ptrack2 == nullptr) {
    DEBUG_PRINTLN("unrecognized track type 2");
    return nullptr;
  }

  return ptrack2;
}

bool DeviceTrackChunk::load_from_mem_chunk(uint8_t column, uint8_t chunk) {
  size_t chunk_size = sizeof(seq_data_chunk);

  // 1. Safely calculate the byte offset of the seq_data_chunk member.
  uintptr_t offset = reinterpret_cast<uintptr_t>(&seq_data_chunk[0]) -
                     reinterpret_cast<uintptr_t>(this);

  // 2. Calculate the final address using uintptr_t for the variable and all
  // parts of the calculation.
  uintptr_t pos = get_region() +
                  (static_cast<uintptr_t>(get_track_size()) * column) + offset +
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

bool DeviceTrackChunk::load_link_from_mem(uint8_t column) {
  // 1. Safely calculate the byte offset of the 'link' member.
  uintptr_t offset = reinterpret_cast<uintptr_t>(&this->link) -
                     reinterpret_cast<uintptr_t>(this);
  // 2. Calculate the final address using uintptr_t.
  uintptr_t pos = get_region() +
                  (static_cast<uintptr_t>(get_track_size()) * column) + offset;
  // 3. Convert the final integer address back to a pointer.
  volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
  memcpy_bank1(&this->link, ptr, sizeof(GridLink));
  return true;
}
