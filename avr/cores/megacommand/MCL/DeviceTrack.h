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
class MDLFOTrack;
class MNMTrack;
class PerfTrack;
class GridChainTrack;

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

class DeviceTrack : public GridTrack {

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
  __IMPL_DYNAMIK_KAST(MDTrack, MD_TRACK_TYPE, MD_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDFXTrack, MDFX_TRACK_TYPE, MDFX_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDRouteTrack, MDROUTE_TRACK_TYPE, MDROUTE_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDTempoTrack, MDTEMPO_TRACK_TYPE, MDTEMPO_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MNMTrack, MNM_TRACK_TYPE, MNM_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDLFOTrack, MDLFO_TRACK_TYPE, MDLFO_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(PerfTrack, PERF_TRACK_TYPE, PERF_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(GridChainTrack, GRIDCHAIN_TRACK_TYPE, GRIDCHAIN_TRACK_TYPE)

public:
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);

//  virtual bool store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track = nullptr, uint8_t merge = 0, bool online = false, Grid *grid = nullptr) = 0;
  virtual uint16_t get_track_size() = 0;
  virtual void* get_sound_data_ptr() = 0;
  virtual size_t get_sound_data_size() = 0;

  virtual uint16_t calc_latency(uint8_t tracknumber) { return 0; }

  DeviceTrack *init_track_type(uint8_t track_type);
  template <class T> DeviceTrack *init_track_type() {
    T *p;
    _init_track_type_impl(&p);
    return p;
  }

  DeviceTrack *load_from_grid_512(uint8_t column, uint16_t row, Grid *grid = nullptr);
  DeviceTrack *load_from_grid(uint8_t column, uint16_t row);
  template <class T> T *load_from_grid(uint8_t col, uint16_t row) {
    auto *p = load_from_grid(col, row);
    if (!p)
      return nullptr;
    auto ptrack = p->init_track_type(p->active);
    return _dynamik_kast<T>(ptrack);
  }

  template <class T> bool is() { return _dynamik_kast<T>(this) != nullptr; }
  template <class T> T *as() { return _dynamik_kast<T>(this); }
  ///  downloads from BANK1 to the runtime object
  DeviceTrack* load_from_mem(uint8_t col, uint8_t track_type, size_t size = 0) {
    DeviceTrack *that = init_track_type(track_type);
    if (!that->GridTrack::load_from_mem(col, size)) {
      return nullptr;
    }
    auto p = init_track_type(this->active);
    if (p->active != track_type) {
      if (p->get_parent_model() == track_type) {
        if (p->allow_cast_to_parent()) {
          p->active = p->get_parent_model();
        }
      }
      else {
        return nullptr;
      }
    }
    auto ptrack = that->init_track_type(p->active);
    return ptrack;
  }

  int memcmp_sound(uint8_t column) {
    uint16_t pos = get_region() + get_track_size() * (uint16_t)(column) + ((uint16_t) get_sound_data_ptr() - (uint16_t) this);
    volatile uint8_t *ptr = reinterpret_cast<uint8_t *>(pos);
    return memcmp_bank1(get_sound_data_ptr(), ptr, get_sound_data_size());
  }

  template <class T> T *load_from_mem(uint8_t col) {
    DeviceTrack *that = init_track_type<T>();
    /*
    diag_page.println("load", (uint16_t)that);
    diag_page.println("this", (uint16_t)this);
    diag_page.println("region", (uint16_t)that->get_region());
    */
    if (!that->GridTrack::load_from_mem(col)) {
      return nullptr;
    }
    auto ptrack = that->init_track_type(that->active);
    return _dynamik_kast<T>(ptrack);
  }
};

class DeviceTrackChunk : public DeviceTrack {
  public:
  DeviceTrackChunk() {
  }

  uint8_t seq_data_chunk[256];

  bool load_from_mem_chunk(uint8_t column, uint8_t chunk);
  bool load_chunk(volatile void *ptr, uint8_t chunk);
  bool load_link_from_mem(uint8_t column);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false) {};

  uint8_t get_chunk_count() { return (get_seq_data_size() / sizeof(seq_data_chunk)) + 1; }

  virtual uint16_t get_seq_data_size() = 0;
  virtual uint8_t get_model() = 0;
  virtual uint16_t get_track_size() = 0;
  virtual uint16_t get_region() = 0;
  virtual uint8_t get_device_type() = 0;

  virtual void *get_sound_data_ptr() = 0;
  virtual size_t get_sound_data_size() = 0;
  virtual size_t get_sound_cmp_size() { return get_sound_data_size(); }
};

#endif /* DEVICETRACK_H__ */
