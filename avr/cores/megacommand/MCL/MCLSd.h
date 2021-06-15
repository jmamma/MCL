/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSD_H__
#define MCLSD_H__

#include "SdFat.h"
#include "new.h"
#include "type_traits.h"

#define SD_MAX_RETRIES 10

class MCLSd {
  public:
  uint16_t write_fail = 0;
  uint16_t read_fail = 0;
  bool sd_state = false;
  bool sd_init();
  bool load_init();
  bool seek(uint32_t pos, FatFile *filep);
  bool read_data(void *data, size_t len, FatFile *filep);
  bool write_data(void *data, size_t len, FatFile *filep);
  /// read data from SD card and repair vtable
  template <class T> bool read_data_v(T *data, FatFile *filep) {
    auto ret = read_data(data, sizeof(T), filep);
    ::new(data)T;
    return ret;
  }
  /// Specialization for ElektronPattern...
  template <class T> bool read_data_v_noinit(T *data, FatFile *filep) {
    auto ret = read_data(data, sizeof(T), filep);
    ::new(data)T(false);
    return ret;
  }

  /// save data to SD card, including the vtable
  template <class T> bool write_data_v(T *data, FatFile *filep) {
    return write_data(data, sizeof(T), filep);
  }

};

extern MCLSd mcl_sd;
extern SdFat SD;

#endif /* MCLSD_H__ */
