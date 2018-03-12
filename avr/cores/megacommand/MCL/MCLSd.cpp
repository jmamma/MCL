#include "MCLSd.h"

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

    if (cfgfile.open("/config.mcls", O_RDWR)) {
      DEBUG_PRINTLN("Config file open: success");

      if (sd_read_data(( uint8_t*)&cfg, sizeof(Config), &cfgfile)) {
        DEBUG_PRINTLN("Config file read: success");

        if (cfg.version != CONFIG_VERSION) {
          DEBUG_PRINTLN("Incompatible config version");
          if (!cfg_init()) {
            return false;
          }
          new_project_page();
          return true;

        }

        else if (cfg.number_projects > 0) {
          DEBUG_PRINTLN("Project count greater than 0, try to load existing");
          if (!sd_load_project(cfg.project)) {

            new_project_page();
            return true;

          }
          return true;
        }
        else {
          new_project_page();
          return true;

        }
      }
      else {
        DEBUG_PRINTLN("Could not read cfg file.");

        if (!cfg_init()) {
          return false;
        }
        new_project_page();
        return true;

      }
    }
    else {
      DEBUG_PRINTLN("Could not open cfg file. Let's try to create it");
      if (!cfg_init()) {
        return false;
      }
      new_project_page();
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
      sd_write_fail++;
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
    DEBUG_PRINTLN(sd_write_fail);
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
      sd_read_fail++;

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
    DEBUG_PRINTLN(sd_read_fail);

    return false;
  }
}

MCLSd mcl_sd;
