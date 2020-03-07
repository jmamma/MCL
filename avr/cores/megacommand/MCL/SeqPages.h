/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGES_H__
#define SEQPAGES_H__

#include "MCLEncoder.h"
#include "MCLMemory.h"
#include "MCLMenus.h"
#include "ArpPage.h"

#ifdef OLED_DISPLAY
#define ENCODER_RES_SEQ 2
#define ENCODER_RES_PARAM 2
#else
#define ENCODER_RES_SEQ 2
#define ENCODER_RES_PARAM 2
#endif

#define NUM_PARAM_PAGES 2

extern MCLEncoder seq_param1;
extern MCLEncoder seq_param2;
extern MCLEncoder seq_param3;
extern MCLEncoder seq_param4;

extern MCLEncoder seq_lock1;
extern MCLEncoder seq_lock2;

#include "SeqParamPage.h"
#include "SeqPtcPage.h"
#include "SeqRlckPage.h"
#include "SeqRtrkPage.h"
#include "SeqStepPage.h"

#ifdef EXT_TRACKS
#include "SeqExtStepPage.h"
extern uint8_t last_ext_track;
#endif

extern SeqParamPage seq_param_page[NUM_PARAM_PAGES];
extern SeqStepPage seq_step_page;
extern SeqRtrkPage seq_rtrk_page;
extern SeqRlckPage seq_rlck_page;

#ifdef EXT_TRACKS
extern SeqExtStepPage seq_extstep_page;
#endif

extern MCLEncoder ptc_param_oct;
extern MCLEncoder ptc_param_finetune;
extern MCLEncoder ptc_param_len;
extern MCLEncoder ptc_param_scale;

extern SeqPtcPage seq_ptc_page;
extern ArpPage arp_page;

extern MCLEncoder seq_menu_value_encoder;
extern MCLEncoder seq_menu_entry_encoder;
extern MenuPage<9> seq_menu_page;

extern MCLEncoder step_menu_value_encoder;
extern MCLEncoder step_menu_entry_encoder;
extern MenuPage<4> step_menu_page;

extern void mcl_save_sound();
extern void mcl_load_sound();

#endif /* SEQPAGES_H__ */
