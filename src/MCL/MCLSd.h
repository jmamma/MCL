/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#define SD_MAX_RETRIES 10

#if defined(PLATFORM_TBD)

#define SD_CONFIG SdioConfig(RP_CLK_GPIO, RP_CMD_GPIO, RP_DAT0_GPIO)

#else

#define SD_CONFIG SdSpiConfig(SPI1_SS_PIN, DEDICATED_SPI, SD_SCK_MHZ(12), &SPI1)

#endif

#include "SdFat.h"
#include "hardware.h"
typedef FsFile File;

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
      digitalWrite(SPI1_SS_PIN, HIGH);   // SDCard enable is active low
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
  bool sd_init();
  bool load_init();
  bool seek(uint32_t pos, File *filep);
  bool read_data(void *data, size_t len, File *filep);
  bool write_data(void *data, size_t len, File *filep);
};

extern MCLSd mcl_sd;
extern SdFat_ SD;
