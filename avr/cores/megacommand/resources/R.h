#pragma once
#include <avr/pgmspace.h>
#include "MCL.h"
#include "MCL_impl.h"
extern const unsigned char __R_font_default[] PROGMEM;
struct __T_font_default {
  union {
    unsigned char font[0];
    char zz__0[1280];
  };
};

extern const unsigned char __R_font_elektrothic[] PROGMEM;
struct __T_font_elektrothic {
  union {
    GFXglyph ElektrothicGlyphs[0];
    char zz__0[665];
  };
  union {
    unsigned char ElektrothicBitmaps[0];
    char zz__1[593];
  };
};

extern const unsigned char __R_font_tomthumb[] PROGMEM;
struct __T_font_tomthumb {
  union {
    GFXglyph TomThumbGlyphs[0];
    char zz__0[665];
  };
  union {
    unsigned char TomThumbBitmaps[0];
    char zz__1[418];
  };
};

extern const unsigned char __R_icons_boot[] PROGMEM;
struct __T_icons_boot {
  union {
    unsigned char evilknievel_bitmap[0];
    char zz__0[192];
  };
  union {
    unsigned char mcl_logo_bitmap[0];
    char zz__1[152];
  };
};

extern const unsigned char __R_icons_device[] PROGMEM;
struct __T_icons_device {
  union {
    unsigned char icon_mnm[0];
    char zz__0[120];
  };
  union {
    unsigned char icon_a4[0];
    char zz__1[120];
  };
  union {
    unsigned char icon_md[0];
    char zz__2[120];
  };
};

extern const unsigned char __R_icons_knob[] PROGMEM;
struct __T_icons_knob {
  union {
    unsigned char icon_chroma[0];
    char zz__0[75];
  };
  union {
    unsigned char wheel_side[0];
    char zz__1[57];
  };
  union {
    unsigned char wheel_angle[0];
    char zz__2[57];
  };
  union {
    unsigned char wheel_top[0];
    char zz__3[57];
  };
  union {
    unsigned char encoder_small_6[0];
    char zz__4[22];
  };
  union {
    unsigned char encoder_small_5[0];
    char zz__5[22];
  };
  union {
    unsigned char encoder_small_4[0];
    char zz__6[22];
  };
  union {
    unsigned char encoder_small_3[0];
    char zz__7[22];
  };
  union {
    unsigned char encoder_small_2[0];
    char zz__8[22];
  };
  union {
    unsigned char encoder_small_1[0];
    char zz__9[22];
  };
  union {
    unsigned char encoder_small_0[0];
    char zz__10[22];
  };
};

extern const unsigned char __R_icons_page[] PROGMEM;
struct __T_icons_page {
  union {
    unsigned char icon_pianoroll[0];
    char zz__0[75];
  };
  union {
    unsigned char icon_ram1[0];
    char zz__1[75];
  };
  union {
    unsigned char icon_ram2[0];
    char zz__2[75];
  };
  union {
    unsigned char icon_sound[0];
    char zz__3[57];
  };
  union {
    unsigned char icon_route[0];
    char zz__4[48];
  };
  union {
    unsigned char icon_rhytmecho[0];
    char zz__5[75];
  };
  union {
    unsigned char icon_gatebox[0];
    char zz__6[75];
  };
  union {
    unsigned char icon_mixer[0];
    char zz__7[48];
  };
  union {
    unsigned char icon_step[0];
    char zz__8[75];
  };
  union {
    unsigned char icon_para[0];
    char zz__9[57];
  };
  union {
    unsigned char icon_wavd[0];
    char zz__10[57];
  };
  union {
    unsigned char icon_loudness[0];
    char zz__11[48];
  };
  union {
    unsigned char icon_lfo[0];
    char zz__12[72];
  };
  union {
    unsigned char icon_grid[0];
    char zz__13[45];
  };
};

extern const unsigned char __R_machine_names_long[] PROGMEM;
struct __T_machine_names_long {
  union {
    mnm_machine_name_t mnm_machine_names[0];
    char zz__0[240];
  };
  union {
    md_machine_name_t machine_names[0];
    char zz__1[1096];
  };
};

extern const unsigned char __R_machine_names_short[] PROGMEM;
struct __T_machine_names_short {
  union {
    short_machine_name_t mnm_machine_names_short[0];
    char zz__0[140];
  };
  union {
    short_machine_name_t md_machine_names_short[0];
    char zz__1[959];
  };
};

extern const unsigned char __R_machine_param_names[] PROGMEM;
struct __T_machine_param_names {
  union {
    model_param_name_t generic_param_names[0];
    char zz__0[125];
  };
  union {
    model_param_name_t ram_r_model_names[0];
    char zz__1[45];
  };
  union {
    model_param_name_t rom_model_names[0];
    char zz__2[45];
  };
  union {
    model_param_name_t ctr_dx_model_names[0];
    char zz__3[40];
  };
  union {
    model_param_name_t ctr_eq_model_names[0];
    char zz__4[40];
  };
  union {
    model_param_name_t ctr_gb_model_names[0];
    char zz__5[40];
  };
  union {
    model_param_name_t ctr_re_model_names[0];
    char zz__6[40];
  };
  union {
    model_param_name_t ctr_8p_model_names[0];
    char zz__7[125];
  };
  union {
    model_param_name_t ctr_al_model_names[0];
    char zz__8[45];
  };
  union {
    model_param_name_t mid_model_names[0];
    char zz__9[125];
  };
  union {
    model_param_name_t inp_ea_model_names[0];
    char zz__10[45];
  };
  union {
    model_param_name_t inp_fa_model_names[0];
    char zz__11[45];
  };
  union {
    model_param_name_t inp_ga_model_names[0];
    char zz__12[30];
  };
  union {
    model_param_name_t p_i_hh_model_names[0];
    char zz__13[45];
  };
  union {
    model_param_name_t p_i_cc_model_names[0];
    char zz__14[45];
  };
  union {
    model_param_name_t p_i_rc_model_names[0];
    char zz__15[45];
  };
  union {
    model_param_name_t p_i_rs_model_names[0];
    char zz__16[35];
  };
  union {
    model_param_name_t p_i_ma_model_names[0];
    char zz__17[30];
  };
  union {
    model_param_name_t p_i_ml_model_names[0];
    char zz__18[25];
  };
  union {
    model_param_name_t p_i_mt_model_names[0];
    char zz__19[45];
  };
  union {
    model_param_name_t p_i_sd_model_names[0];
    char zz__20[40];
  };
  union {
    model_param_name_t p_i_bd_model_names[0];
    char zz__21[35];
  };
  union {
    model_param_name_t e12_bc_model_names[0];
    char zz__22[45];
  };
  union {
    model_param_name_t e12_sh_model_names[0];
    char zz__23[45];
  };
  union {
    model_param_name_t e12_tr_model_names[0];
    char zz__24[45];
  };
  union {
    model_param_name_t e12_ta_model_names[0];
    char zz__25[45];
  };
  union {
    model_param_name_t e12_br_model_names[0];
    char zz__26[45];
  };
  union {
    model_param_name_t e12_cc_model_names[0];
    char zz__27[45];
  };
  union {
    model_param_name_t e12_rc_model_names[0];
    char zz__28[45];
  };
  union {
    model_param_name_t e12_oh_model_names[0];
    char zz__29[45];
  };
  union {
    model_param_name_t e12_ch_model_names[0];
    char zz__30[45];
  };
  union {
    model_param_name_t e12_cb_model_names[0];
    char zz__31[45];
  };
  union {
    model_param_name_t e12_rs_model_names[0];
    char zz__32[45];
  };
  union {
    model_param_name_t e12_cp_model_names[0];
    char zz__33[45];
  };
  union {
    model_param_name_t e12_lt_model_names[0];
    char zz__34[45];
  };
  union {
    model_param_name_t e12_ht_model_names[0];
    char zz__35[45];
  };
  union {
    model_param_name_t e12_sd_model_names[0];
    char zz__36[45];
  };
  union {
    model_param_name_t e12_bd_model_names[0];
    char zz__37[45];
  };
  union {
    model_param_name_t efm_cy_model_names[0];
    char zz__38[40];
  };
  union {
    model_param_name_t efm_hh_model_names[0];
    char zz__39[45];
  };
  union {
    model_param_name_t efm_cb_model_names[0];
    char zz__40[40];
  };
  union {
    model_param_name_t efm_rs_model_names[0];
    char zz__41[45];
  };
  union {
    model_param_name_t efm_cp_model_names[0];
    char zz__42[45];
  };
  union {
    model_param_name_t efm_xt_model_names[0];
    char zz__43[45];
  };
  union {
    model_param_name_t efm_sd_model_names[0];
    char zz__44[45];
  };
  union {
    model_param_name_t efm_bd_model_names[0];
    char zz__45[45];
  };
  union {
    model_param_name_t trx_xc_model_names[0];
    char zz__46[40];
  };
  union {
    model_param_name_t trx_cl_model_names[0];
    char zz__47[35];
  };
  union {
    model_param_name_t trx_ma_model_names[0];
    char zz__48[45];
  };
  union {
    model_param_name_t trx_cy_model_names[0];
    char zz__49[35];
  };
  union {
    model_param_name_t trx_oh_model_names[0];
    char zz__50[30];
  };
  union {
    model_param_name_t trx_ch_model_names[0];
    char zz__51[30];
  };
  union {
    model_param_name_t trx_cb_model_names[0];
    char zz__52[35];
  };
  union {
    model_param_name_t trx_rs_model_names[0];
    char zz__53[20];
  };
  union {
    model_param_name_t trx_cp_model_names[0];
    char zz__54[45];
  };
  union {
    model_param_name_t trx_xt_model_names[0];
    char zz__55[40];
  };
  union {
    model_param_name_t trx_sd_model_names[0];
    char zz__56[45];
  };
  union {
    model_param_name_t trx_s2_model_names[0];
    char zz__57[45];
  };
  union {
    model_param_name_t trx_b2_model_names[0];
    char zz__58[45];
  };
  union {
    model_param_name_t trx_bd_model_names[0];
    char zz__59[45];
  };
  union {
    model_param_name_t gnd_pu_model_names[0];
    char zz__60[45];
  };
  union {
    model_param_name_t gnd_sw_model_names[0];
    char zz__61[45];
  };
  union {
    model_param_name_t gnd_im_model_names[0];
    char zz__62[25];
  };
  union {
    model_param_name_t gnd_ns_model_names[0];
    char zz__63[10];
  };
  union {
    model_param_name_t gnd_sn_model_names[0];
    char zz__64[45];
  };
};

extern const unsigned char __R_menu_options[] PROGMEM;
struct __T_menu_options {
  union {
    menu_option_t _MENU_OPTIONS[0];
    char zz__0[738];
  };
};

extern const unsigned char __R_page_entries[] PROGMEM;
struct __T_page_entries {
  union {
    PageSelectEntry Entries[0];
    char zz__0[360];
  };
};

extern const unsigned char __R_tuning[] PROGMEM;
struct __T_tuning {
  union {
    unsigned char tonal_tuning[0];
    char zz__0[64];
  };
  union {
    unsigned char e12_lt_tuning[0];
    char zz__1[40];
  };
  union {
    unsigned char e12_cb_tuning[0];
    char zz__2[48];
  };
  union {
    unsigned char e12_bc_tuning[0];
    char zz__3[48];
  };
  union {
    unsigned char efm_cy_tuning[0];
    char zz__4[30];
  };
  union {
    unsigned char efm_cb_tuning[0];
    char zz__5[30];
  };
  union {
    unsigned char trx_rs_tuning[0];
    char zz__6[12];
  };
  union {
    unsigned char trx_b2_tuning[0];
    char zz__7[24];
  };
  union {
    unsigned char gnd_sn_tuning[0];
    char zz__8[96];
  };
  union {
    unsigned char rom_tuning[0];
    char zz__9[48];
  };
  union {
    unsigned char trx_s2_tuning[0];
    char zz__10[19];
  };
  union {
    unsigned char trx_bd_tuning[0];
    char zz__11[24];
  };
  union {
    unsigned char trx_xt_tuning[0];
    char zz__12[24];
  };
  union {
    unsigned char trx_xc_tuning[0];
    char zz__13[24];
  };
  union {
    unsigned char trx_sd_tuning[0];
    char zz__14[12];
  };
  union {
    unsigned char trx_cl_tuning[0];
    char zz__15[21];
  };
  union {
    unsigned char efm_bd_tuning[0];
    char zz__16[48];
  };
  union {
    unsigned char efm_xt_tuning[0];
    char zz__17[24];
  };
  union {
    unsigned char efm_sd_tuning[0];
    char zz__18[30];
  };
  union {
    unsigned char efm_cp_tuning[0];
    char zz__19[66];
  };
  union {
    unsigned char efm_hh_tuning[0];
    char zz__20[30];
  };
  union {
    unsigned char efm_rs_tuning[0];
    char zz__21[48];
  };
};

