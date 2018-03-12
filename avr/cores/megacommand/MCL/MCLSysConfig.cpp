#include "MCLSysConfig.hh"

bool MCLSysConfig::write_cfg() {
  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Writing cfg");

  cfgfile.close();
  ret = cfgfile.open("/config.mcls", O_RDWR);
  if (!ret) {
    DEBUG_PRINTLN("Open cfg file failed");
    return false;
  }

  ret = sd_write_data(( uint8_t*)&cfg, sizeof(Config), &cfgfile);
  if (!ret) {
    DEBUG_PRINTLN("Write cfg failed");
  }
  DEBUG_PRINTLN("Write cfg okay");
  cfgfile.close();
  cfg_save_lastclock = slowclock;
  return true;
}

bool MCLSysConfig::init() {
  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Initialising cfgfile");

  //DEBUG_PRINTLN("conf ext");
  cfgfile.remove();
  ret = cfgfile.createContiguous("/config.mcls", (uint32_t) GRID_SLOT_BYTES);
  if (ret) {
    DEBUG_PRINTLN("Created new cfgfile");
  }
  else {
    DEBUG_PRINTLN("Failed to create new cfgfile");
    return false;
  }

  char my_string[16] = "/project000.mcl";

  cfg.version = CONFIG_VERSION;
  cfg.number_projects = 0;
  m_strncpy(cfg.project, my_string, 16);
  cfg.clock_send = 0;
  cfg.clock_rec = 0;
  cfg.uart1_turbo = 2;
  cfg.uart2_turbo = 2;
  cur_row = 0;
  cur_col = 0;
  cfg.cues = 0;
  cfgfile.close();

  ret = write_cfg();
  if (!ret) {
    return false;
  }
  return true;
}
File cfgfile;
MCLSysConfig cfg;
