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
  static constexpr size_t __total_size = 1280;
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
  static constexpr size_t __total_size = 1258;
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
  static constexpr size_t __total_size = 1083;
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
  static constexpr size_t __total_size = 344;
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
  static constexpr size_t __total_size = 360;
};

extern const unsigned char __R_icons_knob[] PROGMEM;
struct __T_icons_knob {
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
  static constexpr size_t __total_size = 325;
};

extern const unsigned char __R_icons_page[] PROGMEM;
struct __T_icons_page {
  union {
    unsigned char icon_chroma[0];
    char zz__icon_chroma[75];
  };
  static constexpr size_t countof_icon_chroma = 75 / sizeof(unsigned char);
  static constexpr size_t sizeofof_icon_chroma = 75;
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
  static constexpr size_t __total_size = 957;
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
  static constexpr size_t __total_size = 1336;
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
  static constexpr size_t __total_size = 1099;
};

extern const unsigned char __R_machine_param_names[] PROGMEM;
struct __T_machine_param_names {
  union {
    model_param_name_t mnm_model_param_names[0];
    char zz__mnm_model_param_names[955];
  };
  static constexpr size_t countof_mnm_model_param_names = 955 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_mnm_model_param_names = 955;
  union {
    model_param_name_t md_model_param_names[0];
    char zz__md_model_param_names[2910];
  };
  static constexpr size_t countof_md_model_param_names = 2910 / sizeof(model_param_name_t);
  static constexpr size_t sizeofof_md_model_param_names = 2910;
  static constexpr size_t __total_size = 3865;
};

extern const unsigned char __R_menu_layouts[] PROGMEM;
struct __T_menu_layouts {
  union {
    menu_t<3> wav_menu_layout[0];
    char zz__wav_menu_layout[75];
  };
  static constexpr size_t countof_wav_menu_layout = 75 / sizeof(menu_t<3>);
  static constexpr size_t sizeofof_wav_menu_layout = 75;
  union {
    menu_t<grid_slot_page_N> slot_menu_layout[0];
    char zz__slot_menu_layout[180];
  };
  static constexpr size_t countof_slot_menu_layout = 180 / sizeof(menu_t<grid_slot_page_N>);
  static constexpr size_t sizeofof_slot_menu_layout = 180;
  union {
    menu_t<4> step_menu_layout[0];
    char zz__step_menu_layout[96];
  };
  static constexpr size_t countof_step_menu_layout = 96 / sizeof(menu_t<4>);
  static constexpr size_t sizeofof_step_menu_layout = 96;
  union {
    menu_t<19> seq_menu_layout[0];
    char zz__seq_menu_layout[411];
  };
  static constexpr size_t countof_seq_menu_layout = 411 / sizeof(menu_t<19>);
  static constexpr size_t sizeofof_seq_menu_layout = 411;
  union {
    menu_t<5> file_menu_layout[0];
    char zz__file_menu_layout[117];
  };
  static constexpr size_t countof_file_menu_layout = 117 / sizeof(menu_t<5>);
  static constexpr size_t sizeofof_file_menu_layout = 117;
  union {
    menu_t<1> mclconfig_menu_layout[0];
    char zz__mclconfig_menu_layout[33];
  };
  static constexpr size_t countof_mclconfig_menu_layout = 33 / sizeof(menu_t<1>);
  static constexpr size_t sizeofof_mclconfig_menu_layout = 33;
  union {
    menu_t<3> chain_menu_layout[0];
    char zz__chain_menu_layout[75];
  };
  static constexpr size_t countof_chain_menu_layout = 75 / sizeof(menu_t<3>);
  static constexpr size_t sizeofof_chain_menu_layout = 75;
  union {
    menu_t<3> mdconfig_menu_layout[0];
    char zz__mdconfig_menu_layout[75];
  };
  static constexpr size_t countof_mdconfig_menu_layout = 75 / sizeof(menu_t<3>);
  static constexpr size_t sizeofof_mdconfig_menu_layout = 75;
  union {
    menu_t<6> midiconfig_menu_layout[0];
    char zz__midiconfig_menu_layout[138];
  };
  static constexpr size_t countof_midiconfig_menu_layout = 138 / sizeof(menu_t<6>);
  static constexpr size_t sizeofof_midiconfig_menu_layout = 138;
  union {
    menu_t<1> rampage1_menu_layout[0];
    char zz__rampage1_menu_layout[33];
  };
  static constexpr size_t countof_rampage1_menu_layout = 33 / sizeof(menu_t<1>);
  static constexpr size_t sizeofof_rampage1_menu_layout = 33;
  union {
    menu_t<1> auxconfig_menu_layout[0];
    char zz__auxconfig_menu_layout[33];
  };
  static constexpr size_t countof_auxconfig_menu_layout = 33 / sizeof(menu_t<1>);
  static constexpr size_t sizeofof_auxconfig_menu_layout = 33;
  union {
    menu_t<9> system_menu_layout[0];
    char zz__system_menu_layout[201];
  };
  static constexpr size_t countof_system_menu_layout = 201 / sizeof(menu_t<9>);
  static constexpr size_t sizeofof_system_menu_layout = 201;
  static constexpr size_t __total_size = 1467;
};

extern const unsigned char __R_menu_options[] PROGMEM;
struct __T_menu_options {
  union {
    menu_option_t MENU_OPTIONS[0];
    char zz__MENU_OPTIONS[738];
  };
  static constexpr size_t countof_MENU_OPTIONS = 738 / sizeof(menu_option_t);
  static constexpr size_t sizeofof_MENU_OPTIONS = 738;
  static constexpr size_t __total_size = 738;
};

extern const unsigned char __R_page_entries[] PROGMEM;
struct __T_page_entries {
  union {
    PageSelectEntry Entries[0];
    char zz__Entries[360];
  };
  static constexpr size_t countof_Entries = 360 / sizeof(PageSelectEntry);
  static constexpr size_t sizeofof_Entries = 360;
  static constexpr size_t __total_size = 360;
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
  static constexpr size_t __total_size = 810;
};

