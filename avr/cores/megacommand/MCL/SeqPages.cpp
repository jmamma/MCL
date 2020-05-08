#include "MCL.h"

MCLEncoder seq_param1(0, 3, ENCODER_RES_SEQ);
MCLEncoder seq_param2(0, 4, ENCODER_RES_SEQ);
MCLEncoder seq_param3(0, 10, ENCODER_RES_SEQ);
MCLEncoder seq_param4(0, 64, ENCODER_RES_SEQ);

MCLEncoder seq_lock1(0, 127, ENCODER_RES_PARAM);
MCLEncoder seq_lock2(0, 127, ENCODER_RES_PARAM);

MCLEncoder ptc_param_oct(0, 8, ENCODER_RES_SEQ);
MCLEncoder ptc_param_finetune(0, 64, ENCODER_RES_SEQ);
MCLEncoder ptc_param_len(0, 64, ENCODER_RES_SEQ);
MCLEncoder ptc_param_scale(0, 23, ENCODER_RES_SEQ);

SeqParamPage seq_param_page[NUM_PARAM_PAGES];
SeqStepPage seq_step_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);
SeqRtrkPage seq_rtrk_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);
SeqRlckPage seq_rlck_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);

#ifdef EXT_TRACKS
uint8_t last_ext_track;
SeqExtStepPage seq_extstep_page(&seq_param1, &seq_param2, &seq_param3,
                                &seq_param4);
#endif

SeqPtcPage seq_ptc_page(&ptc_param_oct, &ptc_param_finetune, &ptc_param_len, &ptc_param_scale);

ArpPage arp_page(&arp_und, &arp_mode, &arp_speed, &arp_oct);

const menu_t<10> seq_menu_layout PROGMEM = {
    "SEQ",
    {
        {"ARPEGGIATOR", 0, 0, 0, (uint8_t *)NULL, (Page *) &arp_page, NULL, {}},
        {"TRANSPOSE:", 0, 12, 0, (uint8_t *)&seq_ptc_page.key, (Page *) NULL, NULL, {}},
        {"TRACK SEL:", 1, 17, 0, (uint8_t *)&opt_trackid, (Page *)NULL, opt_trackid_handler, {}},
        {"COPY:", 0, 3, 3, (uint8_t *)&opt_copy, (Page *)NULL, opt_copy_track_handler, { {0, "--",}, {1, "TRK"}, {2, "ALL"}}},
        {"CLEAR:", 0, 3, 3, (uint8_t *)&opt_clear, (Page *)NULL, opt_clear_track_handler, { {0, "--",}, {1, "TRK"}, {2, "ALL"}}},
        {"CLEAR:", 0, 3, 3, (uint8_t *)&opt_clear, (Page *)NULL, opt_clear_locks_handler, { {0, "--",}, {1, "LCKS"}, {2, "ALL"}}},
        {"PASTE:", 0, 3, 3, (uint8_t *)&opt_paste, (Page *)NULL, opt_paste_track_handler, { {0, "--",}, {1, "TRK"}, {2, "ALL"}}},
        {"TRACK RES:", 1, 3, 2, (uint8_t *)&opt_resolution, (Page *)NULL, opt_resolution_handler, { {1, "2x"}, {2, "1x"} }},
        {"SHIFT:", 0, 5, 5, (uint8_t *)&opt_shift, (Page *)NULL, opt_shift_track_handler, { {0, "--",}, {1, "L"}, {2, "R"}, {3,"L>ALL"}, {4, "R>ALL"}}},
        {"REVERSE:", 0, 3, 3, (uint8_t *)&opt_reverse, (Page *)NULL, opt_reverse_track_handler, { {0, "--",}, {1, "TRK"}, {2, "ALL"} }},
    },
    seq_menu_handler,
};

MCLEncoder seq_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder seq_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<10> seq_menu_page(&seq_menu_layout, &seq_menu_value_encoder, &seq_menu_entry_encoder);

const menu_t<4> step_menu_layout PROGMEM = {
    "STP",
    {
        {"CLEAR:", 0, 2, 2, (uint8_t *)&opt_clear_step, (Page *)NULL, opt_clear_step_locks_handler, { {0, "--",}, {1, "LCKS"}}},
        {"COPY STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_copy_step_handler, {}},
        {"PASTE STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_paste_step_handler, {}},
        {"MUTE STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_mute_step_handler, {}},
    },
    step_menu_handler,
};

MCLEncoder step_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder step_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<4> step_menu_page(&step_menu_layout, &step_menu_value_encoder, &step_menu_entry_encoder);


//SeqLFOPage seq_lfo_page[NUM_LFO_PAGES];
