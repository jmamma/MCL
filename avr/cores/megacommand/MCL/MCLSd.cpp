#include "MCLSd.h"
#include "MCL.h"
/*
   Function for writing to the project file
*/
SdFat SD;

bool MCLSd::load_init() {
  bool ret = false;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Initializing SD Card");
  //File file("/test.mcl",O_WRITE);
  /*Configuration file used to store settings when Minicommand is turned off*/
  for (uint8_t n = 0; n < SD_MAX_RETRIES && ret == false; n++) {
    ret = SD.begin(53, SPI_FULL_SPEED);
    if (!ret) {
      delay(500);
    }
  }
  if (ret == false) {
    DEBUG_PRINTLN("SD Card Initializing failed");
    GUI.flash_strings_fill("SD CARD ERROR", "");
    return false;
  }

  else {
    DEBUG_PRINTLN("SD Init okay");

    if (mcl_cfg.cfgfile.open("/config.mcls", O_RDWR)) {
      DEBUG_PRINTLN("Config file open: success");

      if (read_data(( uint8_t*)&mcl_cfg, sizeof(MCLSysConfigData), &mcl_cfg.cfgfile)) {
        DEBUG_PRINTLN("Config file read: success");

        if (mcl_cfg.version != CONFIG_VERSION) {
          DEBUG_PRINTLN("Incompatible config version");
          if (!mcl_cfg.cfg_init()) {
DEBUG_PRINTLN("Could not init cfg");
            return false;
          }
          GUI.setPage(&new_proj_page);
          return true;

        }

        else if (mcl_cfg.number_projects > 0) {
          DEBUG_PRINTLN("Project count greater than 0, try to load existing");
          if (!proj.load_project(mcl_cfg.project)) {

            GUI.setPage(&new_proj_page);
            return true;

          }
          return true;
        }
        else {
          GUI.setPage(&new_proj_page);
          return true;

        }
      }
      else {
        DEBUG_PRINTLN("Could not read cfg file.");

        if (!mcl_cfg.cfg_init()) {
          return false;
        }
        GUI.setPage(&new_proj_page);
        return true;

      }
    }
    else {
      DEBUG_PRINTLN("Could not open cfg file. Let's try to create it");
      if (!mcl_cfg.cfg_init()) {
        return false;
      }
      GUI.setPage(&new_proj_page);
      return true;

    }
    return true;
  }

}


bool MCLSd::write_data(void *data, size_t len, FatFile *filep) {

  int b;
  bool pass = false;
  bool ret;
  uint32_t pos = filep->curPosition();

  uint8_t n = 0;

  for (n = 0; n < SD_MAX_RETRIES && pass == false; n++) {


    b = filep->write(( uint8_t*) data, len);


    if (b < len) {
      DEBUG_PRINT_FN();
      DEBUG_PRINT("Write Attempt: ");
      DEBUG_PRINTLN(n);
      DEBUG_PRINT("Write failed: ");
      DEBUG_PRINT(b);
      DEBUG_PRINT(" of ");
      DEBUG_PRINTLN(len);
      write_fail++;
      pass = false;
      /*reset position*/
      ret = filep->seekSet(pos);
      if (!ret) {
        DEBUG_PRINTLN("Could not seek, failing");
        return false;
      }
    }
    if (b == len) {
      pass = true;
    }
  }

  if (pass) {
    return true;
  }
  else {
    DEBUG_PRINTLN("Total write failures");
    DEBUG_PRINTLN(write_fail);
    return false;
  }
}
/*
   Function for reading from the project file
*/
bool MCLSd::read_data(void *data, size_t len, FatFile *filep) {

  int b;
  bool pass = false;
  bool ret;
  uint32_t pos = filep->curPosition();

  uint8_t n = 0;

  for (n = 0; n < SD_MAX_RETRIES && pass == false; n++) {


    b = filep->read(( uint8_t*) data, len);


    if (b < len) {
      DEBUG_PRINT_FN();
      DEBUG_PRINT("Read Attempt: ");
      DEBUG_PRINTLN(n);
      DEBUG_PRINT("Read failed: ");
      DEBUG_PRINT(b);
      DEBUG_PRINT(" of ");
      DEBUG_PRINTLN(len);
      read_fail++;

      /*reset position*/
      ret = filep->seekSet(pos);
      if (!ret) {
        DEBUG_PRINTLN("Could not seek, failing");
        return false;
      }
      pass = false;
    }
    if (b == len) {
      pass = true;
    }
  }

  if (pass) {
    return true;
  }
  else {
    DEBUG_PRINTLN("Total read failures");
    DEBUG_PRINTLN(read_fail);

    return false;
  }
}

MCLSd mcl_sd;
