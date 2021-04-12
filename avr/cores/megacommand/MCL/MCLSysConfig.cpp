#include "MCL_impl.h"

void mclsys_apply_config() {
  DEBUG_PRINT_FN();
  mcl_cfg.write_cfg();
  midi_setup.cfg_ports();
#ifndef DEBUGMODE
#ifdef MEGACOMMAND
  if ((!Serial) && (mcl_cfg.display_mirror == 1)) {
    GUI.display_mirror = true;

    Serial.begin(SERIAL_SPEED);
  }
  if ((Serial) && (mcl_cfg.display_mirror == 0)) {
    GUI.display_mirror = false;
    Serial.end();
  }
#endif
#endif
  if (mcl_cfg.screen_saver == 1) {
    GUI.use_screen_saver = true;
  } else {
    GUI.use_screen_saver = false;
  }
}

bool MCLSysConfig::write_cfg() {
  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Writing cfg"));
  cfgfile.close();
  ret = cfgfile.open("/config.mcls", O_RDWR);
  if (!ret) {
    DEBUG_PRINTLN(F("Open cfg file failed"));
    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(MCLSysConfigData), &cfgfile);
  if (!ret) {
    DEBUG_PRINTLN(F("Write cfg failed"));
  }
  DEBUG_PRINTLN(F("Write cfg okay"));
  cfgfile.close();
  cfg_save_lastclock = slowclock;
  return true;
}

bool MCLSysConfig::cfg_init() {
  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Initialising cfgfile"));

  // DEBUG_PRINTLN(F("conf ext"));
  cfgfile.remove();
  ret = cfgfile.createContiguous("/config.mcls", (uint32_t)GRID_SLOT_BYTES);
  if (ret) {
    DEBUG_PRINTLN(F("Created new cfgfile"));
  } else {
    DEBUG_PRINTLN(F("Failed to create new cfgfile"));
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
  memset(&routing, 6, sizeof(routing));
  poly_mask = 0;
  uart2_ctrl_mode = MIDI_LOCAL_MODE;
  mutes = 0;
  display_mirror = 0;
  screen_saver = 1;
  tempo = 125;
  midi_forward = 0;
  auto_save = 1;
  auto_normalize = 1;
  chain_mode = 2;
  chain_rand_min = 0;
  chain_rand_max = 1;
  ram_page_mode = 0;
  track_select = 1;
  track_type_select = 0xF;
  uart2_device = 0;
  cfgfile.close();

  ret = write_cfg();
  if (!ret) {
    return false;
  }
  return true;
}

MCLSysConfig mcl_cfg;
