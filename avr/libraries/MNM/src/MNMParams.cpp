#include "WProgram.h"
#include "helpers.h"
#include "MNMParams.hh"
#include "Elektron.hh"
#include "MNM.h"

uint8_t monomachine_sysex_hdr[5] = {
  0x00,
  0x20,
  0x3c,
  0x03, /* monomachine ID */
  0x00 /* base channel padding */
};

mnm_machine_name_t mnm_machine_names[] PROGMEM = {
  { "GND-GND", 0 },
  { "GND-SIN", 1 },
  { "GND-NOIS", 2 },

  { "SWAVE-SAW", 4 },
  { "SWAVE-PULS", 5 },
  { "SWAVE-ENS", 14 },

  { "SID-6581", 3 },
  
  { "DPRO-WAVE", 6 },
  { "DPRO-BBOX", 7 },
  { "DPRO-DDRW", 32 },
  { "DPRO-DENS", 33 },
  
  { "FM+-STAT", 8 },
  { "FM+-PAR", 9 },
  { "FM+-DYN", 10 },
  
  { "VO-VO-6", 11 },
  
  { "FX-THRU", 12 },
  { "FX-REVERB", 13 },
  { "FX-CHORUS", 15 },
  { "FX-DYNAMIX", 16 },
  { "FX-RINGMOD", 17 }
};

PGM_P MNMClass::getMachineName(uint8_t machine) {
  for (uint8_t i = 0; i < countof(mnm_machine_names); i++) {
    if (pgm_read_byte(&mnm_machine_names[i].id) == machine) {
      return mnm_machine_names[i].name;
    }
  }
  return NULL;
}

model_param_name_t mnm_gnd_sin_model_names[] PROGMEM = { {"TUN", 7},
							 {"", 127} };
model_param_name_t mnm_gnd_nois_model_names[] PROGMEM = { {"ST", 0},
							  {"RED", 1},
							  {"TUN", 7},
							  {"", 127} };
model_param_name_t mnm_swave_saw_model_names[] PROGMEM = { {"UNL", 0},
							   {"UNW", 1},
							   {"UNX", 2},
							   {"SBX", 4},
							   {"SB1", 5},
							   {"SB2", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_swave_puls_model_names[] PROGMEM = { {"UNL", 0},
							    {"UNW", 1},
							    {"SB1", 2},
							    {"SB2", 3},
							    {"PW", 4},
							    {"PWD", 5},
							    {"PWR", 6},
							    {"TUN", 7},
							    {"", 127} };
model_param_name_t mnm_swave_ens_model_names[] PROGMEM = { {"PC2", 0},
							   {"PC3", 1},
							   {"PC4", 2},
							   {"WAV", 3},
							   {"PW", 4},
							   {"CHL", 5},
							   {"CHW", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_sid_6581_model_names[] PROGMEM = { {"PW", 0},
							  {"PWD", 1},
							  {"PWR", 2},
							  {"WAV", 3},
							  {"MOD", 4},
							  {"MSR", 5},
							  {"MFQ", 6},
							  {"TUN", 7},
							  {"", 127} };
model_param_name_t mnm_dpro_wave_model_names[] PROGMEM = { {"WAV", 0},
							   {"WP", 1},
							   {"WPM", 2},
							   {"WPR", 3},
							   {"SYN", 4},
							   {"SFQ", 5},
							   {"TUN", 7},
							  {"", 127} };
model_param_name_t mnm_dpro_bbox_model_names[] PROGMEM = { {"PTC", 0},
							   {"STR", 1},
							   {"RTG", 4},
							   {"RTM", 5},
							   {"", 127} };
model_param_name_t mnm_dpro_ddrw_model_names[] PROGMEM = { {"WV1", 0},
							   {"MIX", 1},
							   {"WV2", 2},
							   {"TIM", 3},
							   {"BR1", 4},
							   {"WID", 5},
							   {"BR2", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_dpro_dens_model_names[] PROGMEM = { {"PC2", 0},
							   {"PC3", 1},
							   {"PC4", 2},
							   {"WAV", 3},
							   {"CHL", 5},
							   {"CHW", 6},
							   {"TUN", 7},
							   {"", 127} };

model_param_name_t mnm_fm_stat_model_names[] PROGMEM = { {"1FQ", 0},
							   {"1FI", 1},
							   {"1NV", 2},
							   {"1FB", 3},
							   {"2FQ", 4},
							   {"2VL", 5},
							   {"TON", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_fm_par_model_names[] PROGMEM = { {"1FQ", 0},
							   {"1NV", 1},
							   {"2FQ", 2},
							   {"2NV", 3},
							   {"3FQ", 4},
							   {"3NV", 5},
							   {"TON", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_fm_dyn_model_names[] PROGMEM = { {"1FQ", 0},
							   {"1FN", 1},
							   {"1VL", 2},
							   {"1VN", 3},
							   {"2FQ", 4},
							   {"2NV", 5},
							   {"2FB", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_vo_vo6_model_names[] PROGMEM = { {"VC1", 0},
							   {"VC2", 1},
							   {"VSW", 2},
							   {"VOI", 3},
							   {"CNS", 4},
							   {"CLN", 5},
							   {"VCL", 6},
							   {"TUN", 7},
							   {"", 127} };
model_param_name_t mnm_fx_thru_model_names[] PROGMEM = {   {"INP", 7},
							   {"", 127} };
model_param_name_t mnm_fx_reverb_model_names[] PROGMEM = { {"DEC", 0},
							   {"DMP", 1},
							   {"GAT", 2},
							   {"MIX", 3},
							   {"HP", 4},
							   {"LP", 5},
							   {"INP", 7},
							   {"", 127} };
model_param_name_t mnm_fx_chorus_model_names[] PROGMEM = { {"DEL", 0},
							   {"DEP", 1},
							   {"SPD", 2},
							   {"MIX", 3},
							   {"FB", 4},
							   {"WID", 5},
							   {"LP", 6},
							   {"INP", 7},
							   {"", 127} };
model_param_name_t mnm_fx_dynamix_model_names[] PROGMEM = { {"ATK", 0},
							   {"REL", 1},
							   {"THR", 2},
							   {"MIX", 3},
							   {"RAT", 4},
							   {"GAI", 5},
							   {"RMS", 6},
							   {"INP", 7},
							   {"", 127} };
model_param_name_t mnm_fx_ringmod_model_names[] PROGMEM = { {"WAV", 0},
							    {"EXT", 1},
							    {"MIX", 3},
							    {"INP", 7},
							    {"", 127} };

model_param_name_t mnm_generic_param_names[] PROGMEM = { {"ATK", 0x08 },
							 {"HLD", 0x09 },
							 {"DEC", 0x0a },
							 {"REL", 0x0b },
							 {"DST", 0x0c },
							 {"VOL", 0x0d },
							 {"PAN", 0x0e },
							 {"PRT", 0x0f },

							 {"BAS", 0x10 },
							 {"WDT", 0x11 },
							 {"HPQ", 0x12 },
							 {"LPQ", 0x13 },
							 {"ATK", 0x14 },
							 {"DEC", 0x15 },
							 {"BOF", 0x16 },
							 {"WOF", 0x17 },

							 {"EQF", 0x18 },
							 {"EQG", 0x19 },
							 {"SRR", 0x1a },
							 {"DTI", 0x1b },
							 {"DSN", 0x1c },
							 {"DFB", 0x1d },
							 {"DBS", 0x1e },
							 {"DWI", 0x1f },

							 {"PAG", 0x20 },
							 {"DST", 0x21 },
							 {"TRG", 0x22 },
							 {"WAV", 0x23 },
							 {"MLT", 0x24 },
							 {"SPD", 0x25 },
							 {"INT", 0x26 },
							 {"DPT", 0x27 },

							 {"PAG", 0x28 },
							 {"DST", 0x29 },
							 {"TRG", 0x2a },
							 {"WAV", 0x2b },
							 {"MLT", 0x2c },
							 {"SPD", 0x2d },
							 {"INT", 0x2e },
							 {"DPT", 0x2f },

							 {"PAG", 0x30 },
							 {"DST", 0x31 },
							 {"TRG", 0x32 },
							 {"WAV", 0x33 },
							 {"MLT", 0x34 },
							 {"SPD", 0x35 },
							 {"INT", 0x36 },
							 {"DPT", 0x37 },

							 {"MUT", 100 },
							 {"LEV", 101 },

							 {"", 127}};
model_to_param_names_t mnm_model_param_names[] = {
  { MNM_GND_SIN_MODEL, mnm_gnd_sin_model_names },
  { MNM_GND_NOIS_MODEL, mnm_gnd_nois_model_names },

  { MNM_SID_6581_MODEL, mnm_sid_6581_model_names },

  { MNM_SWAVE_SAW_MODEL, mnm_swave_saw_model_names },
  { MNM_SWAVE_PULS_MODEL, mnm_swave_puls_model_names },
  { MNM_SWAVE_ENS_MODEL, mnm_swave_ens_model_names },

  { MNM_DPRO_WAVE_MODEL, mnm_dpro_wave_model_names },
  { MNM_DPRO_BBOX_MODEL, mnm_dpro_bbox_model_names },
  { MNM_DPRO_DDRW_MODEL, mnm_dpro_ddrw_model_names },
  { MNM_DPRO_DENS_MODEL, mnm_dpro_dens_model_names },

  { MNM_FM_STAT_MODEL, mnm_fm_stat_model_names },
  { MNM_FM_PAR_MODEL, mnm_fm_par_model_names },
  { MNM_FM_DYN_MODEL, mnm_fm_dyn_model_names },

  { MNM_VO_VO6_MODEL, mnm_vo_vo6_model_names },

  { MNM_FX_THRU_MODEL, mnm_fx_thru_model_names },
  { MNM_FX_REVERB_MODEL, mnm_fx_reverb_model_names },
  { MNM_FX_CHORUS_MODEL, mnm_fx_chorus_model_names },
  { MNM_FX_DYNAMIX_MODEL, mnm_fx_dynamix_model_names },
  { MNM_FX_RINGMOD_MODEL, mnm_fx_ringmod_model_names }
};

static PGM_P get_param_name(model_param_name_t *names, uint8_t param) {
  uint8_t i = 0;
  uint8_t id;
  if (names == NULL)
    return NULL;

  while ((id = pgm_read_byte(&names[i].id)) != 127) {
    if (id == param) {
      return names[i].name ;
    }
    i++;
  }
  return NULL;
}

static model_param_name_t *get_model_param_names(uint8_t model) {
  for (uint16_t i = 0; i < countof(mnm_model_param_names); i++) {
    if (model == mnm_model_param_names[i].model) {
      return mnm_model_param_names[i].names;
    }
  }
  return NULL;
}

PGM_P MNMClass::getModelParamName(uint8_t model, uint8_t param) {
  if (param >= 8) {
    return get_param_name(mnm_generic_param_names, param);
  } else {
    return get_param_name(get_model_param_names(model), param);
  }
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
