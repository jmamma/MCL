#include "MCLSD.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "StackMonitor.h"
#include "Project.h"
#include "PtcGroups.h"

bool MCLSd::join_path(char *dst, size_t dst_len, const char *dir,
                      const char *entry) {
  if (dst_len == 0) {
    return false;
  }
  char *write = dst;
  char *end = dst + dst_len - 1;
  while (*dir != '\0') {
    if (write == end) {
      *dst = '\0';
      return false;
    }
    *write++ = *dir++;
  }
  if (write != dst && write[-1] != '/') {
    if (write == end) {
      *dst = '\0';
      return false;
    }
    *write++ = '/';
  }
  while (*entry != '\0') {
    if (write == end) {
      *dst = '\0';
      return false;
    }
    *write++ = *entry++;
  }
  *write = '\0';
  return true;
}

#ifdef __AVR__
void mcl_oled_set_sd_dedicated_spi(bool dedicated) {
  SD.setDedicatedSpi(dedicated);
}
#endif

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

        if (mcl_cfg.project_config > 1) {
          mcl_cfg.project_config = 0;
        }
        ptc_groups.load(mcl_cfg.ptc_group);

        if (mcl_cfg.number_projects > 0) {
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
    filep->seekSet(pos);
    n++;
  } while (n < SD_MAX_RETRIES);

  return false;
}

/*
   Function for reading from the project file
*/
bool MCLSd::read_data(void *data, size_t len, File *filep) {
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
    filep->seekSet(pos);
    n++;
  } while (n < SD_MAX_RETRIES);

  return false;
}

bool MCLSd::copy_file(const char *src, const char *dst, uint8_t progress_base,
                      uint8_t progress_span, uint8_t progress_max) {
  File in;
  File out;
  if (!in.open(src, O_READ)) {
    return false;
  }
  if (!out.open(dst, O_RDWR | O_CREAT | O_EXCL)) {
    in.close();
    return false;
  }

  bool ok = true;
  uint32_t size = in.size();
  if (size > 0 && !out.preAllocate(size)) {
    ok = false;
  }

  uint8_t buf[512];
  uint32_t copied = 0;
  uint8_t progress_end = progress_base + progress_span;
  while (ok) {
    int n = in.read(buf, sizeof(buf));
    if (n < 0) {
      ok = false;
      break;
    }
    if (n == 0) {
      break;
    }
    ok = write_data(buf, (size_t)n, &out);
    copied += n;
    if (ok && progress_span > 0 && progress_max > 0 && size > 0) {
      uint8_t progress =
          progress_base + ((uint32_t)copied * progress_span) / size;
      if (progress > progress_end) {
        progress = progress_end;
      }
      mcl_gui.draw_progress_bar(progress, progress_max, false, 31, 21);
    }
  }
  if (ok) {
    ok = out.sync();
  }

  out.close();
  in.close();
  if (!ok) {
    SD.remove(dst);
  }
  return ok;
}

bool MCLSd::remove_dir(const char *dir) {
  File d;
  if (!d.open(dir, O_READ) || !d.isDirectory()) {
    d.close();
    return false;
  }
  d.rewind();

  File entry_file;
  char entry[FILE_ENTRY_SIZE];
  char path[128];
  bool ok = true;

  while (entry_file.openNext(&d, O_READ)) {
    entry_file.getName(entry, sizeof(entry));
    bool is_dir = entry_file.isDirectory();
    entry_file.close();

    if (entry[0] == '\0' || entry[0] == '.') {
      continue;
    }
    if (!join_path(path, sizeof(path), dir, entry)) {
      ok = false;
      continue;
    }
    if (is_dir) {
      if (!remove_dir(path)) {
        ok = false;
      }
    } else if (!SD.remove(path)) {
      ok = false;
    }
  }

  d.close();
  if (!SD.rmdir(dir)) {
    ok = false;
  }
  return ok;
}

#ifndef __AVR__
bool MCLSd::copy_dir(const char *src, const char *dst, uint8_t progress_base,
                     uint8_t progress_span, uint8_t progress_max) {
  if (SD.exists(dst)) {
    return false;
  }
  if (!SD.mkdir(dst, true)) {
    return false;
  }

  File dir;
  if (!dir.open(src, O_READ) || !dir.isDirectory()) {
    dir.close();
    remove_dir(dst);
    return false;
  }
  dir.rewind();

  File entry_file;
  char entry[FILE_ENTRY_SIZE];
  char src_path[128];
  char dst_path[128];
  bool ok = true;

  while (ok && entry_file.openNext(&dir, O_READ)) {
    entry_file.getName(entry, sizeof(entry));
    bool is_dir = entry_file.isDirectory();
    entry_file.close();

    if (entry[0] == '\0' || entry[0] == '.') {
      continue;
    }
    if (!join_path(src_path, sizeof(src_path), src, entry) ||
        !join_path(dst_path, sizeof(dst_path), dst, entry)) {
      ok = false;
      break;
    }
    ok = is_dir ? copy_dir(src_path, dst_path, progress_base, progress_span,
                           progress_max)
                : copy_file(src_path, dst_path, progress_base, progress_span,
                            progress_max);
  }

  entry_file.close();
  dir.close();
  if (!ok) {
    remove_dir(dst);
  }
  return ok;
}
#endif

MCLSd mcl_sd;
