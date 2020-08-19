/* Justin Mammarella jmamma@gmail.com 2018 */
#ifndef DEVICETRACK_H__
#define DEVICETRACK_H__

#include "GridTrack.h"

#define A4_TRACK_TYPE_270 2
#define MD_TRACK_TYPE_270 1
#define EXT_TRACK_TYPE_270 3

#define MD_TRACK_TYPE 4
#define A4_TRACK_TYPE 5
#define EXT_TRACK_TYPE 6

class EmptyTrack;
class ExtTrack;
class A4Track;
class MDTrack;


#define __IMPL_DYNAMIK_KAST(klass, aktive) \
  void _dynamik_kast_impl(DeviceTrack* p, klass** pp) { \
    if (p->active == aktive) { \
      *pp = (klass*)p; \
    } else { \
      *pp = nullptr; \
    } \
  }

class DeviceTrack : public GridTrack {

private:
  template<class T> T* _dynamik_kast(DeviceTrack* p) { 
    T* ret;
    _dynamik_kast_impl(p, &ret); 
    return ret;
  }
  __IMPL_DYNAMIK_KAST(EmptyTrack, EMPTY_TRACK_TYPE || p->active == 255)
  __IMPL_DYNAMIK_KAST(ExtTrack, EXT_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(A4Track, A4_TRACK_TYPE)
  __IMPL_DYNAMIK_KAST(MDTrack, MD_TRACK_TYPE)

public:
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);
  virtual bool store_in_grid(uint8_t column, uint16_t row, uint8_t merge = 0,
                             bool online = false) = 0;
  DeviceTrack *init_track_type(uint8_t track_type);

  DeviceTrack *load_from_grid(uint8_t column, uint16_t row);
  template <class T> T *load_from_grid(uint8_t col, uint16_t row) {
    auto *p = load_from_grid(col, row);
    if (!p) return nullptr;
    return _dynamik_kast<T>(p);
  }

  template <class T> bool is() { return _dynamik_kast<T>(this) != nullptr; }
  template <class T> T* as() { return _dynamik_kast<T>(this); }

  ///  downloads from BANK1 to the runtime object
  DeviceTrack* load_from_mem(uint8_t col) {
    if (!GridTrack::load_from_mem(col)) {
      return nullptr;
    }
    return this;
  }

  ///  downloads from BANK1 to the runtime object
  template <class T> T *load_from_mem(uint8_t col) {
    if (!GridTrack::load_from_mem(col)) {
      return nullptr;
    }
    return _dynamik_kast<T>(this);
  }
};

#endif /* DEVICETRACK_H__ */
