#include "Elektron.h"

short_machine_name_t md_machine_names_short[137] = {
    {"GN", "--", 0},   {"GN", "SN", 1},   {"GN", "NS", 2},   {"GN", "IM", 3},
    {"GN", "SW", 4},   {"GN", "PU", 5},   {"TR", "BD", 16},  {"TR", "SD", 17},
    {"TR", "XT", 18},  {"TR", "CP", 19},  {"TR", "RS", 20},  {"TR", "RS", 21},
    {"TR", "CH", 22},  {"TR", "OH", 23},  {"TR", "CY", 24},  {"TR", "MA", 25},
    {"TR", "CL", 26},  {"TR", "XC", 27},  {"TR", "B2", 28},  {"TR", "S2", 29},
    {"FM", "BD", 32},  {"FM", "SD", 33},  {"FM", "XT", 34},  {"FM", "CP", 35},
    {"FM", "RS", 36},  {"FM", "CB", 37},  {"FM", "HH", 38},  {"FM", "CY", 39},
    {"E2", "BD", 48},  {"E2", "SD", 49},  {"E2", "HT", 50},  {"E2", "LT", 51},
    {"E2", "CP", 52},  {"E2", "RS", 53},  {"E2", "CB", 54},  {"E2", "CH", 55},
    {"E2", "OH", 56},  {"E2", "RC", 57},  {"E2", "CC", 58},  {"E2", "BR", 59},
    {"E2", "TA", 60},  {"E2", "TR", 61},  {"E2", "SH", 62},  {"E2", "BC", 63},
    {"PI", "BD", 64},  {"PI", "SD", 65},  {"PI", "MT", 66},  {"PI", "ML", 67},
    {"PI", "MA", 68},  {"PI", "RS", 69},  {"PI", "RC", 70},  {"PI", "CC", 71},
    {"PI", "HH", 72},  {"IN", "GA", 80},  {"IN", "GB", 81},  {"IN", "FA", 82},
    {"IN", "FB", 83},  {"IN", "EA", 84},  {"IN", "EB", 85},  {"MI", "01", 96},
    {"MI", "02", 97},  {"MI", "03", 98},  {"MI", "04", 99},  {"MI", "05", 100},
    {"MI", "06", 101}, {"MI", "07", 102}, {"MI", "08", 103}, {"MI", "09", 104},
    {"MI", "10", 105}, {"MI", "11", 106}, {"MI", "12", 107}, {"MI", "13", 108},
    {"MI", "14", 109}, {"MI", "15", 110}, {"MI", "16", 111}, {"CT", "AL", 112},
    {"CT", "8P", 113}, {"CT", "RE", 120}, {"CT", "GB", 121}, {"CT", "EQ", 122},
    {"CT", "DX", 123}, {"RO", "01", 128}, {"RO", "02", 129}, {"RO", "03", 130},
    {"RO", "04", 131}, {"RO", "05", 132}, {"RO", "06", 133}, {"RO", "07", 134},
    {"RO", "08", 135}, {"RO", "09", 136}, {"RO", "10", 137}, {"RO", "11", 138},
    {"RO", "12", 139}, {"RO", "13", 140}, {"RO", "14", 141}, {"RO", "15", 142},
    {"RO", "16", 143}, {"RO", "17", 144}, {"RO", "18", 145}, {"RO", "19", 146},
    {"RO", "20", 147}, {"RO", "21", 148}, {"RO", "22", 149}, {"RO", "23", 150},
    {"RO", "24", 151}, {"RO", "25", 152}, {"RO", "26", 153}, {"RO", "27", 154},
    {"RO", "28", 155}, {"RO", "29", 156}, {"RO", "30", 157}, {"RO", "31", 158},
    {"RO", "32", 159}, {"RA", "R1", 160}, {"RA", "R2", 161}, {"RA", "P1", 162},
    {"RA", "P2", 163}, {"RA", "R3", 165}, {"RA", "R4", 166}, {"RA", "P3", 167},
    {"RA", "P4", 168}, {"RO", "33", 176}, {"RO", "34", 177}, {"RO", "35", 178},
    {"RO", "36", 179}, {"RO", "37", 180}, {"RO", "38", 181}, {"RO", "39", 182},
    {"RO", "40", 183}, {"RO", "41", 184}, {"RO", "42", 185}, {"RO", "43", 186},
    {"RO", "44", 187}, {"RO", "45", 188}, {"RO", "46", 189}, {"RO", "47", 190},
    {"RO", "48", 191}};

md_machine_name_t machine_names[137] = {
    {"GND---", 0},   {"GND-SN", 1},   {"GND-NS", 2},   {"GND-IM", 3},
    {"GND-SW", 4},   {"GND-PU", 5},   {"TRX-BD", 16},  {"TRX-SD", 17},
    {"TRX-XT", 18},  {"TRX-CP", 19},  {"TRX-RS", 20},  {"TRX-RS", 21},
    {"TRX-CH", 22},  {"TRX-OH", 23},  {"TRX-CY", 24},  {"TRX-MA", 25},
    {"TRX-CL", 26},  {"TRX-XC", 27},  {"TRX-B2", 28},  {"TRX-S2", 29},
    {"EFM-BD", 32},  {"EFM-SD", 33},  {"EFM-XT", 34},  {"EFM-CP", 35},
    {"EFM-RS", 36},  {"EFM-CB", 37},  {"EFM-HH", 38},  {"EFM-CY", 39},
    {"E12-BD", 48},  {"E12-SD", 49},  {"E12-HT", 50},  {"E12-LT", 51},
    {"E12-CP", 52},  {"E12-RS", 53},  {"E12-CB", 54},  {"E12-CH", 55},
    {"E12-OH", 56},  {"E12-RC", 57},  {"E12-CC", 58},  {"E12-BR", 59},
    {"E12-TA", 60},  {"E12-TR", 61},  {"E12-SH", 62},  {"E12-BC", 63},
    {"P-I-BD", 64},  {"P-I-SD", 65},  {"P-I-MT", 66},  {"P-I-ML", 67},
    {"P-I-MA", 68},  {"P-I-RS", 69},  {"P-I-RC", 70},  {"P-I-CC", 71},
    {"P-I-HH", 72},  {"INP-GA", 80},  {"INP-GB", 81},  {"INP-FA", 82},
    {"INP-FB", 83},  {"INP-EA", 84},  {"INP-EB", 85},  {"MID-01", 96},
    {"MID-02", 97},  {"MID-03", 98},  {"MID-04", 99},  {"MID-05", 100},
    {"MID-06", 101}, {"MID-07", 102}, {"MID-08", 103}, {"MID-09", 104},
    {"MID-10", 105}, {"MID-11", 106}, {"MID-12", 107}, {"MID-13", 108},
    {"MID-14", 109}, {"MID-15", 110}, {"MID-16", 111}, {"CTR-AL", 112},
    {"CTR-8P", 113}, {"CTR-RE", 120}, {"CTR-GB", 121}, {"CTR-EQ", 122},
    {"CTR-DX", 123}, {"ROM-01", 128}, {"ROM-02", 129}, {"ROM-03", 130},
    {"ROM-04", 131}, {"ROM-05", 132}, {"ROM-06", 133}, {"ROM-07", 134},
    {"ROM-08", 135}, {"ROM-09", 136}, {"ROM-10", 137}, {"ROM-11", 138},
    {"ROM-12", 139}, {"ROM-13", 140}, {"ROM-14", 141}, {"ROM-15", 142},
    {"ROM-16", 143}, {"ROM-17", 144}, {"ROM-18", 145}, {"ROM-19", 146},
    {"ROM-20", 147}, {"ROM-21", 148}, {"ROM-22", 149}, {"ROM-23", 150},
    {"ROM-24", 151}, {"ROM-25", 152}, {"ROM-26", 153}, {"ROM-27", 154},
    {"ROM-28", 155}, {"ROM-29", 156}, {"ROM-30", 157}, {"ROM-31", 158},
    {"ROM-32", 159}, {"RAM-R1", 160}, {"RAM-R2", 161}, {"RAM-P1", 162},
    {"RAM-P2", 163}, {"RAM-R3", 165}, {"RAM-R4", 166}, {"RAM-P3", 167},
    {"RAM-P4", 168}, {"ROM-33", 176}, {"ROM-34", 177}, {"ROM-35", 178},
    {"ROM-36", 179}, {"ROM-37", 180}, {"ROM-38", 181}, {"ROM-39", 182},
    {"ROM-40", 183}, {"ROM-41", 184}, {"ROM-42", 185}, {"ROM-43", 186},
    {"ROM-44", 187}, {"ROM-45", 188}, {"ROM-46", 189}, {"ROM-47", 190},
    {"ROM-48", 191}};

mnm_machine_name_t mnm_machine_names[] = {
    {"GND-GND", 0},    {"GND-SIN", 1},    {"GND-NOIS", 2},

    {"SWAVE-SAW", 4},  {"SWAVE-PULS", 5}, {"SWAVE-ENS", 14},

    {"SID-6581", 3},

    {"DPRO-WAVE", 6},  {"DPRO-BBOX", 7},  {"DPRO-DDRW", 32}, {"DPRO-DENS", 33},

    {"FM+-STAT", 8},   {"FM+-PAR", 9},    {"FM+-DYN", 10},

    {"VO-VO-6", 11},

    {"FX-THRU", 12},   {"FX-REVERB", 13}, {"FX-CHORUS", 15}, {"FX-DYNAMIX", 16},
    {"FX-RINGMOD", 17}};

short_machine_name_t mnm_machine_names_short[] = {
    {"GN", "--", 0},  {"GN", "SN", 1},  {"GN", "NS", 2},

    {"SW", "SW", 4},  {"SW", "PU", 5},  {"SW", "EN", 14},

    {"SI", "65", 3},

    {"DP", "WA", 6},  {"DP", "BB", 7},  {"DP", "DD", 32}, {"DP", "DE", 33},

    {"FM", "ST", 8},  {"FM", "PA", 9},  {"FM", "DY", 10},

    {"VO", "VO", 11},

    {"FX", "TH", 12}, {"FX", "RV", 13}, {"FX", "CR", 15}, {"FX", "DM", 16},
    {"FX", "RM", 17}};

model_param_name_t gnd_sn_model_names[] = {{"PT1", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"PT2", 4}, {"PT3", 5},
                                           {"PT4", 6}, {"UNI", 7}, {"", 127}};

model_param_name_t gnd_ns_model_names[] = {{"DEC", 0}, {"", 127}};
model_param_name_t gnd_im_model_names[] = {
    {"UP", 0}, {"UVL", 1}, {"DWN", 2}, {"DVL", 3}, {"", 127}};
model_param_name_t gnd_sw_model_names[] = {{"PT1", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"PT2", 4}, {"PT3", 5},
                                           {"SKE", 6}, {"UNI", 7}, {"", 127}};
model_param_name_t gnd_pu_model_names[] = {{"PT1", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"PT2", 4}, {"PT3", 5},
                                           {"WID", 6}, {"UNI", 7}, {"", 127}};

model_param_name_t trx_bd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"STR", 4}, {"NS", 5},
                                           {"HRM", 6}, {"CLP", 7}, {"", 127}};
model_param_name_t trx_b2_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"HLD", 3}, {"TCK", 4}, {"NS", 5},
                                           {"DRT", 6}, {"DST", 7}, {"", 127}};
model_param_name_t trx_s2_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"NS", 2},
                                           {"NDE", 3}, {"PWR", 4}, {"TUN", 5},
                                           {"NTU", 6}, {"NTY", 7}, {"", 127}};
model_param_name_t trx_sd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"BMP", 2},
                                           {"BNV", 3}, {"SNP", 4}, {"TON", 5},
                                           {"TUN", 6}, {"CLP", 7}, {"", 127}};
model_param_name_t trx_xt_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"DMP", 4}, {"DST", 5},
                                           {"DTP", 6}, {"", 127}};
model_param_name_t trx_cp_model_names[] = {{"CLP", 0}, {"TON", 1}, {"HRD", 2},
                                           {"RCH", 3}, {"RAT", 4}, {"ROM", 5},
                                           {"RSZ", 6}, {"RTN", 7}, {"", 127}};
model_param_name_t trx_rs_model_names[] = {
    {"PTC", 0}, {"DEC", 1}, {"DST", 2}, {"", 127}};
model_param_name_t trx_cb_model_names[] = {{"PTC", 0}, {"DEC", 2}, {"ENH", 3},
                                           {"DMP", 4}, {"TON", 5}, {"BMP", 6},
                                           {"", 127}};
model_param_name_t trx_ch_model_names[] = {{"GAP", 0}, {"DEC", 1}, {"HPF", 2},
                                           {"LPF", 3}, {"MTL", 4}, {"", 127}};
model_param_name_t trx_oh_model_names[] = {{"GAP", 0}, {"DEC", 1}, {"HPF", 2},
                                           {"LPF", 3}, {"MTL", 4}, {"", 127}};
model_param_name_t trx_cy_model_names[] = {{"RCH", 0}, {"DEC", 1}, {"TOP", 2},
                                           {"TTU", 3}, {"SZ", 4},  {"PK", 5},
                                           {"", 127}};
model_param_name_t trx_ma_model_names[] = {{"ATT", 0}, {"SUS", 1}, {"REV", 2},
                                           {"DMP", 3}, {"RTL", 4}, {"RTP", 5},
                                           {"TON", 6}, {"HRD", 7}, {"", 127}};
model_param_name_t trx_cl_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"DUA", 2},
                                           {"ENH", 3}, {"TUN", 4}, {"CLC", 5},
                                           {"", 127}};
model_param_name_t trx_xc_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"DMP", 4}, {"DST", 5},
                                           {"DTP", 6}, {"", 127}};

model_param_name_t efm_bd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"MFB", 7}, {"", 127}};
model_param_name_t efm_sd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"NS", 2},
                                           {"NDC", 3}, {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"HPF", 7}, {"", 127}};
model_param_name_t efm_xt_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"RMP", 2},
                                           {"RDC", 3}, {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"CLC", 7}, {"", 127}};
model_param_name_t efm_cp_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"CLP", 2},
                                           {"CDC", 3}, {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"HPF", 7}, {"", 127}};
model_param_name_t efm_rs_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"MOD", 2},
                                           {"HPF", 3}, {"SNR", 4}, {"SPT", 5},
                                           {"SDC", 6}, {"SMD", 7}, {"", 127}};
model_param_name_t efm_cb_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"SNP", 2},
                                           {"FB", 3},  {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"", 127}};
model_param_name_t efm_hh_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"TRM", 2},
                                           {"TFQ", 3}, {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"FB", 7},  {"", 127}};
model_param_name_t efm_cy_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"FB", 2},
                                           {"HPF", 3}, {"MOD", 4}, {"MFQ", 5},
                                           {"MDC", 6}, {"", 127}};

model_param_name_t e12_bd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"SNP", 2},
                                           {"SPL", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_sd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"RNG", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_ht_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_lt_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"RNG", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_cp_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_rs_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"RTL", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_cb_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_ch_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_oh_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"STP", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_rc_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"BEL", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_cc_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_br_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"REL", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_ta_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_tr_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"HPQ", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_sh_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"SLW", 3}, {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};
model_param_name_t e12_bc_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HP", 2},
                                           {"BC", 3},  {"STR", 4}, {"RTG", 5},
                                           {"RTM", 6}, {"BND", 7}, {"", 127}};

model_param_name_t p_i_bd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HRD", 2},
                                           {"HMR", 3}, {"TNS", 4}, {"DMP", 5},
                                           {"", 127}};
model_param_name_t p_i_sd_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HRD", 2},
                                           {"TNS", 3}, {"RVL", 4}, {"RDC", 5},
                                           {"RNG", 6}, {"", 127}};
model_param_name_t p_i_mt_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HRD", 2},
                                           {"HMR", 3}, {"TUN", 4}, {"DMP", 5},
                                           {"SZ", 6},  {"POS", 7}, {"", 127}};
model_param_name_t p_i_ml_model_names[] = {
    {"PTC", 0}, {"DEC", 1}, {"HRD", 2}, {"TNS", 3}, {"", 127}};
model_param_name_t p_i_ma_model_names[] = {{"GRN", 0}, {"DEC", 1}, {"GLN", 2},
                                           {"SZ", 4},  {"HRD", 5}, {"", 127}};
model_param_name_t p_i_rs_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HRD", 2},
                                           {"RNG", 3}, {"RVL", 4}, {"RDC", 5},
                                           {"", 127}};
model_param_name_t p_i_rc_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HRD", 2},
                                           {"RNG", 3}, {"AG", 4},  {"AU", 5},
                                           {"BR", 6},  {"GRB", 7}, {"", 127}};
model_param_name_t p_i_cc_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HRD", 2},
                                           {"RNG", 3}, {"AG", 4},  {"AU", 5},
                                           {"BR", 6},  {"GRB", 7}, {"", 127}};
model_param_name_t p_i_hh_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"CLS", 2},
                                           {"RNG", 3}, {"AG", 4},  {"AU", 5},
                                           {"BR", 6},  {"CLS", 7}, {"", 127}};

model_param_name_t inp_ga_model_names[] = {{"VOL", 0}, {"GAT", 1}, {"ATK", 2},
                                           {"HLD", 3}, {"DEC", 4}, {"", 127}};
model_param_name_t inp_fa_model_names[] = {{"ALV", 0}, {"GAT", 1}, {"FAT", 2},
                                           {"FHL", 3}, {"FDC", 4}, {"FDP", 5},
                                           {"FFQ", 6}, {"FQ", 7},  {"", 127}};
model_param_name_t inp_ea_model_names[] = {{"AVL", 0}, {"AHL", 1}, {"ADC", 2},
                                           {"FQ", 3},  {"FDP", 4}, {"FHL", 5},
                                           {"FDC", 6}, {"FFQ", 7}, {"", 127}};

model_param_name_t mid_model_names[] = {
    {"NOT", 0},  {"N2", 1},   {"N3", 2},   {"LEN", 3},  {"VEL", 4},
    {"PB", 5},   {"MW", 6},   {"AT", 7},   {"C1D", 8},  {"C1V", 9},
    {"C2D", 10}, {"C2V", 11}, {"C3D", 12}, {"C3V", 13}, {"C4D", 14},
    {"C4V", 15}, {"C5D", 16}, {"C5V", 17}, {"C6D", 18}, {"C6V", 19},
    {"PCG", 20}, {"LFS", 21}, {"LFD", 22}, {"LFM", 23}, {"", 127}};

model_param_name_t ctr_al_model_names[] = {{"SN1", 0}, {"SN2", 1}, {"SN3", 2},
                                           {"SN4", 3}, {"SN5", 4}, {"SN6", 5},
                                           {"SN7", 6}, {"SN8", 7}, {"", 127}};
model_param_name_t ctr_8p_model_names[] = {
    {"P1", 0},   {"P2", 1},   {"P3", 2},   {"P4", 3},   {"P5", 4},
    {"P6", 5},   {"P7", 6},   {"P8", 7},   {"P1T", 8},  {"P1P", 9},
    {"P2T", 10}, {"P2P", 11}, {"P3T", 12}, {"P3P", 13}, {"P4T", 14},
    {"P4P", 15}, {"P5T", 16}, {"P5P", 17}, {"P6T", 18}, {"P6P", 19},
    {"P7T", 20}, {"P7P", 21}, {"P8T", 22}, {"P8P", 23}, {"", 127}};

model_param_name_t ctr_re_model_names[] = {{"TIM", 0}, {"MOD", 1}, {"MFQ", 2},
                                           {"FB", 3},  {"FLF", 4}, {"FLW", 5},
                                           {"MON", 6}, {"LEV", 7}};

model_param_name_t ctr_gb_model_names[] = {{"DVL", 0}, {"PRE", 1}, {"DEC", 2},
                                           {"DMP", 3}, {"HP", 4},  {"LP", 5},
                                           {"GAT", 6}, {"LEV", 7}};

model_param_name_t ctr_eq_model_names[] = {{"LF", 0}, {"LG", 1}, {"HF", 2},
                                           {"HG", 3}, {"PF", 4}, {"PG", 5},
                                           {"PQ", 6}, {"GAI", 7}};

model_param_name_t ctr_dx_model_names[] = {{"ATK", 0}, {"REL", 1}, {"THR", 2},
                                           {"RAT", 3}, {"KNE", 4}, {"HP", 5},
                                           {"GAI", 6}, {"MIX", 7}};

model_param_name_t rom_model_names[] = {{"PTC", 0}, {"DEC", 1}, {"HLD", 2},
                                        {"BRR", 3}, {"STR", 4}, {"END", 5},
                                        {"RTG", 6}, {"RTM", 7}, {"", 127}};
model_param_name_t ram_r_model_names[] = {{"MLV", 0}, {"MBL", 1}, {"ILV", 2},
                                          {"IBL", 3}, {"CU1", 4}, {"CU2", 5},
                                          {"LEN", 6}, {"RAT", 7}, {"", 127}};

model_param_name_t generic_param_names[] = {
    {"P1", 0},   {"P2", 1},   {"P3", 2},   {"P4", 3},   {"P5", 4},
    {"P6", 5},   {"P7", 6},   {"P8", 7},   {"AMD", 8},  {"AMF", 9},
    {"EQF", 10}, {"EQG", 11}, {"FLF", 12}, {"FLW", 13}, {"FLQ", 14},
    {"SRR", 15}, {"DST", 16}, {"VOL", 17}, {"PAN", 18}, {"DEL", 19},
    {"REV", 20}, {"LFS", 21}, {"LFD", 22}, {"LFM", 23}, {"", 127}};
