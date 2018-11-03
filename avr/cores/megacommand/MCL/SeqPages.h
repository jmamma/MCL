/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGES_H__
#define SEQPAGES_H__

#include "MCLEncoder.h"
#define ENCODER_RES_SEQ 2
#define ENCODER_RES_PARAM 2
#define NUM_PARAM_PAGES 2
#define NUM_LFO_PAGES 4

extern MCLEncoder seq_param1;
extern MCLEncoder seq_param2;
extern MCLEncoder seq_param3;
extern MCLEncoder seq_param4;

extern MCLEncoder seq_lock1;
extern MCLEncoder seq_lock2;

#include "SeqParamPage.h"
#include "SeqStepPage.h"
#include "SeqExtStepPage.h"
#include "SeqRtrkPage.h"
#include "SeqRlckPage.h"
#include "SeqPtcPage.h"

extern uint8_t last_ext_track;

extern SeqParamPage seq_param_page[NUM_PARAM_PAGES];
extern SeqStepPage seq_step_page;
extern SeqRtrkPage seq_rtrk_page;
extern SeqRlckPage seq_rlck_page;
extern SeqExtStepPage seq_extstep_page;
extern SeqPtcPage seq_ptc_page;

class SeqPages {
public:
};
extern SeqPages seq_pages;
#endif /* SEQPAGES_H__ */
