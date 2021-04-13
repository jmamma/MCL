/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "helpers.h"
#include "MD.h"
#include "ResourceManager.h"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 *
 * \addtogroup md_params MachineDrum parameters
 *
 * @{
 **/

/** Names for the LFO parameters. **/
const char *MDLFONames[8] = {
  "TRK",
  "PRM",
  "SH1",
  "SH2",
  "TYP",
  "SPD",
  "DPT",
  "MIX"
};

/**
 * Names for the different machine models of the machinedrum.
 **/

#ifndef DISABLE_MACHINE_NAMES

#endif

/// Caller is responsible to make sure machine_names_short is loaded in RM
const char* getMDMachineNameShort(uint8_t machine, uint8_t type) {
  if (machine == 0) {
    if (type == 1) {
      return R.machine_names_short->md_machine_names_short[0].name2;
    }
  }
  return getMachineNameShort(
      machine, type, 
      R.machine_names_short->md_machine_names_short, 
      R.machine_names_short->countof_md_machine_names_short);
}

/// Caller is responsible to make sure machine_names_long is loaded in RM
const char* MDClass::getMachineName(uint8_t machine) {
  for (uint8_t i = 0; i < R.machine_names_long->countof_machine_names; i++) {
    if (R.machine_names_long->machine_names[i].id == machine) {
      return R.machine_names_long->machine_names[i].name;
    }
  }
  return NULL;
}

model_to_param_names_t model_param_names[] = {
  { GND_SN_MODEL, 0 },
  { GND_NS_MODEL, 9 },
  { GND_IM_MODEL, 11},
  { GND_SW_MODEL, 16},
  { GND_PU_MODEL, 25},

  { TRX_BD_MODEL, 34},
  { TRX_B2_MODEL, 43},
  { TRX_SD_MODEL, 61},
  { TRX_XT_MODEL, 70},
  { TRX_CP_MODEL, 78},
  { TRX_RS_MODEL, 87},
  { TRX_CB_MODEL, 91},
  { TRX_CH_MODEL, 98},
  { TRX_OH_MODEL, 104},
  { TRX_CY_MODEL, 110},
  { TRX_MA_MODEL, 117},
  { TRX_CL_MODEL, 126},
  { TRX_XC_MODEL, 133},
  { TRX_S2_MODEL, 52},

  { EFM_BD_MODEL, 141},
  { EFM_SD_MODEL, 150},
  { EFM_XT_MODEL, 159},
  { EFM_CP_MODEL, 168},
  { EFM_RS_MODEL, 177},
  { EFM_CB_MODEL, 186},
  { EFM_HH_MODEL, 194},
  { EFM_CY_MODEL, 203},

  { E12_BD_MODEL, 211},
  { E12_SD_MODEL, 220},
  { E12_HT_MODEL, 229},
  { E12_LT_MODEL, 238},
  { E12_CP_MODEL, 247},
  { E12_RS_MODEL, 256},
  { E12_CB_MODEL, 265},
  { E12_CH_MODEL, 274},
  { E12_OH_MODEL, 283},
  { E12_RC_MODEL, 292},
  { E12_CC_MODEL, 301},
  { E12_BR_MODEL, 310},
  { E12_TA_MODEL, 319},
  { E12_TR_MODEL, 328},
  { E12_SH_MODEL, 337},
  { E12_BC_MODEL, 346},

  { P_I_BD_MODEL, 355},
  { P_I_SD_MODEL, 362},
  { P_I_MT_MODEL, 370},
  { P_I_ML_MODEL, 379},
  { P_I_MA_MODEL, 384},
  { P_I_RS_MODEL, 390},
  { P_I_RC_MODEL, 397},
  { P_I_CC_MODEL, 406},
  { P_I_HH_MODEL, 415},

  { INP_GA_MODEL, 424},
  { INP_FA_MODEL, 430},
  { INP_EA_MODEL, 439},

  { INP_GB_MODEL, 424},
  { INP_FB_MODEL, 430},
  { INP_EB_MODEL, 439},

  { MID_MODEL,    448},

  { CTR_AL_MODEL, 473},
  { CTR_8P_MODEL, 482},

  { CTR_RE_MODEL, 507},
  { CTR_GB_MODEL, 515},
  { CTR_EQ_MODEL, 523},
  { CTR_DX_MODEL, 531},

  { ROM_MODEL,    539},
  { RAM_R1_MODEL, 548},
  { RAM_R2_MODEL, 548},
  { RAM_R3_MODEL, 548},
  { RAM_R4_MODEL, 548}
};

static const char* get_param_name(const model_param_name_t *names, uint8_t param) {
  uint8_t i = 0;
  uint8_t id;
  if (names == NULL)
    return NULL;
  
  while ((id = names[i].id) != 127 && i < 24) {
    if (id == param) {
      return names[i].name ;
    }
    i++;
  }
  return NULL;
}

static uint16_t get_model_param_names(uint8_t model) {
  for (uint16_t i = 0; i < countof(model_param_names); i++) {
    if (model == model_param_names[i].model) {
      return model_param_names[i].offset;
    }
  }
  return 0xFFFF;
}

/// Caller is responsible to make machine_param_names is loaded in RM
const char* model_param_name(uint8_t model, uint8_t param) {
  if (param == 32) {
    return "MUT";
  } else if (param == 33) {
    return "LEV";
  }

  uint16_t model_idx;

  if (model >= MID_MODEL && model <= MID_16_MODEL) {
    model_idx = 448; // midi
  } else if (model >= CTR_8P_MODEL && model < ROM_01_MODEL) {
    model_idx = get_model_param_names(model);
  } else if (param >= 8 || model == 0xFF) {
    model_idx = 557; //generic
  } else if ((model >= ROM_01_MODEL && model <= ROM_32_MODEL) ||
      (model >= ROM_33_MODEL && model <= ROM_48_MODEL))  {
    model_idx = 539; // rom
  } else if (model == RAM_R1_MODEL ||
      model == RAM_R2_MODEL ||
      model == RAM_R3_MODEL ||
      model == RAM_R4_MODEL) {
    model_idx = 548; // ram_r
  } else if (model == RAM_P1_MODEL ||
	model == RAM_P2_MODEL ||
	model == RAM_P3_MODEL ||
	model == RAM_P4_MODEL) {
    model_idx = 539; // rom
  } else {
    model_idx = get_model_param_names(model);
  }

  if (model_idx >= R.machine_param_names->countof_md_model_param_names) {
    return nullptr;
  }
  else {
    return get_param_name(
        R.machine_param_names->md_model_param_names + model_idx, 
        param);
  }
}

uint8_t map_fx_to_model(uint8_t fx_type) {
 if ((fx_type > MD_FX_DYN) || (fx_type < MD_FX_ECHO)) { return 255; }
 return fx_type - MD_FX_ECHO + CTR_RE_MODEL;
}


const char* fx_param_name(uint8_t fx_type, uint8_t param) {
   return model_param_name(map_fx_to_model(fx_type), param);
}

static const uint8_t efm_rs_tuning[] PROGMEM = {
   1,  3, 6, 9, 11, 14, 17, 19, 22, 25, 27, 30, 33, 35, 38, 41, 43,
  46, 49, 51, 54, 57, 59, 62, 65, 67, 70, 73, 75, 78, 81, 83, 86,
  89, 91, 94, 97, 99, 102, 105, 107, 110, 113, 115, 118, 121, 123,
  126
};
static const uint8_t efm_hh_tuning[] PROGMEM = {
  1, 5, 9, 14, 18, 22, 27, 31, 35, 39, 44, 48, 52, 56, 61, 65, 69,
  73, 78, 82, 86, 91, 95, 99, 103, 108, 112, 116, 120, 125
};
static const uint8_t efm_cp_tuning[] PROGMEM = {
  0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 29, 31, 33,
  35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 62, 64, 66,
  68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 95, 97, 99, 101,
  103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127};

static const uint8_t efm_sd_tuning[] PROGMEM = {
  1, 5, 9, 14, 18, 22, 27, 31, 35, 39, 44, 48, 52, 56, 61, 65, 69, 73, 78, 82, 
86, 91, 95, 99, 103, 108, 112, 116, 120, 125, 
};

static const uint8_t efm_xt_tuning[] PROGMEM = {
  1, 7, 12, 17, 23, 28, 33, 39, 44, 49, 55, 60, 65, 71, 76, 81, 87, 92, 97, 102, 
  108, 113, 118, 124, 
};

static const uint8_t efm_bd_tuning[] PROGMEM = {
  1, 3, 6, 9, 11, 14, 17, 19, 22, 25, 27, 30, 33, 35, 38, 41, 43, 46, 49, 51, 54, 
  57, 59, 62, 65, 67, 70, 73, 75, 78, 81, 83, 86, 89, 91, 94, 97, 99, 102, 105, 107, 
  110, 113, 115, 118, 121, 123, 126, 
};
static const uint8_t trx_cl_tuning[] PROGMEM = {
  5, 11, 17, 23, 29, 36, 42, 48, 54, 60, 66, 72, 78, 84, 91, 97, 103, 109, 115, 121, 
  127, 
};
static const uint8_t trx_sd_tuning[] PROGMEM = {
  3, 13, 24, 35, 45, 56, 67, 77, 88, 98, 109, 120, 
};
static const uint8_t trx_xc_tuning[] PROGMEM = {
  1, 6, 11, 17, 22, 27, 33, 38, 43, 49, 54, 60, 65, 70, 76, 81, 86, 92, 97, 102, 
  108, 113, 118, 124, 
};
static const uint8_t trx_xt_tuning[] PROGMEM = {
  2, 7, 12, 18, 23, 28, 34, 39, 44, 49, 55, 60, 65, 71, 76, 81, 87, 92, 97, 103, 
  108, 113, 118, 124,
};
static const uint8_t trx_bd_tuning[] PROGMEM = {
  1, 7, 12, 17, 23, 28, 33, 39, 44, 49, 55, 60, 66, 71, 76, 82, 87, 92, 98, 103, 
  108, 114, 119, 124, 
};

static const uint8_t trx_s2_tuning[] PROGMEM = {
  3, 7, 11, 15, 20, 24, 30, 35, 41, 47, 54, 60, 68, 76, 84, 92, 101, 111, 121
};

static const uint8_t rom_tuning[] PROGMEM = {
  0, 2, 5, 7, 9, 12, 14, 16, 19, 21, 23, 26, 28, 31, 34, 37, 40, 43, 46, 49, 52, 
  55, 58, 61, 64, 67, 70, 73, 76, 79, 82, 85, 88, 91, 94, 97, 100, 102, 105, 107, 
  109, (112), 114, 116, 119, 121, 123, 125, 
};

static const uint8_t gnd_sn_tuning[] PROGMEM = {
  0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15, 16, 18, 19, 20,
  22, 23, 24, 26, 27, 28, 30, 31, 32, 34, 35, 36, 38, 39, 40, 42, 43, 44, 46, 47, 48,
  50, 51, 52, 54, 55, 56, 58, 59, 60, 62, 63, 64, 66, 67, 68, 70, 71, 72, 74, 75, 76,
  78, 79, 80, 82, 83, 84, 86, 87, 88, 90, 91, 92, 94, 95, 96, 98, 99, 100,
  102, 103, 104, 106, 107, 108, 110, 111, 112, 114, 115, 116, 118, 119, 120,
  122, 123, 124, 126, 127
};

static const uint8_t trx_b2_tuning[] PROGMEM = {
  31, 33, 36, 39, 43, 45, 48, 51, 56, 60, 62, 66, 70, 74, 79, 83, 89, 94, 97, 104, 108, 113, 120, 125
};

static const uint8_t trx_rs_tuning[] PROGMEM = {
  2, 10, 17, 26, 36, 45, 55, 66, 78, 90, 103, 117
};

static const uint8_t efm_cb_tuning[] PROGMEM = {
  2, 6, 10, 14, 19, 23, 27, 32, 36, 40, 44, 48, 53, 57, 62, 66, 70, 74, 78, 83,
  87, 91, 96, 100, 104, 108, 113, 117, 121, 126
};

static const uint8_t efm_cy_tuning[] PROGMEM = {
  1, 6, 10, 14, 18, 22, 27, 31, 36, 40, 44, 49, 52, 57, 61, 65, 70, 74, 78, 82,
  87, 91, 95, 100, 103, 108, 112, 116, 121, 125
};

static const uint8_t e12_bc_tuning[] PROGMEM = {
  2, 4, 7, 9, 12, 15, 17, 20, 22, 25, 28, 31, 34, 36, 39, 42, 44, 47, 49, 52, 55, 57, 60, 62, 65,
  68, 71, 74, 76, 79, 82, 84, 87, 89, 92, 95, 97, 100, 103, 106, 108, 111, 114, 116, 119, 122,
  124, 127
};

static const uint8_t e12_cb_tuning[] PROGMEM = {
  1, 4, 6, 9, 12, 14, 17, 19, 22, 25, 27, 30, 33, 36, 39, 41, 44, 46, 49, 52, 54, 57, 59,
  62, 65, 68, 71, 73, 76, 79, 81, 84, 86, 89, 92, 94, 97, 100, 102, 105, 107, 111, 113, 116,
  119, 121, 124, 126
};

static const uint8_t e12_lt_tuning[] PROGMEM = {
  2, 5, 8, 11, 15, 18, 21, 25, 28, 31, 34, 37, 41, 44, 47, 50, 53, 57, 60, 63, 66, 70,
  73, 76, 79, 82, 85, 89, 92, 95, 98, 101, 105, 108, 111, 114, 118, 121, 124, 127
};

static const tuning_t rom_tuning_t = { ROM_MODEL,    45, 
				       sizeof(rom_tuning), 4,   rom_tuning };

static const uint8_t tonal_tuning[] PROGMEM = {
0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46,
48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92,
94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126
};

static const tuning_t rom_tonal_tuning_t = { ROM_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning };


static const tuning_t tunings_tonal[] = {

  { EFM_BD_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_SD_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_XT_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_CP_MODEL, MIDI_NOTE_CS2, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_RS_MODEL, MIDI_NOTE_CS2, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_CB_MODEL, MIDI_NOTE_CS1, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_HH_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { EFM_CY_MODEL, MIDI_NOTE_CS3, sizeof(tonal_tuning), 0, tonal_tuning },

  { TRX_BD_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { TRX_SD_MODEL, MIDI_NOTE_CS1, sizeof(tonal_tuning), 0, tonal_tuning },
  { TRX_XT_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { TRX_RS_MODEL, MIDI_NOTE_CS2, sizeof(tonal_tuning), 0, tonal_tuning },
  { TRX_XC_MODEL, MIDI_NOTE_CS1, sizeof(tonal_tuning), 0, tonal_tuning },
  { TRX_B2_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 0, tonal_tuning },
  { TRX_S2_MODEL, MIDI_NOTE_CS1, sizeof(tonal_tuning), 0, tonal_tuning },

  { GND_SN_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 3, tonal_tuning },
  { GND_SW_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 3, tonal_tuning },
  { GND_PU_MODEL, MIDI_NOTE_CS0, sizeof(tonal_tuning), 3, tonal_tuning },

};

static const tuning_t tunings[] = {
  { EFM_RS_MODEL, MIDI_NOTE_B4, sizeof(efm_rs_tuning), 4, efm_rs_tuning },
  { EFM_HH_MODEL, MIDI_NOTE_B4, sizeof(efm_hh_tuning), 8, efm_hh_tuning },
  { EFM_CP_MODEL, MIDI_NOTE_B3, sizeof(efm_cp_tuning), 3, efm_cp_tuning },
  { EFM_SD_MODEL, MIDI_NOTE_B3, sizeof(efm_sd_tuning), 5, efm_sd_tuning },
  { EFM_XT_MODEL, MIDI_NOTE_F2, sizeof(efm_xt_tuning), 7, efm_xt_tuning },
  { EFM_BD_MODEL, MIDI_NOTE_AB1, sizeof(efm_bd_tuning), 4, efm_bd_tuning },
  { TRX_CL_MODEL, MIDI_NOTE_B6, sizeof(trx_cl_tuning), 7, trx_cl_tuning },
  { TRX_SD_MODEL, MIDI_NOTE_F4, sizeof(trx_sd_tuning), 12, trx_sd_tuning },
  { TRX_XC_MODEL, MIDI_NOTE_F3, sizeof(trx_xc_tuning), 6, trx_xc_tuning },
  { TRX_XT_MODEL, MIDI_NOTE_B3, sizeof(trx_xt_tuning), 6, trx_xt_tuning },
  { TRX_BD_MODEL, MIDI_NOTE_B1, sizeof(trx_bd_tuning), 7, trx_bd_tuning },
  { GND_SN_MODEL, MIDI_NOTE_F2, sizeof(gnd_sn_tuning), 3, gnd_sn_tuning },
  { GND_SW_MODEL, MIDI_NOTE_F2, sizeof(gnd_sn_tuning), 3, gnd_sn_tuning },
  { GND_PU_MODEL, MIDI_NOTE_F2, sizeof(gnd_sn_tuning), 3, gnd_sn_tuning },
  { TRX_B2_MODEL, MIDI_NOTE_A1, sizeof(trx_b2_tuning), 8, trx_b2_tuning },
  { TRX_RS_MODEL, MIDI_NOTE_F4, sizeof(trx_rs_tuning), 13, trx_rs_tuning },
  { TRX_S2_MODEL, MIDI_NOTE_F2, sizeof(trx_s2_tuning), 0, trx_s2_tuning },
  { EFM_CB_MODEL, MIDI_NOTE_F3, sizeof(efm_cb_tuning), 5, efm_cb_tuning },
  { ROM_MODEL,    MIDI_NOTE_A3, sizeof(rom_tuning),    4, rom_tuning    },
  { EFM_CY_MODEL, MIDI_NOTE_B2, sizeof(efm_cy_tuning), 6, efm_cy_tuning },
  { E12_BC_MODEL, MIDI_NOTE_D3, sizeof(e12_bc_tuning), 4, e12_bc_tuning },
  { E12_CB_MODEL, MIDI_NOTE_DS3, sizeof(e12_cb_tuning), 4, e12_cb_tuning },
  { E12_LT_MODEL, MIDI_NOTE_FS5, sizeof(e12_lt_tuning), 4, e12_lt_tuning },
};

tuning_t const *track_tunings[16];

const tuning_t PROGMEM *MDClass::getModelTuning(uint8_t model, bool tonal) {
  uint8_t i;

  if ((model >= 128) && (model <= 191)) {
    //if (tonal) {
    //  return &rom_tonal_tuning_t;
   // }
   // else {
      return &rom_tuning_t;
   // }
  }

  //if ((model >= E12_SD_MODEL) && (model <= E12_BC_MODEL) && (tonal)) {
  //  return &rom_tonal_tuning_t;
  //}


  const tuning_t *t = tunings;
  uint8_t len = countof(tunings);

  if (tonal) {
    t = tunings_tonal;
    len = countof(tunings_tonal);
  }

  for (i = 0; i < len; i++) {
    if (model == t[i].model) {
      return t + i;
    }
  }

  return NULL;
}

/* @} @} */
