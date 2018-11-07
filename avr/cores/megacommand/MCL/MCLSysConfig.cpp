#include "MCL.h"
#include "MCLSysConfig.h"

void mclsys_apply_config() {
  DEBUG_PRINT_FN();
  mcl_cfg.write_cfg();
  midi_setup.cfg_ports();
#ifndef DEBUGMODE
  if ((!Serial) && (mcl_cfg.display_mirror == 1)) {
    GUI.display_mirror = true;

    Serial.begin(SERIAL_SPEED);
  }
  if ((Serial) && (mcl_cfg.display_mirror == 0)) {
    GUI.display_mirror = false;
    Serial.end();
  }
#endif
}

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

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(MCLSysConfigData), &cfgfile);
  if (!ret) {
    DEBUG_PRINTLN("Write cfg failed");
  }
  DEBUG_PRINTLN("Write cfg okay");
  cfgfile.close();
  cfg_save_lastclock = slowclock;
  return true;
}

bool MCLSysConfig::cfg_init() {
  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Initialising cfgfile");

  // DEBUG_PRINTLN("conf ext");
  cfgfile.remove();
  ret = cfgfile.createContiguous("/config.mcls", (uint32_t)GRID_SLOT_BYTES);
  if (ret) {
    DEBUG_PRINTLN("Created new cfgfile");
  } else {
    DEBUG_PRINTLN("Failed to create new cfgfile");
    return false;
  }

  char my_string[16] = "/project000.mcl";

  version = CONFIG_VERSION;
  number_projects = 0;
  m_strncpy(project, my_string, 16);
  clock_send = 0;
  clock_rec = 0;
  uart1_turbo = 3;
  uart2_turbo = 3;
  col = 0;
  row = 0;
  cur_row = 0;
  cur_col = 0;
  cues = 0;
  poly_mask = 0;
  uart2_ctrl_mode = MIDI_LOCAL_MODE;
  mutes = 0;
  display_mirror = 0;
  tempo = 125;
  midi_forward = 0;
  auto_merge = 0;
  auto_save = 1;
  chain_mode = 0;
  chain_rand_min = 0;
  chain_rand_max = 1;

  cfgfile.close();

  ret = write_cfg();
  if (!ret) {
    return false;
  }
  return true;
}

MCLSysConfig mcl_cfg;
