#include "WProgram.h"
#include "helpers.h"
#include "MNMParams.h"
#include "Elektron.h"
#include "MNM.h"
#include "ResourceManager.h"

uint8_t monomachine_sysex_hdr[5] = {
  0x00,
  0x20,
  0x3c,
  0x03, /* monomachine ID */
  0x00 /* base channel padding */
};

/// Caller is responsible to make sure machine_names_short is loaded in RM
const char* getMNMMachineNameShort(uint8_t machine, uint8_t type) {
  if (machine == 0) {
    if (type == 1) {
      return R.machine_names_short->mnm_machine_names_short[0].name2;
    }
  }
  return getMachineNameShort(
      machine, type, 
      R.machine_names_short->mnm_machine_names_short, 
      R.machine_names_short->countof_mnm_machine_names_short);
}

/// Caller is responsible to make sure machine_names_long is loaded in RM
const char* MNMClass::getMachineName(uint8_t machine) {
  for (uint8_t i = 0; i < R.machine_names_long->countof_mnm_machine_names; i++) {
    if (R.machine_names_long->mnm_machine_names[i].id == machine) {
      return R.machine_names_long->mnm_machine_names[i].name;
    }
  }
  return NULL;
}

model_to_param_names_t mnm_model_param_names[] = {
  { MNM_GND_SIN_MODEL,    0},
  { MNM_GND_NOIS_MODEL,   2},

  { MNM_SID_6581_MODEL,   6},

  { MNM_SWAVE_SAW_MODEL,  14},
  { MNM_SWAVE_PULS_MODEL, 23},
  { MNM_SWAVE_ENS_MODEL,  32},

  { MNM_DPRO_WAVE_MODEL,  41},
  { MNM_DPRO_BBOX_MODEL,  49},
  { MNM_DPRO_DDRW_MODEL,  54},
  { MNM_DPRO_DENS_MODEL,  63},

  { MNM_FM_STAT_MODEL,    71},
  { MNM_FM_PAR_MODEL,     80},
  { MNM_FM_DYN_MODEL,     89},

  { MNM_VO_VO6_MODEL,     98},

  { MNM_FX_THRU_MODEL,    107},
  { MNM_FX_REVERB_MODEL,  109},
  { MNM_FX_CHORUS_MODEL,  117},
  { MNM_FX_DYNAMIX_MODEL, 126},
  { MNM_FX_RINGMOD_MODEL, 135}
};

static const char* get_param_name(const model_param_name_t *names, uint8_t param) {
  uint8_t i = 0;
  uint8_t id;
  if (names == NULL)
    return NULL;

  while ((id = names[i].id) != 127) {
    if (id == param) {
      return names[i].name ;
    }
    i++;
  }
  return NULL;
}

static uint16_t get_model_param_names(uint8_t model) {
  for (uint16_t i = 0; i < countof(mnm_model_param_names); i++) {
    if (model == mnm_model_param_names[i].model) {
      return mnm_model_param_names[i].offset;
    }
  }
  return 0xFFFF;
}

const char* MNMClass::getModelParamName(uint8_t model, uint8_t param) {
  uint16_t idx;
  if (param >= 8) {
    idx = 140; // generic
  } else {
    idx = get_model_param_names(model);
  }

  if (idx == 0xFFFF) return nullptr;
  else return get_param_name(
      R.machine_param_names->mnm_model_param_names + idx,
      param);
}

void MNMClass::getPatternName(uint8_t pattern, char str[5]) {
  uint8_t bank = pattern / 16;
  uint8_t num = pattern % 16 + 1;
  str[0] = 'A' + bank;
  str[1] = '0' + (num / 10);
  str[2] = '0' + (num % 10);
  str[3] = ' ';
  str[4] = 0;
}
