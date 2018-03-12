/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSD_H__
#define MCLSD_H__
#include "MCL.h"
#include "SdFat.h"
class MCLSd {
  public:
  bool load_init();
  bool read_data(void *data, size_t len, FatFile *filep);
  bool write_data(void *data, size_t len, FatFile *filep);
};

extern MCLSd mcl_sd;
extern SdFat SD;

#endif /* MCLSD_H__ */
