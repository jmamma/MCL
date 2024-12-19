/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "SdFat.h"
#include "hardware.h"
typedef FsFile File;

#define SD_MAX_RETRIES 10

class SdFat_ : public SdFat {
public:
    // Constructor
    SdFat_() : SdFat() {}

    // Method to access the card directly
    SdCard* getCard() {
        // Access the protected member through the base class method
        return card();
    }

    // Method to toggle dedicated SPI
    void setDedicatedSpi(bool dedicated) {
        if (getCard()) {
            getCard()->setDedicatedSpi(dedicated);
        }
    }
};

class MCLSd {
  public:
  uint16_t write_fail = 0;
  uint16_t read_fail = 0;
  bool sd_state = false;
  bool sd_init();
  bool load_init();
  bool seek(uint32_t pos, File *filep);
  bool read_data(void *data, size_t len, File *filep);
  bool write_data(void *data, size_t len, File *filep);
};

extern MCLSd mcl_sd;
extern SdFat_ SD;
