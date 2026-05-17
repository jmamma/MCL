/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#define SD_MAX_RETRIES 10

#include "SdFat.h"
#include "hardware.h"
#ifdef __AVR__
#include "AvrHardwarePins.h"
#endif

#if SDFAT_FILE_TYPE > 1
typedef FsFile File;
#endif

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
    void lock_spi() {
      setDedicatedSpi(false);         // Consider refactor, this is to disable the SD card
#ifdef __AVR__
      mcl_avr_pins::sd_cs_write(true);   // SDCard enable is active low
#else
      digitalWrite(SPI1_SS_PIN, HIGH);   // SDCard enable is active low
#endif
    }
    void unlock_spi() {
      setDedicatedSpi(true);
    }
};

class MCLSd {
  public:
  uint16_t write_fail = 0;
  uint16_t read_fail = 0;
  bool sd_state = false;
  char mcl_root[32];
  bool sd_init();
  bool load_init();
  bool seek(uint32_t pos, File *filep);
  bool read_data(void *data, size_t len, File *filep);
  bool write_data(void *data, size_t len, File *filep);
  bool copy_file(const char *src, const char *dst, uint8_t progress_base = 0,
                 uint8_t progress_span = 0, uint8_t progress_max = 0);
#ifndef __AVR__
  bool copy_dir(const char *src, const char *dst, uint8_t progress_base = 0,
                uint8_t progress_span = 0, uint8_t progress_max = 0);
#endif
  bool remove_dir(const char *dir);

#ifndef __AVR__
  const char *full_path(const char *path, char *buffer, size_t size);
#else
  inline const char *full_path(const char *path, char *buffer, size_t size) {
    (void)buffer; (void)size;
    return path;
  }
#endif
};

extern MCLSd mcl_sd;
extern SdFat_ SD;
