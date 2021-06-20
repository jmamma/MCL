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

public:
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);
  virtual bool store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track = nullptr, uint8_t merge = 0, bool online = false) = 0;
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
  DeviceTrack* load_from_mem(uint8_t col, uint8_t track_type) {
    DeviceTrack *that = init_track_type(track_type);
    if (!that->GridTrack::load_from_mem(col)) {
      return nullptr;
    }
    if (that->active != track_type) {
      return nullptr;
    }
    auto ptrack = that->init_track_type(that->active);
    return ptrack;
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

#endif /* DEVICETRACK_H__ */
