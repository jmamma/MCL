/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSYSCONFIG_H__
#define MCLSYSCONFIG_H__

#include "SdFat.h"
#define CONFIG_VERSION 4000

#define MIDI_OMNI_MODE 17
#define MIDI_LOCAL_MODE 0

extern void mclsys_apply_config();
extern void mclsys_apply_config_midi();

class MCLSysConfigData {
public:
  uint32_t version;
  char project[16];
  uint8_t number_projects;
  uint8_t uart1_turbo;
  uint8_t uart2_turbo;
  uint8_t clock_send;
  uint8_t clock_rec;
  uint8_t drumRouting[16];
  uint8_t routing[24];
  uint8_t row;
  uint8_t col;
  uint8_t cur_row;
  uint8_t cur_col;
  uint16_t poly_mask;
  uint8_t uart2_ctrl_mode;
  uint32_t mutes;
  uint8_t display_mirror;
  uint8_t screen_saver;
  float tempo;
  uint8_t midi_forward;
  uint8_t auto_save;
  uint8_t chain_mode;
  uint8_t chain_queue_length;
  uint8_t chain_load_quant;

  uint8_t auto_normalize;
  uint8_t ram_page_mode;
  uint8_t track_select;
  uint16_t track_type_select;
  uint8_t uart2_device;

  //to be deleted
  uint8_t link_rand_min;
  uint8_t link_rand_max;
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
