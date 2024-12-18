/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSYSCONFIG_H__
#define MCLSYSCONFIG_H__

#include "MCLSd.h"
#define CONFIG_VERSION 4011

#define MIDI_OMNI_MODE 17
#define MIDI_LOCAL_MODE 0

extern void mclsys_apply_config();
extern void mclsys_apply_config_midi();

extern void usb_os_update();
extern void usb_dfu_mode();
extern void usb_disk_mode();
extern void mcl_setup();

class MCLSysConfigData {
public:
  uint32_t version;
  char project[16];
  uint8_t number_projects;
  uint8_t uart1_turbo_speed;
  uint8_t uart2_turbo_speed;
  uint8_t usb_turbo_speed;
  uint8_t clock_send;
  uint8_t clock_rec;
  uint8_t drumRouting[16];
  uint8_t routing[24];
  uint8_t row;
  uint8_t col;
  uint8_t cur_row;
  uint8_t cur_col;
  uint16_t poly_mask;
  uint8_t uart2_ctrl_chan;
  uint8_t uart2_poly_chan;
  uint8_t uart2_prg_in;
  uint8_t uart2_prg_out;
  uint8_t uart2_prg_mode;
  uint32_t mutes;
  uint8_t display_mirror;
  uint8_t rec_quant;
  float tempo;

  uint8_t midi_forward_1;
  uint8_t midi_forward_2;
  uint8_t midi_forward_usb;

  uint8_t rec_automation;
  uint8_t load_mode;
  uint8_t chain_queue_length;
  uint8_t chain_load_quant;

  uint8_t auto_normalize;
  uint8_t ram_page_mode;
  uint8_t track_select;
  uint16_t track_type_select;
  uint8_t uart2_device;
  uint8_t uart_cc_loopback;
  uint8_t usb_mode;
  uint8_t midi_transport_rec;
  uint8_t midi_transport_send;
  uint8_t midi_ctrl_port;
  uint8_t md_trig_channel;
  uint8_t seq_dev;
  uint8_t uart2_cc_mute;
  uint8_t uart2_cc_level;
  uint8_t uart1_device;
  uint8_t grid_page_mode;
};

class MCLSysConfig : public MCLSysConfigData {
public:
  uint16_t cfg_save_lastclock = 0;
  File cfgfile;
  bool write_cfg();
  bool cfg_init();
};

extern MCLSysConfig mcl_cfg;

#endif /* MCLSYSCONFIG_H__ */
