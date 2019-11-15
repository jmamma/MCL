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
MCLEncoder ptc_param_scale(0, 15, ENCODER_RES_SEQ);

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

const menu_t<7> seq_menu_layout PROGMEM = {
    "SEQ",
    {
        {"TRACK SEL.", 1, 17, 0, (uint8_t *)&opt_trackid, (Page *)NULL, opt_trackid_handler, {}},
        {"COPY TRK.", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"CLEAR", 0, 2, 2, (uint8_t *)&opt_clearall, (Page *)NULL, opt_clear_track_handler, { {0, "TRK."}, {1, "ALL"}}},
        {"CLEAR", 0, 0, 2, (uint8_t *)&opt_clearall, (Page *)NULL, opt_clear_locks_handler, { {0, "LCKS."}, {1, "ALL"}}},
        {"PASTE TRK.", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"TRACK RES.", 1, 3, 2, (uint8_t *)&opt_resolution, (Page *)NULL, opt_resolution_handler, { {1, "2x"}, {2, "1x"} }},
        {"STEP SHIFT", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
    },
    NULL,
};

MCLEncoder seq_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder seq_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<7> seq_menu_page(&seq_menu_layout, &seq_menu_value_encoder, &seq_menu_entry_encoder);

//SeqLFOPage seq_lfo_page[NUM_LFO_PAGES];
