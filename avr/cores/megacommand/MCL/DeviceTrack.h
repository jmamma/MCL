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

class DeviceTrack : public GridTrack {

public:
  //  bool get_track_from_sysex(int tracknumber, uint8_t column);
  //  void place_track_in_sysex(int tracknumber, uint8_t column);
  virtual bool store_in_grid(uint8_t column, uint16_t row, uint8_t merge = 0,
                             bool online = false) = 0;
  DeviceTrack *init_track_type(uint8_t track_type);
  DeviceTrack *load_from_grid(uint8_t column, uint16_t row);

  template <class T> bool is() { return dynamic_cast<T *>(this) != nullptr; }
  template <class T> T *load_from_mem(uint8_t col) {
    if (!GridTrack::load_from_mem(col)) {
      return nullptr;
    }
    return dynamic_cast<T*>(this);
  }
  template <class T> T *load_from_grid(uint8_t col, uint16_t row) {
    auto *p = load_from_grid(col, row);
    if (!p) return nullptr;
    return dynamic_cast<T*>(p);
  }
};

#endif /* DEVICETRACK_H__ */
