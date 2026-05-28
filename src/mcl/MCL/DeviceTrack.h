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
class MDTempoTrack;
class MNMTrack;
class PerfTrack;
class GridChainTrack;
class SPSXTrack;
#if !defined(__AVR__)
class MidiTrack;
class A4MidiTrack;
class MNMMidiTrack;
#endif
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
  __IMPL_DYNAMIK_KAST(ExtTrack, EXT_TRACK_TYPE, EXT_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(A4Track, A4_TRACK_TYPE, A4_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(SPSXTrack, MDSPSX_TRACK_TYPE, MDSPSX_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDTrack, MD_TRACK_TYPE, MD_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDFXTrack, MDFX_TRACK_TYPE, MDFX_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDRouteTrack, MD_ROUTE_TRACK_TYPE, MD_ROUTE_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDTempoTrack, MDTEMPO_TRACK_TYPE, MDTEMPO_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MNMTrack, MNM_TRACK_TYPE, MNM_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(PerfTrack, PERF_TRACK_TYPE, PERF_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(GridChainTrack, GRIDCHAIN_TRACK_TYPE, GRIDCHAIN_TRACK_TYPE)
#if !defined(__AVR__)
  __IMPL_DYNAMIK_KAST(MidiTrack, MIDI_TRACK_TYPE, MIDI_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(A4MidiTrack, A4_MIDI_TRACK_TYPE, A4_MIDI_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MNMMidiTrack, MNM_MIDI_TRACK_TYPE, MNM_MIDI_TRACK_TYPE)
#endif
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
#if !defined(__AVR__)
  virtual bool can_materialize_as(uint8_t track_type);
#endif
  virtual DeviceTrack *materialize_as(uint8_t track_type,
                                      uint8_t tracknumber,
                                      SeqTrack *seq_track);
  virtual bool materialized_storage_range(uint8_t track_type,
                                          uint16_t &source_offset,
                                          uint16_t &target_offset,
                                          uint16_t &len) {
    (void)track_type;
    (void)source_offset;
    (void)target_offset;
    (void)len;
    return false;
  }
  DeviceTrack *init_materialized_track_type(uint8_t track_type) {
    uint8_t old_version = version;
    uint8_t old_reserved = reserved;
    GridLink old_link = link;
    DeviceTrack *track = init_track_type(track_type);
    track->version = old_version;
    track->reserved = old_reserved;
    track->link = old_link;
    return track;
  }
  DeviceTrack *init_loaded_track_type(uint8_t track_type) {
    uint8_t loaded_header[DEVICE_TRACK_LEN];
    memcpy(loaded_header, _this(), DEVICE_TRACK_LEN);
    DeviceTrack *track = init_track_type(track_type);
    memcpy(track->_this(), loaded_header, DEVICE_TRACK_LEN);
    return track;
  }
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
  DeviceTrack *load_from_mem(GridSlot col, uint8_t track_type, size_t size = 0);

  DeviceTrack *materialize_storage_range(uint8_t track_type,
                                         uint16_t source_offset,
                                         uint16_t target_offset,
                                         uint16_t len);

  int memcmp_sound(GridSlot column) {
    // 1. Get the base address. It doesn't matter if it's from the constexpr or linker world.
    //    It's now safely inside a uintptr_t.
    uintptr_t region_base = get_region();

    // 2. Calculate offsets using safe, portable uintptr_t arithmetic.
    uintptr_t sound_data_offset =
        reinterpret_cast<uintptr_t>(get_sound_data_ptr()) -
        reinterpret_cast<uintptr_t>(_this());
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
        reinterpret_cast<uintptr_t>(sound) - reinterpret_cast<uintptr_t>(_this());
    uintptr_t pos = region_base +
                    (static_cast<uintptr_t>(get_region_size()) * column) +
                    sound_data_offset;
    volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
    memcpy_bank1(sound, ptr, sound_size);
  }

  bool restore_sound_from_mem_if_type(GridSlot column, uint8_t expected_type) {
    void *sound = get_sound_data_ptr();
    size_t sound_size = get_sound_data_size();
    if (sound == nullptr || sound_size == 0) {
      return true;
    }

    uintptr_t region_base = get_region();
    uintptr_t slot_base =
        region_base +
        (static_cast<uintptr_t>(get_region_size()) * column);
    uintptr_t active_offset =
        reinterpret_cast<uintptr_t>(&active) -
        reinterpret_cast<uintptr_t>(_this());
    uint8_t cached_active = EMPTY_TRACK_TYPE;
    volatile uint8_t *active_ptr =
        reinterpret_cast<volatile uint8_t *>(slot_base + active_offset);
    memcpy_bank1(&cached_active, active_ptr, sizeof(cached_active));
    if (cached_active != expected_type) {
      return false;
    }
    uintptr_t sound_data_offset =
        reinterpret_cast<uintptr_t>(sound) - reinterpret_cast<uintptr_t>(_this());
    uintptr_t pos = slot_base + sound_data_offset;
    volatile uint8_t *ptr = reinterpret_cast<volatile uint8_t *>(pos);
    memcpy_bank1(sound, ptr, sound_size);
    return true;
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
  virtual void *get_sound_data_ptr() = 0;
  virtual size_t get_sound_data_size() = 0;
  virtual size_t get_sound_cmp_size() { return get_sound_data_size(); }
};

#endif /* DEVICETRACK_H__ */
