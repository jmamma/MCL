#include "MCLMenus.h"
#include "MCL_impl.h"
#include "Project.h"
#include "ResourceManager.h"

MCLEncoder options_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder config_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param3(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param4(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param5(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param6(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param7(0, 17, ENCODER_RES_SYS);

void new_proj_handler() { proj.new_project_prompt(); }

const Page *const menu_target_pages[] PROGMEM = {
    nullptr,

    // 1 - load_proj_page
    (Page *)&load_proj_page,
    (Page *)&convert_proj_page,
    (Page *)&midi_config_page,
    (Page *)&md_config_page,
    (Page *)&chain_config_page,
    (Page *)&aux_config_page,

    // 7
    (Page *)&mcl_config_page,

    // 8 - ram_config_page
    (Page *)&ram_config_page,

    // 9
    (Page *)&poly_page,
    // 10
    (Page *)&arp_page,
};

const uint8_t *const menu_target_param[] PROGMEM = {
    nullptr,

    // 1
    &mcl_cfg.ram_page_mode,

    // 2
    &mcl_cfg.uart1_turbo, &mcl_cfg.uart2_turbo, &mcl_cfg.uart2_device,
    &mcl_cfg.clock_rec, &mcl_cfg.clock_send, &mcl_cfg.midi_forward,

    // 8
    &mcl_cfg.auto_normalize, &mcl_cfg.uart2_ctrl_mode,

    // 10
    &mcl_cfg.chain_mode, &mcl_cfg.link_rand_min, &mcl_cfg.link_rand_max,

    // 13
    &mcl_cfg.display_mirror,

    // 14
    &opt_trackid, &SeqPage::mask_type, &SeqPage::pianoroll_mode,
    &SeqPage::param_select, &SeqPage::slide, &seq_ptc_page.key,
    &SeqPage::velocity, &SeqPage::cond, &opt_speed, &opt_length, &opt_channel,
    &opt_copy, &opt_clear, &opt_paste, &opt_shift, &opt_reverse,

    // 30
    &opt_clear_step,

    // 31
    &grid_page.grid_select_apply, &mcl_cfg.chain_mode, &slot.link.loops,
    &slot.link.row, &grid_page.slot_apply, &grid_page.slot_clear,
    &grid_page.slot_copy, &grid_page.slot_paste, &slot.link.length,

    // 40
    &WavDesignerPage::opt_mode, &WavDesignerPage::opt_shape,

    // 42 - end
};

const menu_function_t menu_target_functions[] PROGMEM = {
    nullptr,
    // 1 - mclsys_apply_config
    mclsys_apply_config,
    // 2 - new_proj_handler
    new_proj_handler,
    // 3
    opt_trackid_handler,
    opt_mask_handler,
    opt_speed_handler,
    opt_length_handler,
    opt_channel_handler,
    opt_copy_track_handler,
    opt_clear_track_handler,
    opt_clear_locks_handler,
    opt_paste_track_handler,
    opt_shift_track_handler,
    opt_reverse_track_handler,
    // 14
    seq_menu_handler,
    // 15
    opt_clear_step_locks_handler,
    opt_copy_step_handler,
    opt_paste_step_handler,
    opt_mute_step_handler,
    // 19
    step_menu_handler,
    // 20
    rename_row,
    // 21
    apply_slot_changes_cb,
    // 22
    wav_render,
    // 23
    wav_menu_handler,
    // 24
    mclsys_apply_config_midi,
};

MenuPage<1> aux_config_page(&config_param1, &config_param6);
MenuPage<7> system_page(&options_param1, &options_param2);
MenuPage<6> midi_config_page(&config_param1, &config_param3);
MenuPage<3> md_config_page(&config_param1, &config_param4);
MenuPage<3> chain_config_page(&config_param1, &config_param6);
MenuPage<1> mcl_config_page(&config_param1, &config_param5);
MenuPage<1> ram_config_page(&config_param1, &config_param7);

MCLEncoder input_encoder1(0, 127, ENCODER_RES_SYS);
MCLEncoder input_encoder2(0, 127, ENCODER_RES_SYS);

TextInputPage text_input_page(&input_encoder1, &input_encoder2);

MCLEncoder file_menu_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<5> file_menu_page(&config_param1, &file_menu_encoder);

MCLEncoder seq_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder seq_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<19> seq_menu_page(&seq_menu_value_encoder, &seq_menu_entry_encoder);

MCLEncoder step_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder step_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<4> step_menu_page(&step_menu_value_encoder, &step_menu_entry_encoder);

MCLEncoder grid_slot_param1(0, 7, ENCODER_RES_PAT);
MCLEncoder grid_slot_param2(0, 16, ENCODER_RES_PAT);
MenuPage<grid_slot_page_N> grid_slot_page(&grid_slot_param1, &grid_slot_param2);

MCLEncoder wav_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder wav_menu_entry_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<3> wav_menu_page(&wav_menu_value_encoder, &wav_menu_entry_encoder);
