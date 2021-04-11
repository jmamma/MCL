#include "Elektron.h"

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
