#include "MCLSD.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "StackMonitor.h"
#include "Project.h"
/*
   Function for initialising the SD Card
*/

#ifdef __AVR__
SdFat_ SD __attribute__((section(".sdcard")));
#else
SdFat_ SD;
#endif

bool MCLSd::sd_init() {
  bool ret = false;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Initializing SD Card"));
  // File file("/test.mcl",O_WRITE);
  /*Configuration file used to store settings when Minicommand is turned off*/
  for (uint8_t n = 0; n < SD_MAX_RETRIES && ret == false; n++) {
    SPI1.setRX(SDCARD_MISO_PIN);
    SPI1.setTX(SDCARD_MOSI_PIN);
    SPI1.setSCK(SDCARD_SCK_PIN);
    ret = SD.begin(SdSpiConfig(SDCARD_SS_PIN, DEDICATED_SPI, SD_SCK_MHZ(25), &SPI1));
    //ret = SD.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(50)));
    if (!ret) {
      delay(50);
    }
  }
  if (ret == false) {
    sd_state = false;
    DEBUG_PRINTLN(F("SD Init fail"));
    return false;
  }
  sd_state = true;

  DEBUG_PRINTLN(F("SD Init okay"));
  return true;
}
bool MCLSd::load_init() {
  bool ret = false;
  int b;

  if (sd_state) {

    if (mcl_cfg.cfgfile.open("/config.mcls", O_RDWR)) {
      DEBUG_PRINTLN(F("Config file open: success"));

      if (read_data((uint8_t *)&mcl_cfg, sizeof(MCLSysConfigData),
                    &mcl_cfg.cfgfile)) {
        DEBUG_PRINTLN(F("Config file read: success"));

        if (mcl_cfg.version != CONFIG_VERSION) {
          DEBUG_PRINTLN(F("Incompatible config version"));
          if (!mcl_cfg.cfg_init()) {
            DEBUG_PRINTLN(F("Could not init cfg"));
            return false;
          }
          gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
          oled_display.clearDisplay();
          mcl_gui.wait_for_project();
          return true;

        }

        else if (mcl_cfg.number_projects > 0) {
          DEBUG_PRINTLN(
              F("Project count greater than 0, try to load existing"));
          if (!proj.load_project(mcl_cfg.project)) {
            DEBUG_PRINTLN(F("error loading project"));
            mcl_gui.wait_for_project();
            return true;

          } else {
            DEBUG_PRINTLN(F("Project loaded successfully, load grid"));
            return true;
          }
          return true;
        } else {
          mcl_gui.wait_for_project();
          return true;
        }
      } else {
        DEBUG_PRINTLN(F("Could not read cfg file."));

        if (!mcl_cfg.cfg_init()) {
          return false;
        }
        mcl_gui.wait_for_project();
        return true;
      }
    } else {
      DEBUG_PRINTLN(F("Could not open cfg file. Let's try to create it"));
      if (!mcl_cfg.cfg_init()) {
        return false;
      }
      oled_display.clearDisplay();
      mcl_gui.wait_for_project();
      return true;
    }
    return true;
  }
  return false;
}

bool MCLSd::seek(uint32_t pos, File *filep) {
  bool ret;
  uint8_t n = 0;
  if (!filep) {
    DEBUG_PRINTLN(F("huh"));
    return false;
  }

  do {
    ret = filep->seekSet(pos);
    if (ret) {
      return true;
    }
    DEBUG_PRINTLN("seek retry");
    DEBUG_PRINTLN(pos);
    delay(20);
    n++;
  } while (n < SD_MAX_RETRIES);

  return false;
}

bool MCLSd::write_data(void *data, size_t len, File *filep) {
  bool ret;
  uint32_t pos = filep->curPosition();
  uint8_t n = 0;

  do {
    size_t b = filep->write((uint8_t *)data, len);
    if (b == len) {
      return true;
    }
    DEBUG_PRINTLN("write retry");
    delay(20);
    write_fail++;
    ret = filep->seekSet(pos);
    n++;
  } while (n < SD_MAX_RETRIES);

  return false;
}

/*
   Function for reading from the project file
*/
bool MCLSd::read_data(void *data, size_t len, File *filep) {
  bool ret;
  uint32_t pos = filep->curPosition();
  uint8_t n = 0;

  do {
    size_t b = filep->read((uint8_t *)data, len);
    if (b == len) {
      return true;
    }
    DEBUG_PRINTLN("read retry");
    delay(20);
    read_fail++;
    ret = filep->seekSet(pos);
    n++;
  } while (n < SD_MAX_RETRIES);

  return false;
}

MCLSd mcl_sd;
