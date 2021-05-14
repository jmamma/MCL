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

#define NUM_PARAM_PAGES 4


#define SEQ_MENU_TRACK 0
#define SEQ_MENU_MASK 1
#define SEQ_MENU_PIANOROLL 2
#define SEQ_MENU_PARAMSELECT 3
#define SEQ_MENU_SLIDE 4
#define SEQ_MENU_ARP 5
#define SEQ_MENU_TRANSPOSE 6
#define SEQ_MENU_VEL 7
#define SEQ_MENU_PROB 8
#define SEQ_MENU_SPEED 9
#define SEQ_MENU_LENGTH 10
#define SEQ_MENU_CHANNEL 11
#define SEQ_MENU_COPY 12
#define SEQ_MENU_CLEAR_TRACK 13
#define SEQ_MENU_CLEAR_LOCKS 14
#define SEQ_MENU_PASTE 15
#define SEQ_MENU_SHIFT 16
#define SEQ_MENU_REVERSE 17
#define SEQ_MENU_POLY 18

extern MCLEncoder seq_param1;
extern MCLEncoder seq_param2;
extern MCLEncoder seq_param3;
extern MCLEncoder seq_param4;

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
