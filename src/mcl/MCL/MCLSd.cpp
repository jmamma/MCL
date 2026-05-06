#include "MCLSD.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "StackMonitor.h"
#include "Project.h"
/*
   Function for initialising the SD Card
*/

bool MCLSd::sd_init() {
  bool ret = false;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Initializing SD Card"));
  // File file("/test.mcl",O_WRITE);
  /*Configuration file used to store settings when Minicommand is turned off*/
  for (uint8_t n = 0; n < SD_MAX_RETRIES && ret == false; n++) {

    ret = SD.begin(SD_CONFIG);
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

#ifndef __AVR__
  if (SD.exists("/config.mcls")) {
    strcpy(mcl_root, "");
    DEBUG_PRINTLN(F("Root is /"));
  } else {
    strcpy(mcl_root, "/MCL");
    DEBUG_PRINTLN(F("Root is /MCL"));
    if (!SD.exists(mcl_root)) {
      SD.mkdir(mcl_root, true);
      char buf[64];
      strcpy(buf, mcl_root); strcat(buf, "/Projects");
      SD.mkdir(buf, true);
      strcpy(buf, mcl_root); strcat(buf, "/Samples");
      SD.mkdir(buf, true);
      strcpy(buf, mcl_root); strcat(buf, "/Samples/WAV");
      SD.mkdir(buf, true);
      strcpy(buf, mcl_root); strcat(buf, "/Samples/SYX");
      SD.mkdir(buf, true);
      strcpy(buf, mcl_root); strcat(buf, "/Sounds");
      SD.mkdir(buf, true);
    }
  }
#endif

  DEBUG_PRINTLN(F("SD Init okay"));
  return true;
}

#ifndef __AVR__
const char *MCLSd::full_path(const char *path, char *buffer, size_t size) {
  if (mcl_root[0] == '\0') return path;
  if (path[0] != '/') return path;

  size_t root_len = strlen(mcl_root);
  if (strncmp(path, mcl_root, root_len) == 0 &&
      (path[root_len] == '\0' || path[root_len] == '/')) {
    return path;
  }

  if (size == 0) return path;
  strncpy(buffer, mcl_root, size);
  buffer[size - 1] = '\0';
  if (path[1] != '\0') {
    strncat(buffer, path, size - strlen(buffer) - 1);
  }
  return buffer;
}
#endif
bool MCLSd::load_init() {
  bool ret = false;
  int b;

  if (sd_state) {
    char path[64];
    if (mcl_cfg.cfgfile.open(full_path("/config.mcls", path, sizeof(path)), O_RDWR)) {
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
