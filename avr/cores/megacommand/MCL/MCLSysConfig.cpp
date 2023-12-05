#include "MCL_impl.h"

void usb_wait() {
  oled_display.clearDisplay();
  oled_display.textbox("PLEASE WAIT", "");
  oled_display.display();
  delay(4000);
}

bool megacmd_check() {
  if (!IS_MEGACMD()) {
    oled_display.textbox("MODE ", "N/A");
    oled_display.display();
    return false;
  }
  return true;
}

void usb_os_update() {
  usb_wait();
  change_usb_mode(USB_SERIAL);
  oled_display.clearDisplay();
  oled_display.textbox("OS UPDATE", "");
  oled_display.display();
  while (1)
    ;
}

void usb_dfu_mode() {
  usb_wait();
  oled_display.clearDisplay();
  oled_display.textbox("DFU ", "MODE");
  oled_display.display();
  //DFU mode is activated via datalines between CPUs
  SET_USB_MODE(USB_DFU);
  while (1)
    ;
}

void usb_disk_mode() {
  if (!megacmd_check()) {
    return;
  }
  usb_wait();
  oled_display.clearDisplay();

  oled_display.textbox("USB ", "DISK");
  oled_display.display();

  LOCAL_SPI_DISABLE();
  EXTERNAL_SPI_ENABLE();
  change_usb_mode(USB_STORAGE);
  while (1)
    ;
}

void mclsys_apply_config() {
  DEBUG_PRINT_FN();
  GUI.display_mirror = mcl_cfg.display_mirror;
  mcl_cfg.write_cfg();
}

void mclsys_apply_config_midi() {
  mclsys_apply_config();
  midi_setup.cfg_ports();
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

  memset((uint8_t *)&version, 0, sizeof(MCLSysConfigData)); //<---- flush zero to config

  version = CONFIG_VERSION;
  //number_projects = 0;
  strncpy(project, my_string, 16);
  //clock_send = 0;
  //clock_rec = 0;
  uart1_turbo_speed = 3;
  uart2_turbo_speed = 3;
  usb_turbo_speed = 3;
  //col = 0;
  //row = 0;
  //cur_row = 0;
  //cur_col = 0;
  memset(&routing, 6, sizeof(routing));
  //poly_mask = 0;
  uart2_ctrl_chan = MIDI_LOCAL_MODE;
  uart2_poly_chan = MIDI_LOCAL_MODE;
  uart2_prg_in = MIDI_LOCAL_MODE;
  uart2_prg_out = MIDI_LOCAL_MODE;
  //mutes = 0;
  //display_mirror = 0;
  //rec_quant = 0;
  tempo = 125;

  //midi_forward_1 = 0;
  //midi_forward_2 = 0;
  //midi_forward_usb = 0;

  rec_automation = 1;
  auto_normalize = 1;
  load_mode = LOAD_MANUAL;
  chain_queue_length = 1;
  chain_load_quant = 16;
  //ram_page_mode = 0;
  track_select = 1;
  track_type_select = 0b00000011;
  //uart2_device = 0;
  //uart_cc_loopback = 0;
  //uart2_prg_mode = 0;
  usb_mode = USB_SERIAL;
  //midi_transport_rec = 0;
  //midi_transport_send = 0;
  midi_ctrl_port = 1;
  //md_trig_channel = 0;
  //seq_dev = 0;
  uart2_cc_mute = 128;
  uart2_cc_level = 128;
  //uart2_turbo = 0;
  cfgfile.close();
  ret = write_cfg();
  if (!ret) {
    return false;
  }
  return true;
}

MCLSysConfig mcl_cfg;
