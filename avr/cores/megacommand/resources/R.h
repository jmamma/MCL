#pragma once
#include <avr/pgmspace.h>
#include "MCL.h"
#include "MCL_impl.h"
extern const unsigned char __R_font_default[] PROGMEM;
struct __T_font_default {
  union {
    unsigned char font[0];
    char zz__font[1280];
  };
  static constexpr size_t countof_font = 1280 / sizeof(unsigned char);
  static constexpr size_t sizeofof_font = 1280;
};

extern const unsigned char __R_font_elektrothic[] PROGMEM;
struct __T_font_elektrothic {
  union {
    GFXglyph ElektrothicGlyphs[0];
    char zz__ElektrothicGlyphs[665];
  };
  static constexpr size_t countof_ElektrothicGlyphs = 665 / sizeof(GFXglyph);
  static constexpr size_t sizeofof_ElektrothicGlyphs = 665;
  union {
    unsigned char ElektrothicBitmaps[0];
    char zz__ElektrothicBitmaps[593];
  };
  static constexpr size_t countof_ElektrothicBitmaps = 593 / sizeof(unsigned char);
  static constexpr size_t sizeofof_ElektrothicBitmaps = 593;
};

extern const unsigned char __R_font_tomthumb[] PROGMEM;
struct __T_font_tomthumb {
  union {
    GFXglyph TomThumbGlyphs[0];
    char zz__TomThumbGlyphs[665];
  };
  static constexpr size_t countof_TomThumbGlyphs = 665 / sizeof(GFXglyph);
  static constexpr size_t sizeofof_TomThumbGlyphs = 665;
  union {
    unsigned char TomThumbBitmaps[0];
    char zz__TomThumbBitmaps[418];
  };
  static constexpr size_t countof_TomThumbBitmaps = 418 / sizeof(unsigned char);
  static constexpr size_t sizeofof_TomThumbBitmaps = 418;
};

extern const unsigned char __R_icons_boot[] PROGMEM;
struct __T_icons_boot {
  union {
    unsigned char evilknievel_bitmap[0];
    char zz__evilknievel_bitmap[192];
  };
  static constexpr size_t countof_evilknievel_bitmap = 192 / sizeof(unsigned char);
  static constexpr size_t sizeofof_evilknievel_bitmap = 192;
  union {
    unsigned char mcl_logo_bitmap[0];
    char zz__mcl_logo_bitmap[152];
  };
  static constexpr size_t countof_mcl_logo_bitmap = 152 / sizeof(unsigned char);
  static constexpr size_t sizeofof_mcl_logo_bitmap = 152;
};

extern const unsigned char __R_icons_device[] PROGMEM;
struct __T_icons_device {
  union {
    unsigned char icon_mnm[0];
    char zz__icon_mnm[120];
  };
  static constexpr size_t countof_icon_mnm = 120 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_mnm = 120;
  union {
    unsigned char icon_a4[0];
    char zz__icon_a4[120];
  };
  static constexpr size_t countof_icon_a4 = 120 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_a4 = 120;
  union {
    unsigned char icon_md[0];
    char zz__icon_md[120];
  };
  static constexpr size_t countof_icon_md = 120 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_md = 120;
};

extern const unsigned char __R_icons_knob[] PROGMEM;
struct __T_icons_knob {
  union {
    unsigned char icon_chroma[0];
    char zz__icon_chroma[75];
  };
  static constexpr size_t countof_icon_chroma = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_chroma = 75;
  union {
    unsigned char wheel_side[0];
    char zz__wheel_side[57];
  };
  static constexpr size_t countof_wheel_side = 57 / sizeof(unsigned char);
  static constexpr size_t sizeofof_wheel_side = 57;
  union {
    unsigned char wheel_angle[0];
    char zz__wheel_angle[57];
  };
  static constexpr size_t countof_wheel_angle = 57 / sizeof(unsigned char);
  static constexpr size_t sizeofof_wheel_angle = 57;
  union {
    unsigned char wheel_top[0];
    char zz__wheel_top[57];
  };
  static constexpr size_t countof_wheel_top = 57 / sizeof(unsigned char);
  static constexpr size_t sizeofof_wheel_top = 57;
  union {
    unsigned char encoder_small_6[0];
    char zz__encoder_small_6[22];
  };
  static constexpr size_t countof_encoder_small_6 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_6 = 22;
  union {
    unsigned char encoder_small_5[0];
    char zz__encoder_small_5[22];
  };
  static constexpr size_t countof_encoder_small_5 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_5 = 22;
  union {
    unsigned char encoder_small_4[0];
    char zz__encoder_small_4[22];
  };
  static constexpr size_t countof_encoder_small_4 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_4 = 22;
  union {
    unsigned char encoder_small_3[0];
    char zz__encoder_small_3[22];
  };
  static constexpr size_t countof_encoder_small_3 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_3 = 22;
  union {
    unsigned char encoder_small_2[0];
    char zz__encoder_small_2[22];
  };
  static constexpr size_t countof_encoder_small_2 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_2 = 22;
  union {
    unsigned char encoder_small_1[0];
    char zz__encoder_small_1[22];
  };
  static constexpr size_t countof_encoder_small_1 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_1 = 22;
  union {
    unsigned char encoder_small_0[0];
    char zz__encoder_small_0[22];
  };
  static constexpr size_t countof_encoder_small_0 = 22 / sizeof(unsigned char);
  static constexpr size_t sizeofof_encoder_small_0 = 22;
};

extern const unsigned char __R_icons_page[] PROGMEM;
struct __T_icons_page {
  union {
    unsigned char icon_pianoroll[0];
    char zz__icon_pianoroll[75];
  };
  static constexpr size_t countof_icon_pianoroll = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_pianoroll = 75;
  union {
    unsigned char icon_ram1[0];
    char zz__icon_ram1[75];
  };
  static constexpr size_t countof_icon_ram1 = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_ram1 = 75;
  union {
    unsigned char icon_ram2[0];
    char zz__icon_ram2[75];
  };
  static constexpr size_t countof_icon_ram2 = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_ram2 = 75;
  union {
    unsigned char icon_sound[0];
    char zz__icon_sound[57];
  };
  static constexpr size_t countof_icon_sound = 57 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_sound = 57;
  union {
    unsigned char icon_route[0];
    char zz__icon_route[48];
  };
  static constexpr size_t countof_icon_route = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_route = 48;
  union {
    unsigned char icon_rhytmecho[0];
    char zz__icon_rhytmecho[75];
  };
  static constexpr size_t countof_icon_rhytmecho = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_rhytmecho = 75;
  union {
    unsigned char icon_gatebox[0];
    char zz__icon_gatebox[75];
  };
  static constexpr size_t countof_icon_gatebox = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_gatebox = 75;
  union {
    unsigned char icon_mixer[0];
    char zz__icon_mixer[48];
  };
  static constexpr size_t countof_icon_mixer = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_mixer = 48;
  union {
    unsigned char icon_step[0];
    char zz__icon_step[75];
  };
  static constexpr size_t countof_icon_step = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_step = 75;
  union {
    unsigned char icon_para[0];
    char zz__icon_para[57];
  };
  static constexpr size_t countof_icon_para = 57 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_para = 57;
  union {
    unsigned char icon_wavd[0];
    char zz__icon_wavd[57];
  };
  static constexpr size_t countof_icon_wavd = 57 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_wavd = 57;
  union {
    unsigned char icon_loudness[0];
    char zz__icon_loudness[48];
  };
  static constexpr size_t countof_icon_loudness = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_loudness = 48;
  union {
    unsigned char icon_lfo[0];
    char zz__icon_lfo[72];
  };
  static constexpr size_t countof_icon_lfo = 72 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_lfo = 72;
  union {
    unsigned char icon_grid[0];
    char zz__icon_grid[45];
  };
  static constexpr size_t countof_icon_grid = 45 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_grid = 45;
};

extern const unsigned char __R_machine_names_long[] PROGMEM;
struct __T_machine_names_long {
  union {
    mnm_machine_name_t mnm_machine_names[0];
    char zz__mnm_machine_names[240];
  };
  static constexpr size_t countof_mnm_machine_names = 240 / sizeof(mnm_machine_name_t);
  static constexpr size_t sizeofof_mnm_machine_names = 240;
  union {
    md_machine_name_t machine_names[0];
    char zz__machine_names[1096];
  };
  static constexpr size_t countof_machine_names = 1096 / sizeof(md_machine_name_t);
  static constexpr size_t sizeofof_machine_names = 1096;
};

extern const unsigned char __R_machine_names_short[] PROGMEM;
struct __T_machine_names_short {
  union {
    short_machine_name_t mnm_machine_names_short[0];
    char zz__mnm_machine_names_short[140];
  };
  static constexpr size_t countof_mnm_machine_names_short = 140 / sizeof(short_machine_name_t);
  static constexpr size_t sizeofof_mnm_machine_names_short = 140;
  union {
    short_machine_name_t md_machine_names_short[0];
    char zz__md_machine_names_short[959];
  };
  static constexpr size_t countof_md_machine_names_short = 959 / sizeof(short_machine_name_t);
  static constexpr size_t sizeofof_md_machine_names_short = 959;
};

extern const unsigned char __R_machine_param_names[] PROGMEM;
struct __T_machine_param_names {
  union {
    model_param_name_t generic_param_names[0];
    char zz__generic_param_names[125];
  };
  static constexpr size_t countof_generic_param_names = 125 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_generic_param_names = 125;
  union {
    model_param_name_t ram_r_model_names[0];
    char zz__ram_r_model_names[45];
  };
  static constexpr size_t countof_ram_r_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ram_r_model_names = 45;
  union {
    model_param_name_t rom_model_names[0];
    char zz__rom_model_names[45];
  };
  static constexpr size_t countof_rom_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_rom_model_names = 45;
  union {
    model_param_name_t ctr_dx_model_names[0];
    char zz__ctr_dx_model_names[40];
  };
  static constexpr size_t countof_ctr_dx_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ctr_dx_model_names = 40;
  union {
    model_param_name_t ctr_eq_model_names[0];
    char zz__ctr_eq_model_names[40];
  };
  static constexpr size_t countof_ctr_eq_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ctr_eq_model_names = 40;
  union {
    model_param_name_t ctr_gb_model_names[0];
    char zz__ctr_gb_model_names[40];
  };
  static constexpr size_t countof_ctr_gb_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ctr_gb_model_names = 40;
  union {
    model_param_name_t ctr_re_model_names[0];
    char zz__ctr_re_model_names[40];
  };
  static constexpr size_t countof_ctr_re_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ctr_re_model_names = 40;
  union {
    model_param_name_t ctr_8p_model_names[0];
    char zz__ctr_8p_model_names[125];
  };
  static constexpr size_t countof_ctr_8p_model_names = 125 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ctr_8p_model_names = 125;
  union {
    model_param_name_t ctr_al_model_names[0];
    char zz__ctr_al_model_names[45];
  };
  static constexpr size_t countof_ctr_al_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_ctr_al_model_names = 45;
  union {
    model_param_name_t mid_model_names[0];
    char zz__mid_model_names[125];
  };
  static constexpr size_t countof_mid_model_names = 125 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_mid_model_names = 125;
  union {
    model_param_name_t inp_ea_model_names[0];
    char zz__inp_ea_model_names[45];
  };
  static constexpr size_t countof_inp_ea_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_inp_ea_model_names = 45;
  union {
    model_param_name_t inp_fa_model_names[0];
    char zz__inp_fa_model_names[45];
  };
  static constexpr size_t countof_inp_fa_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_inp_fa_model_names = 45;
  union {
    model_param_name_t inp_ga_model_names[0];
    char zz__inp_ga_model_names[30];
  };
  static constexpr size_t countof_inp_ga_model_names = 30 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_inp_ga_model_names = 30;
  union {
    model_param_name_t p_i_hh_model_names[0];
    char zz__p_i_hh_model_names[45];
  };
  static constexpr size_t countof_p_i_hh_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_hh_model_names = 45;
  union {
    model_param_name_t p_i_cc_model_names[0];
    char zz__p_i_cc_model_names[45];
  };
  static constexpr size_t countof_p_i_cc_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_cc_model_names = 45;
  union {
    model_param_name_t p_i_rc_model_names[0];
    char zz__p_i_rc_model_names[45];
  };
  static constexpr size_t countof_p_i_rc_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_rc_model_names = 45;
  union {
    model_param_name_t p_i_rs_model_names[0];
    char zz__p_i_rs_model_names[35];
  };
  static constexpr size_t countof_p_i_rs_model_names = 35 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_rs_model_names = 35;
  union {
    model_param_name_t p_i_ma_model_names[0];
    char zz__p_i_ma_model_names[30];
  };
  static constexpr size_t countof_p_i_ma_model_names = 30 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_ma_model_names = 30;
  union {
    model_param_name_t p_i_ml_model_names[0];
    char zz__p_i_ml_model_names[25];
  };
  static constexpr size_t countof_p_i_ml_model_names = 25 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_ml_model_names = 25;
  union {
    model_param_name_t p_i_mt_model_names[0];
    char zz__p_i_mt_model_names[45];
  };
  static constexpr size_t countof_p_i_mt_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_mt_model_names = 45;
  union {
    model_param_name_t p_i_sd_model_names[0];
    char zz__p_i_sd_model_names[40];
  };
  static constexpr size_t countof_p_i_sd_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_sd_model_names = 40;
  union {
    model_param_name_t p_i_bd_model_names[0];
    char zz__p_i_bd_model_names[35];
  };
  static constexpr size_t countof_p_i_bd_model_names = 35 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_p_i_bd_model_names = 35;
  union {
    model_param_name_t e12_bc_model_names[0];
    char zz__e12_bc_model_names[45];
  };
  static constexpr size_t countof_e12_bc_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_bc_model_names = 45;
  union {
    model_param_name_t e12_sh_model_names[0];
    char zz__e12_sh_model_names[45];
  };
  static constexpr size_t countof_e12_sh_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_sh_model_names = 45;
  union {
    model_param_name_t e12_tr_model_names[0];
    char zz__e12_tr_model_names[45];
  };
  static constexpr size_t countof_e12_tr_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_tr_model_names = 45;
  union {
    model_param_name_t e12_ta_model_names[0];
    char zz__e12_ta_model_names[45];
  };
  static constexpr size_t countof_e12_ta_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_ta_model_names = 45;
  union {
    model_param_name_t e12_br_model_names[0];
    char zz__e12_br_model_names[45];
  };
  static constexpr size_t countof_e12_br_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_br_model_names = 45;
  union {
    model_param_name_t e12_cc_model_names[0];
    char zz__e12_cc_model_names[45];
  };
  static constexpr size_t countof_e12_cc_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_cc_model_names = 45;
  union {
    model_param_name_t e12_rc_model_names[0];
    char zz__e12_rc_model_names[45];
  };
  static constexpr size_t countof_e12_rc_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_rc_model_names = 45;
  union {
    model_param_name_t e12_oh_model_names[0];
    char zz__e12_oh_model_names[45];
  };
  static constexpr size_t countof_e12_oh_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_oh_model_names = 45;
  union {
    model_param_name_t e12_ch_model_names[0];
    char zz__e12_ch_model_names[45];
  };
  static constexpr size_t countof_e12_ch_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_ch_model_names = 45;
  union {
    model_param_name_t e12_cb_model_names[0];
    char zz__e12_cb_model_names[45];
  };
  static constexpr size_t countof_e12_cb_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_cb_model_names = 45;
  union {
    model_param_name_t e12_rs_model_names[0];
    char zz__e12_rs_model_names[45];
  };
  static constexpr size_t countof_e12_rs_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_rs_model_names = 45;
  union {
    model_param_name_t e12_cp_model_names[0];
    char zz__e12_cp_model_names[45];
  };
  static constexpr size_t countof_e12_cp_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_cp_model_names = 45;
  union {
    model_param_name_t e12_lt_model_names[0];
    char zz__e12_lt_model_names[45];
  };
  static constexpr size_t countof_e12_lt_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_lt_model_names = 45;
  union {
    model_param_name_t e12_ht_model_names[0];
    char zz__e12_ht_model_names[45];
  };
  static constexpr size_t countof_e12_ht_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_ht_model_names = 45;
  union {
    model_param_name_t e12_sd_model_names[0];
    char zz__e12_sd_model_names[45];
  };
  static constexpr size_t countof_e12_sd_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_sd_model_names = 45;
  union {
    model_param_name_t e12_bd_model_names[0];
    char zz__e12_bd_model_names[45];
  };
  static constexpr size_t countof_e12_bd_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_e12_bd_model_names = 45;
  union {
    model_param_name_t efm_cy_model_names[0];
    char zz__efm_cy_model_names[40];
  };
  static constexpr size_t countof_efm_cy_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_cy_model_names = 40;
  union {
    model_param_name_t efm_hh_model_names[0];
    char zz__efm_hh_model_names[45];
  };
  static constexpr size_t countof_efm_hh_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_hh_model_names = 45;
  union {
    model_param_name_t efm_cb_model_names[0];
    char zz__efm_cb_model_names[40];
  };
  static constexpr size_t countof_efm_cb_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_cb_model_names = 40;
  union {
    model_param_name_t efm_rs_model_names[0];
    char zz__efm_rs_model_names[45];
  };
  static constexpr size_t countof_efm_rs_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_rs_model_names = 45;
  union {
    model_param_name_t efm_cp_model_names[0];
    char zz__efm_cp_model_names[45];
  };
  static constexpr size_t countof_efm_cp_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_cp_model_names = 45;
  union {
    model_param_name_t efm_xt_model_names[0];
    char zz__efm_xt_model_names[45];
  };
  static constexpr size_t countof_efm_xt_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_xt_model_names = 45;
  union {
    model_param_name_t efm_sd_model_names[0];
    char zz__efm_sd_model_names[45];
  };
  static constexpr size_t countof_efm_sd_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_sd_model_names = 45;
  union {
    model_param_name_t efm_bd_model_names[0];
    char zz__efm_bd_model_names[45];
  };
  static constexpr size_t countof_efm_bd_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_efm_bd_model_names = 45;
  union {
    model_param_name_t trx_xc_model_names[0];
    char zz__trx_xc_model_names[40];
  };
  static constexpr size_t countof_trx_xc_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_xc_model_names = 40;
  union {
    model_param_name_t trx_cl_model_names[0];
    char zz__trx_cl_model_names[35];
  };
  static constexpr size_t countof_trx_cl_model_names = 35 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_cl_model_names = 35;
  union {
    model_param_name_t trx_ma_model_names[0];
    char zz__trx_ma_model_names[45];
  };
  static constexpr size_t countof_trx_ma_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_ma_model_names = 45;
  union {
    model_param_name_t trx_cy_model_names[0];
    char zz__trx_cy_model_names[35];
  };
  static constexpr size_t countof_trx_cy_model_names = 35 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_cy_model_names = 35;
  union {
    model_param_name_t trx_oh_model_names[0];
    char zz__trx_oh_model_names[30];
  };
  static constexpr size_t countof_trx_oh_model_names = 30 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_oh_model_names = 30;
  union {
    model_param_name_t trx_ch_model_names[0];
    char zz__trx_ch_model_names[30];
  };
  static constexpr size_t countof_trx_ch_model_names = 30 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_ch_model_names = 30;
  union {
    model_param_name_t trx_cb_model_names[0];
    char zz__trx_cb_model_names[35];
  };
  static constexpr size_t countof_trx_cb_model_names = 35 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_cb_model_names = 35;
  union {
    model_param_name_t trx_rs_model_names[0];
    char zz__trx_rs_model_names[20];
  };
  static constexpr size_t countof_trx_rs_model_names = 20 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_rs_model_names = 20;
  union {
    model_param_name_t trx_cp_model_names[0];
    char zz__trx_cp_model_names[45];
  };
  static constexpr size_t countof_trx_cp_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_cp_model_names = 45;
  union {
    model_param_name_t trx_xt_model_names[0];
    char zz__trx_xt_model_names[40];
  };
  static constexpr size_t countof_trx_xt_model_names = 40 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_xt_model_names = 40;
  union {
    model_param_name_t trx_sd_model_names[0];
    char zz__trx_sd_model_names[45];
  };
  static constexpr size_t countof_trx_sd_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_sd_model_names = 45;
  union {
    model_param_name_t trx_s2_model_names[0];
    char zz__trx_s2_model_names[45];
  };
  static constexpr size_t countof_trx_s2_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_s2_model_names = 45;
  union {
    model_param_name_t trx_b2_model_names[0];
    char zz__trx_b2_model_names[45];
  };
  static constexpr size_t countof_trx_b2_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_b2_model_names = 45;
  union {
    model_param_name_t trx_bd_model_names[0];
    char zz__trx_bd_model_names[45];
  };
  static constexpr size_t countof_trx_bd_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_trx_bd_model_names = 45;
  union {
    model_param_name_t gnd_pu_model_names[0];
    char zz__gnd_pu_model_names[45];
  };
  static constexpr size_t countof_gnd_pu_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_gnd_pu_model_names = 45;
  union {
    model_param_name_t gnd_sw_model_names[0];
    char zz__gnd_sw_model_names[45];
  };
  static constexpr size_t countof_gnd_sw_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_gnd_sw_model_names = 45;
  union {
    model_param_name_t gnd_im_model_names[0];
    char zz__gnd_im_model_names[25];
  };
  static constexpr size_t countof_gnd_im_model_names = 25 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_gnd_im_model_names = 25;
  union {
    model_param_name_t gnd_ns_model_names[0];
    char zz__gnd_ns_model_names[10];
  };
  static constexpr size_t countof_gnd_ns_model_names = 10 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_gnd_ns_model_names = 10;
  union {
    model_param_name_t gnd_sn_model_names[0];
    char zz__gnd_sn_model_names[45];
  };
  static constexpr size_t countof_gnd_sn_model_names = 45 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_gnd_sn_model_names = 45;
};

extern const unsigned char __R_menu_options[] PROGMEM;
struct __T_menu_options {
  union {
    menu_option_t _MENU_OPTIONS[0];
    char zz___MENU_OPTIONS[738];
  };
  static constexpr size_t countof__MENU_OPTIONS = 738 / sizeof(menu_option_t);
  static constexpr size_t sizeofof__MENU_OPTIONS = 738;
};

extern const unsigned char __R_page_entries[] PROGMEM;
struct __T_page_entries {
  union {
    PageSelectEntry Entries[0];
    char zz__Entries[360];
  };
  static constexpr size_t countof_Entries = 360 / sizeof(PageSelectEntry);
  static constexpr size_t sizeofof_Entries = 360;
};

extern const unsigned char __R_tuning[] PROGMEM;
struct __T_tuning {
  union {
    unsigned char tonal_tuning[0];
    char zz__tonal_tuning[64];
  };
  static constexpr size_t countof_tonal_tuning = 64 / sizeof(unsigned char);
  static constexpr size_t sizeofof_tonal_tuning = 64;
  union {
    unsigned char e12_lt_tuning[0];
    char zz__e12_lt_tuning[40];
  };
  static constexpr size_t countof_e12_lt_tuning = 40 / sizeof(unsigned char);
  static constexpr size_t sizeofof_e12_lt_tuning = 40;
  union {
    unsigned char e12_cb_tuning[0];
    char zz__e12_cb_tuning[48];
  };
  static constexpr size_t countof_e12_cb_tuning = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_e12_cb_tuning = 48;
  union {
    unsigned char e12_bc_tuning[0];
    char zz__e12_bc_tuning[48];
  };
  static constexpr size_t countof_e12_bc_tuning = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_e12_bc_tuning = 48;
  union {
    unsigned char efm_cy_tuning[0];
    char zz__efm_cy_tuning[30];
  };
  static constexpr size_t countof_efm_cy_tuning = 30 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_cy_tuning = 30;
  union {
    unsigned char efm_cb_tuning[0];
    char zz__efm_cb_tuning[30];
  };
  static constexpr size_t countof_efm_cb_tuning = 30 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_cb_tuning = 30;
  union {
    unsigned char trx_rs_tuning[0];
    char zz__trx_rs_tuning[12];
  };
  static constexpr size_t countof_trx_rs_tuning = 12 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_rs_tuning = 12;
  union {
    unsigned char trx_b2_tuning[0];
    char zz__trx_b2_tuning[24];
  };
  static constexpr size_t countof_trx_b2_tuning = 24 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_b2_tuning = 24;
  union {
    unsigned char gnd_sn_tuning[0];
    char zz__gnd_sn_tuning[96];
  };
  static constexpr size_t countof_gnd_sn_tuning = 96 / sizeof(unsigned char);
  static constexpr size_t sizeofof_gnd_sn_tuning = 96;
  union {
    unsigned char rom_tuning[0];
    char zz__rom_tuning[48];
  };
  static constexpr size_t countof_rom_tuning = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_rom_tuning = 48;
  union {
    unsigned char trx_s2_tuning[0];
    char zz__trx_s2_tuning[19];
  };
  static constexpr size_t countof_trx_s2_tuning = 19 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_s2_tuning = 19;
  union {
    unsigned char trx_bd_tuning[0];
    char zz__trx_bd_tuning[24];
  };
  static constexpr size_t countof_trx_bd_tuning = 24 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_bd_tuning = 24;
  union {
    unsigned char trx_xt_tuning[0];
    char zz__trx_xt_tuning[24];
  };
  static constexpr size_t countof_trx_xt_tuning = 24 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_xt_tuning = 24;
  union {
    unsigned char trx_xc_tuning[0];
    char zz__trx_xc_tuning[24];
  };
  static constexpr size_t countof_trx_xc_tuning = 24 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_xc_tuning = 24;
  union {
    unsigned char trx_sd_tuning[0];
    char zz__trx_sd_tuning[12];
  };
  static constexpr size_t countof_trx_sd_tuning = 12 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_sd_tuning = 12;
  union {
    unsigned char trx_cl_tuning[0];
    char zz__trx_cl_tuning[21];
  };
  static constexpr size_t countof_trx_cl_tuning = 21 / sizeof(unsigned char);
  static constexpr size_t sizeofof_trx_cl_tuning = 21;
  union {
    unsigned char efm_bd_tuning[0];
    char zz__efm_bd_tuning[48];
  };
  static constexpr size_t countof_efm_bd_tuning = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_bd_tuning = 48;
  union {
    unsigned char efm_xt_tuning[0];
    char zz__efm_xt_tuning[24];
  };
  static constexpr size_t countof_efm_xt_tuning = 24 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_xt_tuning = 24;
  union {
    unsigned char efm_sd_tuning[0];
    char zz__efm_sd_tuning[30];
  };
  static constexpr size_t countof_efm_sd_tuning = 30 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_sd_tuning = 30;
  union {
    unsigned char efm_cp_tuning[0];
    char zz__efm_cp_tuning[66];
  };
  static constexpr size_t countof_efm_cp_tuning = 66 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_cp_tuning = 66;
  union {
    unsigned char efm_hh_tuning[0];
    char zz__efm_hh_tuning[30];
  };
  static constexpr size_t countof_efm_hh_tuning = 30 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_hh_tuning = 30;
  union {
    unsigned char efm_rs_tuning[0];
    char zz__efm_rs_tuning[48];
  };
  static constexpr size_t countof_efm_rs_tuning = 48 / sizeof(unsigned char);
  static constexpr size_t sizeofof_efm_rs_tuning = 48;
};

