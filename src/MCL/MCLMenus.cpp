#include "MCLMenus.h"
#include "Project.h"
#include "ResourceManager.h"
#include "SeqPages.h"
#include "MCLSysConfig.h"
#include "MCLGUI.h"
#include "GridPages.h"
#include "AuxPages.h"
#include "MCLActions.h"

MCLEncoder options_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder config_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param3(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param4(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param5(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param6(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param7(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param8(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param9(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param10(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param11(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param12(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param13(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param14(0, 17, ENCODER_RES_SYS);


uint8_t opt_import_src = 0;
uint8_t opt_import_dest = 0;
uint8_t opt_import_count = 16;

void new_proj_handler() { proj.new_project_prompt(); }

const uint8_t *const menu_target_param[] PROGMEM = {
    nullptr,

    // 1
    &mcl_cfg.ram_page_mode,

    // 2
    &mcl_cfg.uart1_turbo_speed, &mcl_cfg.uart2_turbo_speed, &mcl_cfg.uart2_device,
    &mcl_cfg.clock_rec, &mcl_cfg.clock_send, &mcl_cfg.midi_forward_1,

    // 8
    &mcl_cfg.auto_normalize, &mcl_cfg.uart2_ctrl_chan,

    // 10
    &mcl_cfg.load_mode, &mcl_cfg.uart_cc_loopback, &mcl_cfg.usb_mode,

    // 13
    &mcl_cfg.display_mirror,

    // 14
    &opt_trackid, &SeqPage::mask_type, &SeqPage::pianoroll_mode,
    &SeqPage::param_select, &SeqPage::slide, &seq_ptc_page.transpose,
    &SeqPage::velocity, &SeqPage::cond, &opt_speed, &opt_length, &opt_channel,
    &opt_copy, &opt_clear, &opt_paste, &opt_shift, &opt_reverse,

    // 30
    &mcl_cfg.rec_automation,

    // 31
    &grid_page.grid_select_apply, &mcl_cfg.load_mode, &slot.link.loops,
    &slot.link.row, &grid_page.slot_apply, &grid_page.slot_clear,
    &grid_page.slot_copy, &grid_page.slot_paste, &slot.link.length,

    // 40
#ifdef WAV_DESIGNER
    &WavDesignerPage::opt_mode, &WavDesignerPage::opt_shape,
#else
    nullptr, nullptr,
#endif
    // 42
    &mcl_cfg.rec_quant,
    // 43
    &opt_import_src, &opt_import_dest, &opt_import_count,
    // 46
    &mcl_cfg.uart2_poly_chan,
    // 47
    &mcl_cfg.uart2_prg_in,
    // 48
    &mcl_cfg.uart2_prg_out,
    // 49
    &mcl_cfg.uart2_prg_mode,
    // 50
    &mcl_cfg.seq_dev,
    // 51
    &mcl_cfg.midi_forward_2,
    // 52
    &mcl_cfg.midi_forward_usb,
    // 53
    &mcl_cfg.midi_transport_rec,
    // 54
    &mcl_cfg.midi_transport_send,
    // 55
    &mcl_cfg.usb_turbo_speed,
    // 56
    &mcl_cfg.midi_ctrl_port,
    // 57
    &mcl_cfg.md_trig_channel,
    // 58
    &perf_page.page_mode,
    // 59
    &perf_page.perf_id,
    // 60
    &mcl_cfg.uart2_cc_mute,
    // 61
    &mcl_cfg.uart1_device,
    // 62
    &mcl_cfg.grid_page_mode,
    // 63,
    &opt_transpose,
};

const menu_function_ptr_t menu_target_functions[] PROGMEM = {
    { .fn = nullptr },
    // 1 - mclsys_apply_config
    { .fn = mclsys_apply_config },
    // 2 - new_proj_handler
    { .fn = new_proj_handler },
    // 3
    { .fn = opt_trackid_handler },
    { .fn = opt_mask_handler },
    { .fn = opt_speed_handler },
    { .fn = opt_length_handler },
    { .fn = opt_channel_handler },
    { .fn = opt_copy_track_handler_cb },
    { .fn = opt_clear_track_handler },
    { .fn = opt_clear_locks_handler },
    { .fn = opt_paste_track_handler },
    { .fn = opt_shift_track_handler },
    { .fn = opt_reverse_track_handler },
    // 14
    { .fn = seq_menu_handler },
    // 15
    { .fn = nullptr },
    { .fn = nullptr },
    { .fn = nullptr },
    { .fn = nullptr },
    // 19
    { .fn = nullptr },
    // 20
    { .fn = rename_row },
    // 21
    { .fn = apply_slot_changes_cb },
#ifdef WAV_DESIGNER
    // 22
    { .fn = wav_render },
    // 23
    { .fn = wavdesign_menu_handler },
#else
    // 22
    { .fn = nullptr },
    // 23
    { .fn = nullptr },
#endif
    // 24
    { .fn = mclsys_apply_config_midi },
    // 25
    { .fn = md_import },
    // 26
    { .fn = usb_dfu_mode },
    // 27
    { .fn = usb_os_update },
    // 28
    { .fn = usb_disk_mode },
    // 29
    { .fn = mcl_setup },
    // 30
    { .fn = rename_perf },
    // 31
    { .fn = opt_transpose_track_handler },
};
MenuPage<aux_config_page_N> aux_config_page(&config_param1, &config_param6);

MenuPage<boot_menu_page_N> boot_menu_page(&options_param1, &options_param2);
MenuPage<start_menu_page_N> start_menu_page(&options_param1, &options_param2);
MenuPage<system_menu_page_N> system_page(&options_param1, &options_param2);
MenuPage<midi_config_page_N> midi_config_page(&config_param1, &config_param3);

MenuPage<md_config_page_N> md_config_page(&config_param1, &config_param4);
MenuPage<chain_config_page_N> chain_config_page(&config_param1, &config_param6);
MenuPage<mcl_config_page_N> mcl_config_page(&config_param1, &config_param5);
MenuPage<md_import_page_N> md_import_page(&config_param1, &config_param8);

MenuPage<midiport_menu_page_N> midiport_menu_page(&config_param1, &config_param9);
MenuPage<midiprogram_menu_page_N> midiprogram_menu_page(&config_param1, &config_param10);
MenuPage<midiclock_menu_page_N> midiclock_menu_page(&config_param1, &config_param11);
MenuPage<midiroute_menu_page_N> midiroute_menu_page(&config_param1, &config_param12);
MenuPage<midimachinedrum_menu_page_N> midimachinedrum_menu_page(&config_param1, &config_param13);
MenuPage<midigeneric_menu_page_N> midigeneric_menu_page(&config_param1, &config_param14);

MCLEncoder input_encoder1(0, 127, ENCODER_RES_SYS);
MCLEncoder input_encoder2(0, 127, ENCODER_RES_SYS);

TextInputPage text_input_page(&input_encoder1, &input_encoder2);

MCLEncoder file_menu_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<file_menu_page_N> file_menu_page(&config_param1, &file_menu_encoder);

MCLEncoder seq_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder seq_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<seq_menu_page_N> seq_menu_page(&seq_menu_value_encoder, &seq_menu_entry_encoder);
MenuPage<perf_menu_page_N> perf_menu_page(&seq_menu_value_encoder, &seq_menu_entry_encoder);

MCLEncoder grid_slot_param1(0, 7, ENCODER_RES_PAT);
MCLEncoder grid_slot_param2(0, 16, ENCODER_RES_PAT);
MenuPage<grid_slot_page_N> grid_slot_page(&grid_slot_param1, &grid_slot_param2);

MCLEncoder wavdesign_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder wavdesign_menu_entry_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<wavdesign_menu_page_N> wavdesign_menu_page(&wavdesign_menu_value_encoder, &wavdesign_menu_entry_encoder);


