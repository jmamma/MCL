/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGES_H__
#define SEQPAGES_H__

#include "MCLEncoder.h"
#include "MCLMenus.h"

#ifdef OLED_DISPLAY
#define ENCODER_RES_SEQ 2
#define ENCODER_RES_PARAM 2
#else
#define ENCODER_RES_SEQ 2
#define ENCODER_RES_PARAM 2
#endif

#define NUM_PARAM_PAGES 2
#define NUM_LFO_PAGES 4

extern MCLEncoder seq_param1;
extern MCLEncoder seq_param2;
extern MCLEncoder seq_param3;
extern MCLEncoder seq_param4;

extern MCLEncoder seq_lock1;
extern MCLEncoder seq_lock2;

#include "SeqExtStepPage.h"
#include "SeqParamPage.h"
#include "SeqPtcPage.h"
#include "SeqRlckPage.h"
#include "SeqRtrkPage.h"
#include "SeqStepPage.h"

extern uint8_t last_ext_track;

extern SeqParamPage seq_param_page[NUM_PARAM_PAGES];
extern SeqStepPage seq_step_page;
extern SeqRtrkPage seq_rtrk_page;
extern SeqRlckPage seq_rlck_page;
extern SeqExtStepPage seq_extstep_page;
extern SeqPtcPage seq_ptc_page;

extern MCLEncoder track_menu_param1;
extern MCLEncoder track_menu_param2;
extern MenuPage track_menu_page;

const menu_t track_menu_layout PROGMEM = {
    "TRACk",
    3,
    {
        {"LENGTH:", 0, 64, 0, (uint8_t *) &SeqPage::length, (Page*) NULL, (void*)NULL, {}},
        {"MULTI:", 1, 2, 2, (uint8_t *) &SeqPage::resolution, (Page*) NULL, (void*)NULL, {{1, "1x"},{2, "2x"}}},

        {"APPLY:", 0, 1, 2, (uint8_t *) &SeqPage::apply, (Page*) NULL, (void*)NULL, {{1, "--"},{1, "ALL"}}},
   },

    (void*)(&mclsys_apply_config),

};


#endif /* SEQPAGES_H__ */
