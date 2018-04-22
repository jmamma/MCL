/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSD_H__
#define MCLSD_H__

#include "SdFat.h"

#define SD_MAX_RETRIES 5 

class MCLSd {
  public:
  uint16_t write_fail = 0;
  uint16_t read_fail = 0;
  bool sd_state = false;
  bool sd_init();
  bool load_init();
  bool read_data(void *data, size_t len, FatFile *filep);
  bool write_data(void *data, size_t len, FatFile *filep);
};

extern MCLSd mcl_sd;
extern SdFat SD;

#endif /* MCLSD_H__ */
