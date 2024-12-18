/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGES_H__
#define SEQPAGES_H__

#include "MCLEncoder.h"
#include "MCLMemory.h"
#include "MCLMenus.h"
#include "ArpPage.h"

#define ENCODER_RES_SEQ 2
#define ENCODER_RES_PARAM 2

#define NUM_PARAM_PAGES 4

#define SEQ_MENU_TRACK 0
#define SEQ_MENU_DEVICE 1
#define SEQ_MENU_MASK 2
#define SEQ_MENU_PIANOROLL 3
#define SEQ_MENU_PARAMSELECT 4
#define SEQ_MENU_SLIDE 5
#define SEQ_MENU_ARP 6
#define SEQ_MENU_KEY 7
#define SEQ_MENU_VEL 8
#define SEQ_MENU_PROB 9
#define SEQ_MENU_SPEED 10
#define SEQ_MENU_LENGTH_MD 11
#define SEQ_MENU_LENGTH_EXT 12
#define SEQ_MENU_CHANNEL 13
#define SEQ_MENU_COPY 14
#define SEQ_MENU_CLEAR_TRACK 15
#define SEQ_MENU_CLEAR_LOCKS 16
#define SEQ_MENU_PASTE 17
#define SEQ_MENU_SHIFT 18
#define SEQ_MENU_REVERSE 19
#define SEQ_MENU_TRANSPOSE 20
#define SEQ_MENU_POLY 21
#define SEQ_MENU_QUANT 22
#define SEQ_MENU_AUTOMATION 23
#define SEQ_MENU_SOUND 24

extern MCLEncoder seq_param1;
extern MCLEncoder seq_param2;
extern MCLEncoder seq_param3;
extern MCLEncoder seq_param4;

extern MCLEncoder seq_extparam1;
extern MCLEncoder seq_extparam2;
extern MCLEncoder seq_extparam3;
extern MCLEncoder seq_extparam4;

extern MCLEncoder seq_lock1;
extern MCLEncoder seq_lock2;

#include "SeqPtcPage.h"
#include "SeqStepPage.h"

#ifdef EXT_TRACKS
#include "SeqExtStepPage.h"
extern uint8_t last_ext_track;
#endif

extern uint8_t last_md_track;

extern SeqStepPage seq_step_page;

#ifdef EXT_TRACKS
extern SeqExtStepPage seq_extstep_page;
#endif

extern MCLEncoder ptc_param_oct;
extern MCLEncoder ptc_param_fine_tune;
extern MCLEncoder ptc_param_len;
extern MCLEncoder ptc_param_scale;

extern SeqPtcPage seq_ptc_page;
extern ArpPage arp_page;

extern void mcl_save_sound();
extern void mcl_load_sound();

#endif /* SEQPAGES_H__ */
