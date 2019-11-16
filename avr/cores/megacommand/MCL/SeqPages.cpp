#include "MCL.h"

const menu_t<5> track_menu_layout PROGMEM = {
  "TRACk",
  {
    {"LENGTH:", 0, 64, 0, (uint8_t *) &SeqPage::length, (Page*) NULL, NULL, {}},
    {"MULTI:", 1, 2, 2, (uint8_t *) &SeqPage::resolution, (Page*) NULL, NULL, {{1, "1x"},{2, "2x"}}},

    {"APPLY:", 0, 1, 2, (uint8_t *) &SeqPage::apply, (Page*) NULL, NULL, {{1, "--"},{1, "ALL"}}},
//    {"LOAD SND:", 0,  0, 0, (uint8_t *) NULL, (Page*) NULL, (void*) &mcl_load_sound, {}},
//    {"SAVE SND:", 0, 0, 0, (uint8_t *) NULL, (Page*) NULL, (void*) &mcl_save_sound, {}},
  },
  (&mclsys_apply_config),
};

MCLEncoder seq_param1(0, 3, ENCODER_RES_SEQ);
MCLEncoder seq_param2(0, 4, ENCODER_RES_SEQ);
MCLEncoder seq_param3(0, 10, ENCODER_RES_SEQ);
MCLEncoder seq_param4(0, 64, ENCODER_RES_SEQ);

MCLEncoder trackselect_enc(0, 15, ENCODER_RES_SEQ);

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
MCLEncoder track_menu_param1(0, 8, ENCODER_RES_PAT);
MCLEncoder track_menu_param2(0, 8, ENCODER_RES_PAT);
MenuPage<5> track_menu_page(&track_menu_layout, &track_menu_param1, &track_menu_param2);


//SeqLFOPage seq_lfo_page[NUM_LFO_PAGES];
