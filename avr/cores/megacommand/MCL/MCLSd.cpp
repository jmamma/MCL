#include "MCL_impl.h"
#include "ResourceManager.h"
/*
   Function for initialising the SD Card
*/
SdFat SD;

bool MCLSd::sd_init() {
  bool ret = false;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Initializing SD Card"));
  // File file("/test.mcl",O_WRITE);
  /*Configuration file used to store settings when Minicommand is turned off*/
  for (uint8_t n = 0; n < SD_MAX_RETRIES && ret == false; n++) {
    ret = SD.begin(SD_CS, SPI_FULL_SPEED);
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

  if (BUTTON_DOWN(Buttons.BUTTON2)) {
#ifdef OLED_DISPLAY
    gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
    oled_display.clearDisplay();
    GUI.ignoreNextEvent(Buttons.BUTTON3);
#endif
  }

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
#ifdef OLED_DISPLAY
          gfx.draw_evil(R.icons_boot->evilknievel_bitmap);
          oled_display.clearDisplay();
#endif
          proj.new_project_prompt();
          return true;

        }

        else if (mcl_cfg.number_projects > 0) {
          DEBUG_PRINTLN(
              F("Project count greater than 0, try to load existing"));
          if (!proj.load_project(mcl_cfg.project)) {
            DEBUG_PRINTLN(F("error loading project"));
            proj.new_project_prompt();
            return true;

          } else {
            DEBUG_PRINTLN(F("Project loaded successfully, load grid"));
            return true;
          }
          return true;
        } else {
          proj.new_project_prompt();
          return true;
        }
      } else {
        DEBUG_PRINTLN(F("Could not read cfg file."));

        if (!mcl_cfg.cfg_init()) {
          return false;
        }
        proj.new_project_prompt();
        return true;
      }
    } else {
      DEBUG_PRINTLN(F("Could not open cfg file. Let's try to create it"));
      if (!mcl_cfg.cfg_init()) {
        return false;
      }
#ifdef OLED_DISPLAY
      oled_display.clearDisplay();
#endif
      proj.new_project_prompt();
      return true;
    }
    return true;
  }
  return false;
}

bool MCLSd::seek(uint32_t pos, FatFile *filep) {
  bool pass = false;
  bool ret;
  for (uint8_t n = 0; n < SD_MAX_RETRIES; n++) {
    ret = filep->seekSet(pos);
    if (!ret) {
      DEBUG_PRINTLN("retry");
      delay(5);
      continue;
    }
    pass = true;
    break;
  }
  return pass;
}

bool MCLSd::write_data(void *data, size_t len, FatFile *filep) {

  size_t b;
  bool pass = false;
  bool ret;
  uint32_t pos = filep->curPosition();

  for (uint8_t n = 0; n < SD_MAX_RETRIES; n++) {
    if (n > 0) {
      DEBUG_PRINTLN("retry");
      delay(5);
    }
    if (pos != filep->curPosition()) {
      ret = filep->seekSet(pos);
      if (!ret)
        continue;
    }
    b = filep->write((uint8_t *)data, len);
    if (b != len) {
      write_fail++;
      continue;
    }
    pass = true;
    break;
  }

  return pass;
}
/*
   Function for reading from the project file
*/
bool MCLSd::read_data(void *data, size_t len, FatFile *filep) {

  size_t b;
  bool ret;
  uint32_t pos = filep->curPosition();

  bool pass = false;
  for (uint8_t n = 0; n < SD_MAX_RETRIES; n++) {
    if (n > 0) {
      DEBUG_PRINTLN("retry");
      delay(5);
    }
    if (pos != filep->curPosition()) {
      ret = filep->seekSet(pos);
      if (!ret)
        continue;
    }
    b = filep->read((uint8_t *)data, len);
    if (b != len) {
      read_fail++;
      continue;
    }
    pass = true;
    break;
  }

  return pass;
}

MCLSd mcl_sd;
