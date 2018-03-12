/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLSYSCONFIG_H__
#define MCLSYSCONFIG_H__
#include "MCL.hh"
class MCLSysConfig {
  public:
    uint32_t version;
    char project[16];
    uint8_t number_projects;
    uint8_t uart1_turbo;
    uint8_t uart2_turbo;
    uint8_t clock_send;
    uint8_t clock_rec;
    uint8_t drumRouting[16];
    uint8_t cue_output;
    uint32_t cues;
    uint8_t cur_row;
    uint8_t cur_col;
    bool write_cfg();
    bool cfg_init();
};
extern MCLSysConfig cfg;
extern File cfgfile;

#endif /* MCLSYSCONFIG_H__ */
