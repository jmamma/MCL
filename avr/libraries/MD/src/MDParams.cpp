/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "helpers.h"
#include "MD.h"

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
/*md_machine_name_t machine_names[134] PROGMEM = {
  { "GND---", 0},
  { "GND-SN", 1},
  { "GND-NS", 2},
  { "GND-IM", 3},
  { "TRX-BD", 16},
  { "TRX-SD", 17},
  { "TRX-XT", 18},
  { "TRX-CP", 19},
  { "TRX-RS", 20},
  { "TRX-RS", 21},
  { "TRX-CH", 22},
  { "TRX-OH", 23},
  { "TRX-CY", 24},
  { "TRX-MA", 25},
  { "TRX-CL", 26},
  { "TRX-XC", 27},
  { "TRX-B2", 28},
  { "EFM-BD", 32},
  { "EFM-SD", 33},
  { "EFM-XT", 34},
  { "EFM-CP", 35},
  { "EFM-RS", 36},
  { "EFM-CB", 37},
  { "EFM-HH", 38},
  { "EFM-CY", 39},
  { "E12-BD", 48},
  { "E12-SD", 49},
  { "E12-HT", 50},
  { "E12-LT", 51},
  { "E12-CP", 52},
  { "E12-RS", 53},
  { "E12-CB", 54},
  { "E12-CH", 55},
  { "E12-OH", 56},
  { "E12-RC", 57},
  { "E12-CC", 58},
  { "E12-BR", 59},
  { "E12-TA", 60},
  { "E12-TR", 61},
  { "E12-SH", 62},
  { "E12-BC", 63},
  { "P-I-BD", 64},
  { "P-I-SD", 65},
  { "P-I-MT", 66},
  { "P-I-ML", 67},
  { "P-I-MA", 68},
  { "P-I-RS", 69},
  { "P-I-RC", 70},
  { "P-I-CC", 71},
  { "P-I-HH", 72},
  { "INP-GA", 80},
  { "INP-GB", 81},
  { "INP-FA", 82},
  { "INP-FB", 83},
  { "INP-EA", 84},
  { "INP-EB", 85},
  { "MID-01", 96},
  { "MID-02", 97},
  { "MID-03", 98},
  { "MID-04", 99},
  { "MID-05", 100},
  { "MID-06", 101},
  { "MID-07", 102},
  { "MID-08", 103},
  { "MID-09", 104},
  { "MID-10", 105},
  { "MID-11", 106},
  { "MID-12", 107},
  { "MID-13", 108},
  { "MID-14", 109},
  { "MID-15", 110},
  { "MID-16", 111},
  { "CTR-AL", 112},
  { "CTR-8P", 113},
  { "CTR-RE", 120},
  { "CTR-GB", 121},
  { "CTR-EQ", 122},
  { "CTR-DX", 123},
  { "ROM-01", 128},
  { "ROM-02", 129},
  { "ROM-03", 130},
  { "ROM-04", 131},
  { "ROM-05", 132},
  { "ROM-06", 133},
  { "ROM-07", 134},
  { "ROM-08", 135},
  { "ROM-09", 136},
  { "ROM-10", 137},
  { "ROM-11", 138},
  { "ROM-12", 139},
  { "ROM-13", 140},
  { "ROM-14", 141},
  { "ROM-15", 142},
  { "ROM-16", 143},
  { "ROM-17", 144},
  { "ROM-18", 145},
  { "ROM-19", 146},
  { "ROM-20", 147},
  { "ROM-21", 148},
  { "ROM-22", 149},
  { "ROM-23", 150},
  { "ROM-24", 151},
  { "ROM-25", 152},
  { "ROM-26", 153},
  { "ROM-27", 154},
  { "ROM-28", 155},
  { "ROM-29", 156},
  { "ROM-30", 157},
  { "ROM-31", 158},
  { "ROM-32", 159},
  { "RAM-R1", 160},
  { "RAM-R2", 161},
  { "RAM-P1", 162},
  { "RAM-P2", 163},
  { "RAM-R3", 165},
  { "RAM-R4", 166},
  { "RAM-P3", 167},
  { "RAM-P4", 168},
  { "ROM-33", 176},
  { "ROM-34", 177},
  { "ROM-35", 178},
  { "ROM-36", 179},
  { "ROM-37", 180},
  { "ROM-38", 181},
  { "ROM-39", 182},
  { "ROM-40", 183},
  { "ROM-41", 184},
  { "ROM-42", 185},
  { "ROM-43", 186},
  { "ROM-44", 187},
  { "ROM-45", 188},
  { "ROM-46", 189},
  { "ROM-47", 190},
  { "ROM-48", 191}
};*/

PGM_P MDClass::getMachineName(uint8_t machine) {
//  for (uint8_t i = 0; i < countof(machine_names); i++) {
 //   if (pgm_read_byte(&machine_names[i].id) == machine) {
  //    return machine_names[i].name;
   // }
 // }
  return NULL;
}

model_param_name_t gnd_sn_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "RDC", 3}, {"", 127} };
model_param_name_t gnd_ns_model_names[] PROGMEM = { { "DEC", 0}, {"", 127} };
model_param_name_t gnd_im_model_names[] PROGMEM = { { "UP", 0},
						    { "UVL", 1},
						    { "DWN", 2},
						    { "DVL", 3}, {"", 127} };

model_param_name_t trx_bd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "RDC", 3},
						    { "STR", 4},
						    { "NS", 5},
						    { "HRM", 6},
						    { "CLP", 7}, {"", 127} };
model_param_name_t trx_b2_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "HLD", 3},
						    { "TCK", 4},
						    { "NS", 5},
						    { "DRT", 6},
						    { "DST", 7}, {"", 127} };
model_param_name_t trx_sd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "BMP", 2},
						    { "BNV", 3},
						    { "SNP", 4},
						    { "TON", 5},
						    { "TUN", 6},
						    { "CLP", 7}, {"", 127} };
model_param_name_t trx_xt_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "RDC", 3},
						    { "DMP", 4},
						    { "DST", 5},
						    { "DTP", 6}, {"", 127} };
model_param_name_t trx_cp_model_names[] PROGMEM = { { "CLP", 0},
						    { "TON", 1},
						    { "HRD", 2},
						    { "RCH", 3},
						    { "RAT", 4},
						    { "ROM", 5},
						    { "RSZ", 6},
						    { "RTN", 7}, {"", 127} };
model_param_name_t trx_rs_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "DST", 2}, {"", 127} };
model_param_name_t trx_cb_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 2},
						    { "ENH", 3},
						    { "DMP", 4},
						    { "TON", 5},
						    { "BMP", 6}, {"", 127} };
model_param_name_t trx_ch_model_names[] PROGMEM = { { "GAP", 0},
						    { "DEC", 1},
						    { "HPF", 2},
						    { "LPF", 3},
						    { "MTL", 4}, {"", 127} };
model_param_name_t trx_oh_model_names[] PROGMEM = { { "GAP", 0},
						    { "DEC", 1},
						    { "HPF", 2},
						    { "LPF", 3},
						    { "MTL", 4}, {"", 127} };
model_param_name_t trx_cy_model_names[] PROGMEM = { { "RCH", 0},
						    { "DEC", 1},
						    { "TOP", 2},
						    { "TTU", 3},
						    { "SZ", 4},
						    { "PK", 5}, {"", 127} };
model_param_name_t trx_ma_model_names[] PROGMEM = { { "ATT", 0},
						    { "SUS", 1},
						    { "REV", 2},
						    { "DMP", 3},
						    { "RTL", 4},
						    { "RTP", 5},
						    { "TON", 6},
						    { "HRD", 7}, {"", 127} };
model_param_name_t trx_cl_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "DUA", 2},
						    { "ENH", 3},
						    { "TUN", 4},
						    { "CLC", 5}, {"", 127} };
model_param_name_t trx_xc_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "RDC", 3},
						    { "DMP", 4},
						    { "DST", 5},
						    { "DTP", 6}, {"", 127} };

model_param_name_t efm_bd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "RDC", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6},
						    { "MFB", 7}, {"", 127} };
model_param_name_t efm_sd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "NS", 2},
						    { "NDC", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6},
						    { "HPF", 7}, {"", 127} };
model_param_name_t efm_xt_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "RMP", 2},
						    { "RDC", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6},
						    { "CLC", 7}, {"", 127} };
model_param_name_t efm_cp_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "CLP", 2},
						    { "CDC", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6},
						    { "HPF", 7}, {"", 127} };
model_param_name_t efm_rs_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "MOD", 2},
						    { "HPF", 3},
						    { "SNR", 4},
						    { "SPT", 5},
						    { "SDC", 6},
						    { "SMD", 7}, {"", 127} };
model_param_name_t efm_cb_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "SNP", 2},
						    { "FB", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6}, {"", 127} };
model_param_name_t efm_hh_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "TRM", 2},
						    { "TFQ", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6},
						    { "FB", 7}, {"", 127} };
model_param_name_t efm_cy_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "FB", 2},
						    { "HPF", 3},
						    { "MOD", 4},
						    { "MFQ", 5},
						    { "MDC", 6}, {"", 127} };

model_param_name_t e12_bd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "SNP", 2},
						    { "SPL", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_sd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "RNG", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_ht_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_lt_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "RNG", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_cp_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_rs_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "RTL", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_cb_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_ch_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_oh_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "STP", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_rc_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "BEL", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_cc_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_br_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "REL", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_ta_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_tr_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "HPQ", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_sh_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "SLW", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };
model_param_name_t e12_bc_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HP", 2},
						    { "BC", 3},
						    { "STR", 4},
						    { "RTG", 5},
						    { "RTM", 6},
						    { "BND", 7}, {"", 127} };

model_param_name_t p_i_bd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "HMR", 3},
						    { "TNS", 4},
						    { "DMP", 5}, {"", 127} };
model_param_name_t p_i_sd_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "TNS", 3},
						    { "RVL", 4},
						    { "RDC", 5},
						    { "RNG", 6}, {"", 127} };
model_param_name_t p_i_mt_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "HMR", 3},
						    { "TUN", 4},
						    { "DMP", 5},
						    { "SZ", 6},
						    { "POS", 7}, {"", 127} };
model_param_name_t p_i_ml_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "TNS", 3}, {"", 127} };
model_param_name_t p_i_ma_model_names[] PROGMEM = { { "GRN", 0},
						    { "DEC", 1},
						    { "GLN", 2},
						    { "SZ", 4},
						    { "HRD", 5}, {"", 127} };
model_param_name_t p_i_rs_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "RNG", 3},
						    { "RVL", 4},
						    { "RDC", 5}, {"", 127} };
model_param_name_t p_i_rc_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "RNG", 3},
						    { "AG", 4},
						    { "AU", 5},
						    { "BR", 6},
						    { "GRB", 7}, {"", 127} };
model_param_name_t p_i_cc_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "HRD", 2},
						    { "RNG", 3},
						    { "AG", 4},
						    { "AU", 5},
						    { "BR", 6},
						    { "GRB", 7}, {"", 127} };
model_param_name_t p_i_hh_model_names[] PROGMEM = { { "PTC", 0},
						    { "DEC", 1},
						    { "CLS", 2},
						    { "RNG", 3},
						    { "AG", 4},
						    { "AU", 5},
						    { "BR", 6},
						    { "CLS", 7}, {"", 127} };

model_param_name_t inp_ga_model_names[] PROGMEM = { { "VOL", 0},
						    { "GAT", 1},
						    { "ATK", 2},
						    { "HLD", 3},
						    { "DEC", 4}, {"", 127} };
model_param_name_t inp_fa_model_names[] PROGMEM = { { "ALV", 0},
						    { "GAT", 1},
						    { "FAT", 2},
						    { "FHL", 3},
						    { "FDC", 4},
						    { "FDP", 5},
						    { "FFQ", 6},
						    { "FQ", 7}, {"", 127} };
model_param_name_t inp_ea_model_names[] PROGMEM = { { "AVL", 0},
						    { "AHL", 1},
						    { "ADC", 2},
						    { "FQ", 3},
						    { "FDP", 4},
						    { "FHL", 5},
						    { "FDC", 6},
						    { "FFQ", 7}, {"", 127} };

model_param_name_t mid_model_names[] PROGMEM = { { "NOT", 0},
						 { "N2", 1},
						 { "N3", 2},
						 { "LEN", 3},
						 { "VEL", 4},
						 { "PB", 5},
						 { "MW", 6},
						 { "AT", 7},
						 { "C1D", 8},
						 { "C1V", 9},
						 { "C2D", 10},
						 { "C2V", 11},
						 { "C3D", 12},
						 { "C3V", 13},
						 { "C4D", 14},
						 { "C4V", 15},
						 { "C5D", 16},
						 { "C5V", 17},
						 { "C6D", 18},
						 { "C6V", 19},
						 { "PCG", 20},
						 { "LFS", 21},
						 { "LFD", 22},
						 { "LFM", 23}, {"", 127} };

model_param_name_t ctr_al_model_names[] PROGMEM = { { "SN1", 0},
						    { "SN2", 1},
						    { "SN3", 2},
						    { "SN4", 3},
						    { "SN5", 4},
						    { "SN6", 5},
						    { "SN7", 6},
						    { "SN8", 7}, {"", 127} };
model_param_name_t ctr_8p_model_names[] PROGMEM = { { "P1", 0},
						    { "P2", 1},
						    { "P3", 2},
						    { "P4", 3}, 
						    { "P5", 4},
						    { "P6", 5},
						    { "P7", 6},
						    { "P8", 7},
						    { "P1T", 8},
						    { "P1P", 9},
						    { "P2T", 10},
						    { "P2P", 11},
						    { "P3T", 12},
						    { "P3P", 13},
						    { "P4T", 14},
						    { "P4P", 15},
						    { "P5T", 16},
						    { "P5P", 17},
						    { "P6T", 18},
						    { "P6P", 19},
						    { "P7T", 20},
						    { "P7P", 21},
						    { "P8T", 22},
						    { "P8P", 23}, {"", 127} };
model_param_name_t rom_model_names[] PROGMEM = { { "PTC", 0},
						 { "DEC", 1},
						 { "HLD", 2},
						 { "BRR", 3},
						 { "STR", 4},
						 { "END", 5},
						 { "RTG", 6},
						 { "RTM", 7}, {"", 127} };
model_param_name_t ram_r_model_names[] PROGMEM = { { "MLV", 0},
						   { "MBL", 1},
						   { "ILV", 2},
						   { "IBL", 3},
						   { "CU1", 4},
						   { "CU2", 5},
						   { "LEN", 6},
						   { "RAT", 7}, {"", 127} };

model_param_name_t generic_param_names[] PROGMEM = { { "P1", 0},
						     { "P2", 1},
						     { "P3", 2},
						     { "P4", 3},
						     { "P5", 4},
						     { "P6", 5},
						     { "P7", 6},
						     { "P8", 7},
						     { "AMD", 8 },
						      { "AMF", 9 },
						      { "EQF", 10 },
						      { "EQG", 11 },
						      { "FLF", 12 },
						      { "FLW", 13 },
						      { "FLQ", 14 },
						      { "SRR", 15 },
						      { "DST", 16 },
						      { "VOL", 17 },
						      { "PAN", 18 },
						      { "DEL", 19 },
						      { "REV", 20 },
						      { "LFS", 21 },
						      { "LFD", 22 },
						      { "LFM", 23 }, { "", 127 } };
  
model_to_param_names_t model_param_names[] = {
  { GND_SN_MODEL, gnd_sn_model_names },
  { GND_NS_MODEL, gnd_ns_model_names },
  { GND_IM_MODEL, gnd_im_model_names },
  
  { TRX_BD_MODEL, trx_bd_model_names },
  { TRX_B2_MODEL, trx_b2_model_names },
  { TRX_SD_MODEL, trx_sd_model_names },
  { TRX_XT_MODEL, trx_xt_model_names },
  { TRX_CP_MODEL, trx_cp_model_names },
  { TRX_RS_MODEL, trx_rs_model_names },
  { TRX_CH_MODEL, trx_ch_model_names },
  { TRX_OH_MODEL, trx_oh_model_names },
  { TRX_CY_MODEL, trx_cy_model_names },
  { TRX_MA_MODEL, trx_ma_model_names },
  { TRX_CL_MODEL, trx_cl_model_names },
  { TRX_XC_MODEL, trx_xc_model_names },

  { EFM_BD_MODEL, efm_bd_model_names },
  { EFM_SD_MODEL, efm_sd_model_names },
  { EFM_XT_MODEL, efm_xt_model_names },
  { EFM_CP_MODEL, efm_cp_model_names },
  { EFM_RS_MODEL, efm_rs_model_names },
  { EFM_CB_MODEL, efm_cb_model_names },
  { EFM_HH_MODEL, efm_hh_model_names },
  { EFM_CY_MODEL, efm_cy_model_names },

  { E12_BD_MODEL, e12_bd_model_names },
  { E12_SD_MODEL, e12_sd_model_names },
  { E12_HT_MODEL, e12_ht_model_names },
  { E12_LT_MODEL, e12_lt_model_names },
  { E12_CP_MODEL, e12_cp_model_names },
  { E12_RS_MODEL, e12_rs_model_names },
  { E12_CB_MODEL, e12_cb_model_names },
  { E12_CH_MODEL, e12_ch_model_names },
  { E12_OH_MODEL, e12_oh_model_names },
  { E12_RC_MODEL, e12_rc_model_names },
  { E12_CC_MODEL, e12_cc_model_names },
  { E12_BR_MODEL, e12_br_model_names },
  { E12_TA_MODEL, e12_ta_model_names },
  { E12_TR_MODEL, e12_tr_model_names },
  { E12_SH_MODEL, e12_sh_model_names },
  { E12_BC_MODEL, e12_bc_model_names },

  { P_I_BD_MODEL, p_i_bd_model_names },
  { P_I_SD_MODEL, p_i_sd_model_names },
  { P_I_MT_MODEL, p_i_mt_model_names },
  { P_I_ML_MODEL, p_i_ml_model_names },
  { P_I_MA_MODEL, p_i_ma_model_names },
  { P_I_RS_MODEL, p_i_rs_model_names },
  { P_I_RC_MODEL, p_i_rc_model_names },
  { P_I_CC_MODEL, p_i_cc_model_names },
  { P_I_HH_MODEL, p_i_hh_model_names },

  { INP_GA_MODEL, inp_ga_model_names },
  { INP_FA_MODEL, inp_fa_model_names },
  { INP_EA_MODEL, inp_ea_model_names },

  { INP_GB_MODEL, inp_ga_model_names },
  { INP_FB_MODEL, inp_fa_model_names },
  { INP_EB_MODEL, inp_ea_model_names },

  { MID_MODEL,    mid_model_names },

  { CTR_AL_MODEL, ctr_al_model_names },
  { CTR_8P_MODEL, ctr_8p_model_names },
  { ROM_MODEL,    rom_model_names },
  { RAM_R1_MODEL,  ram_r_model_names },
  { RAM_R2_MODEL,  ram_r_model_names },
  { RAM_R3_MODEL,  ram_r_model_names },
  { RAM_R4_MODEL,  ram_r_model_names }
};

static PGM_P get_param_name(model_param_name_t *names, uint8_t param) {
  uint8_t i = 0;
  uint8_t id;
  if (names == NULL)
    return NULL;
  
  while ((id = pgm_read_byte(&names[i].id)) != 127 && i < 24) {
    if (id == param) {
      return names[i].name ;
    }
    i++;
  }
  return NULL;
}

static model_param_name_t *get_model_param_names(uint8_t model) {
  for (uint16_t i = 0; i < countof(model_param_names); i++) {
    if (model == model_param_names[i].model) {
      return model_param_names[i].names;
    }
  }
  return NULL;
}

PGM_P model_param_name(uint8_t model, uint8_t param) {
  if (param == 32) {
    return PSTR("MUT");
  } else if (param == 33) {
    return PSTR("LEV");
  }

  if (model >= MID_MODEL && model <= MID_16_MODEL) {
    return get_param_name(mid_model_names, param);
  }
  if (model >= CTR_8P_MODEL && model < ROM_01_MODEL) {
    return get_param_name(get_model_param_names(model), param);
  }

  if (param >= 8) {
    return get_param_name(generic_param_names, param);
  } else {
    if (model == 0xFF) {
      return get_param_name(generic_param_names, param);
    }
  
    if ((model >= ROM_01_MODEL && model <= ROM_32_MODEL) ||
	(model >= ROM_33_MODEL && model <= ROM_48_MODEL)) {
      return get_param_name(rom_model_names, param);
    }
    if (model == RAM_R1_MODEL ||
	model == RAM_R2_MODEL ||
	model == RAM_R3_MODEL ||
	model == RAM_R4_MODEL) {
      return get_param_name(ram_r_model_names, param);
    }
    if (model == RAM_P1_MODEL ||
	model == RAM_P2_MODEL ||
	model == RAM_P3_MODEL ||
	model == RAM_P4_MODEL) {
      return get_param_name(rom_model_names, param);
    }
    return get_param_name(get_model_param_names(model), param);
  }
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
  { TRX_B2_MODEL, MIDI_NOTE_A1, sizeof(trx_b2_tuning), 8, trx_b2_tuning },
  { TRX_RS_MODEL, MIDI_NOTE_F4, sizeof(trx_rs_tuning), 13, trx_rs_tuning },
  { EFM_CB_MODEL, MIDI_NOTE_F3, sizeof(efm_cb_tuning), 5, efm_cb_tuning },
  { ROM_MODEL,    MIDI_NOTE_A3, sizeof(rom_tuning),    4, rom_tuning    },
  { EFM_CY_MODEL, MIDI_NOTE_B2, sizeof(efm_cy_tuning), 6, efm_cy_tuning },
  { E12_BC_MODEL, MIDI_NOTE_D3, sizeof(e12_bc_tuning), 4, e12_bc_tuning },
  { E12_CB_MODEL, MIDI_NOTE_DS3, sizeof(e12_cb_tuning), 4, e12_cb_tuning },
  { E12_LT_MODEL, MIDI_NOTE_FS5, sizeof(e12_lt_tuning), 4, e12_lt_tuning },
};

tuning_t const *track_tunings[16];

const tuning_t PROGMEM *MDClass::getModelTuning(uint8_t model) {
  uint8_t i;
  if (((model >= 128) && (model <= 159))) {
    return &rom_tuning_t;
  }
  for (i = 0; i < countof(tunings); i++) {
    if (model == tunings[i].model) {
      return tunings + i;
    }
  }

  return NULL;
}

/* @} @} */
