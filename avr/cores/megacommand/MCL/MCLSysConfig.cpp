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

  ret = mcl_sd.write_data(( uint8_t*)&data, sizeof(MCLSysConfigData), &cfgfile);
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

  data.version = CONFIG_VERSION;
  data.number_projects = 0;
  m_strncpy(data.project, my_string, 16);
  data.clock_send = 0;
  data.cfg.clock_rec = 0;
  data.cfg.uart1_turbo = 2;
  data.cfg.uart2_turbo = 2;
  data.cur_row = 0;
  data.cur_col = 0;
  data.cues = 0;
  cfgfile.close();

  ret = write_cfg();
  if (!ret) {
    return false;
  }
  return true;
}

MCLSysConfig mcl_cfg; 
