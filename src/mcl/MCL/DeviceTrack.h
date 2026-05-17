/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef DEVICETRACK_H__
#define DEVICETRACK_H__

#include "DiagnosticPage.h"
#include "GridTrack.h"

#define A4_TRACK_TYPE_270 2
#define MD_TRACK_TYPE_270 1
#define EXT_TRACK_TYPE_270 3

class EmptyTrack;
class ExtTrack;
class A4Track;
class MDTrack;
class MDFXTrack;
class MDRouteTrack;
class LegacyMDRouteTrack;
class MDTempoTrack;
class MDLFOTrack;
class MNMTrack;
class PerfTrack;
class GridChainTrack;
class SPSXTrack;
#ifdef PLATFORM_TBD
class TBDTrack;
class TBDMidiTrack;
#endif

#define __IMPL_DYNAMIK_KAST(klass, pred, aktive)                               \
  void _dynamik_kast_impl(DeviceTrack *p, klass **pp) {                        \
    if (p->active == pred) {                                                   \
      *pp = (klass *)p;                                                        \
    } else {                                                                   \
      *pp = nullptr;                                                           \
    }                                                                          \
  }                                                                            \
  void _init_track_type_impl(klass **p) {                                      \
    *p = (klass *)init_track_type(aktive);                                     \
  }

class ATTR_PACKED() DeviceTrack : public GridTrack {

private:
  template <class T> T *_dynamik_kast(DeviceTrack *p) {
    T *ret;
    _dynamik_kast_impl(p, &ret);
    return ret;
  }
  __IMPL_DYNAMIK_KAST(EmptyTrack, EMPTY_TRACK_TYPE || p->active == 255,
                      EMPTY_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(ExtTrack, EXT_TRACK_TYPE || p->active == A4_TRACK_TYPE || p->active == MNM_TRACK_TYPE,
                      EXT_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(A4Track, A4_TRACK_TYPE, A4_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(SPSXTrack, MDSPSX_TRACK_TYPE, MDSPSX_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDTrack, MD_TRACK_TYPE, MD_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDFXTrack, MDFX_TRACK_TYPE, MDFX_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(LegacyMDRouteTrack, MDROUTE_TRACK_TYPE, MDROUTE_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDRouteTrack, MD_ROUTE_TRACK_TYPE, MD_ROUTE_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDTempoTrack, MDTEMPO_TRACK_TYPE, MDTEMPO_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MNMTrack, MNM_TRACK_TYPE, MNM_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDLFOTrack, MDLFO_TRACK_TYPE, MDLFO_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(PerfTrack, PERF_TRACK_TYPE, PERF_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(GridChainTrack, GRIDCHAIN_TRACK_TYPE, GRIDCHAIN_TRACK_TYPE)
#ifdef PLATFORM_TBD
  __IMPL_DYNAMIK_KAST(TBDTrack, TBD_TRACK_TYPE, TBD_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(TBDMidiTrack, TBD_MIDI_TRACK_TYPE, TBD_MIDI_TRACK_TYPE)
#endif

public:

  virtual uint16_t get_track_size() = 0;
  virtual void* get_sound_data_ptr() = 0;
  virtual size_t get_sound_data_size() = 0;

  virtual uint16_t calc_latency(uint8_t tracknumber) { return 0; }

  DeviceTrack *init_track_type(uint8_t track_type);
  virtual bool can_materialize_as(uint8_t track_type);
  virtual DeviceTrack *materialize_as(uint8_t track_type,
                                      uint8_t tracknumber,
                                      SeqTrack *seq_track);
  template <class T> DeviceTrack *init_track_type() {
    T *p;
    _init_track_type_impl(&p);
    return p;
  }

  DeviceTrack *load_from_grid_512(GridSlot column, GridRow row, Grid *grid = nullptr);
  DeviceTrack *load_from_grid(GridSlot column, GridRow row);
  template <class T> T *load_from_grid(GridSlot col, GridRow row) {
    auto *p = load_from_grid(col, row);
    if (!p)
      return nullptr;
    return _dynamik_kast<T>(p);
  }

  template <class T> bool is() { return _dynamik_kast<T>(this) != nullptr; }
  template <class T> T *as() { return _dynamik_kast<T>(this); }
  ///  downloads from BANK1 to the runtime object
  DeviceTrack* load_from_mem(GridSlot col, uint8_t track_type, size_t size = 0) {
    DeviceTrack *that = init_track_type(track_type);
#if !defined(__AVR__)
    uintptr_t load_region = that->get_region();
    uint16_t load_region_size = that->get_region_size();
    uint16_t load_bytes = size ? size : that->get_track_size();
#endif
    if (!that->GridTrack::load_from_mem(col, size)) {
      return nullptr;
    }
    if (that->active == track_type) {
      return that;
    }

    auto p = init_track_type(this->active);
#if defined(__AVR__)
    return p->materialize_as(track_type, col, nullptr);
#else
    uint16_t source_size = p->get_track_size();
    bool parent_cast = p->get_parent_model() == track_type &&
                       p->allow_cast_to_parent();

    uintptr_t pos = load_region +
                    static_cast<uintptr_t>(load_region_size *
                                           static_cast<uint32_t>(col));
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    memcpy_bank1(_this(), ptr, load_bytes);

    if (!p->can_materialize_as(track_type)) {
      return nullptr;
    }
    if (load_bytes < source_size) {
      if (parent_cast) {
        return p->materialize_as(track_type, col, nullptr);
      }
      if (source_size > load_region_size) {
        return nullptr;
      }
      memcpy_bank1(_this(), ptr, source_size);
    }
    return p->materialize_as(track_type, col, nullptr);
#endif
  }

  int memcmp_sound(GridSlot column) {
    // 1. Get the base address. It doesn't matter if it's from the constexpr or linker world.
    //    It's now safely inside a uintptr_t.
    uintptr_t region_base = get_region();

    // 2. Calculate offsets using safe, portable uintptr_t arithmetic.
    uintptr_t sound_data_offset = (reinterpret_cast<uintptr_t>(get_sound_data_ptr()) - reinterpret_cast<uintptr_t>(this));
    uintptr_t pos = region_base + (static_cast<uintptr_t>(get_region_size()) * column) + sound_data_offset;
    // 3. Convert the final calculated address back to a pointer.
    volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
    return memcmp_bank1(get_sound_data_ptr(), ptr, get_sound_data_size());
  }

  void restore_sound_from_mem(GridSlot column) {
    void *sound = get_sound_data_ptr();
    size_t sound_size = get_sound_data_size();
    if (sound == nullptr || sound_size == 0) {
      return;
    }

    uintptr_t region_base = get_region();
    uintptr_t sound_data_offset =
        reinterpret_cast<uintptr_t>(sound) - reinterpret_cast<uintptr_t>(this);
    uintptr_t pos = region_base +
                    (static_cast<uintptr_t>(get_region_size()) * column) +
                    sound_data_offset;
    volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
    memcpy_bank1(sound, ptr, sound_size);
  }

  template <class T> T *load_from_mem(GridSlot col) {
    DeviceTrack *that = init_track_type<T>();
    /*
    diag_page.println("load", (uint16_t)that);
    diag_page.println("this", (uint16_t)this);
    diag_page.println("region", (uint16_t)that->get_region());
    */
    if (!that->GridTrack::load_from_mem(col)) {
      return nullptr;
    }
    return _dynamik_kast<T>(that);
  }
};

class DeviceTrackChunk : public DeviceTrack {
  public:
  DeviceTrackChunk() {
  }

  uint8_t seq_data_chunk[256];

  bool load_from_mem_chunk(GridSlot column, uint8_t chunk);
  bool load_chunk(volatile void *ptr, uint8_t chunk);
  bool load_link_from_mem(GridSlot column);
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false) { return false; };

  uint8_t get_chunk_count() { return (get_seq_data_size() / sizeof(seq_data_chunk)) + 1; }

  virtual uint16_t get_seq_data_size() = 0;
  virtual uint8_t get_model() = 0;
  virtual uint16_t get_track_size() = 0;
  virtual uintptr_t get_region() = 0;
  virtual uint8_t get_device_type() = 0;

  virtual void *get_sound_data_ptr() = 0;
  virtual size_t get_sound_data_size() = 0;
  virtual size_t get_sound_cmp_size() { return get_sound_data_size(); }
};

#endif /* DEVICETRACK_H__ */
