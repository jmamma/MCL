extern MCLEncoder seq_param1(0, 3, ENCODER_RES_SEQ);
extern MCLEncoder seq_param2(0, 64, ENCODER_RES_SEQ);
extern MCLEncoder seq_param3(0, 10, ENCODER_RES_SEQ);
extern MCLEncoder seq_param4(0, 16, ENCODER_RES_SEQ);

extern SeqParamPage seq_param_page[NUM_PARAM_PAGES];
extern SeqStepPage seq_step_page(&seq_param1, &seq_param2, &seq_param3,
                                 &seq_param4);
extern SeqExtStep seq_extstep_page(&seq_param1, &seq_param2, &seq_param3,
                                   &seq_param4);
extern SeqRtrkPage seq_rtrk_page(&seq_param1, &seq_param2, &seq_param3,
                                 &seq_param4);
extern SeqRlckPage seq_rlck_page(&seq_param1, &seq_param2, &seq_param3,
                                 &seq_param4);
extern SeqExtStepPage seq_extstep_page(&seq_param1, &seq_param2, &seq_param3,
                                       &seq_param4);
extern SeqPtcPage seq_ptc_page(&seq_param1, &seq_param2, &seq_param3,
                               &seq_param4);
extern SeqRPtcPage seq_rptc_page(&seq_param1, &seq_param2, &seq_param3,
                                 &seq_param4);
