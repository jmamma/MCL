/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include <midi-common.hh>
#include <string.h>

#include "WProgram.h"

#ifdef MEGACOMMAND
  #define WAV_DESIGNER
  #define LOUDNESS_PAGE
  #define SOUND_PAGE
#endif

//MCL Fonts
#ifdef OLED_DISPLAY
#include "Fonts/TomThumb.h"
#include "Fonts/Elektrothic.h"
#endif

#define VERSION 3000
#define VERSION_STR "3.00"

#define CALLBACK_TIMEOUT 500
#define GUI_NAME_TIMEOUT 800

#define CUE_PAGE 5
#define NEW_PROJECT_PAGE 7
#define MIXER_PAGE 10
#define S_PAGE 3
#define W_PAGE 4
#define SEQ_STEP_PAGE 1
#define SEQ_EXTSTEP_PAGE 18
#define SEQ_PTC_PAGE 16
#define SEQ_EUC_PAGE 20
#define SEQ_EUCPTC_PAGE 21
#define SEQ_RLCK_PAGE 13
#define SEQ_RTRK_PAGE 11
#define SEQ_RPTC_PAGE 14
#define LOAD_PROJECT_PAGE 8

#define MD_KITBUF_POS 63

// Memory layout for SRAM bank 1

extern int8_t curpage;
extern uint8_t patternswitch;

class MCL {
public:
  void setup();
};

extern MCL mcl;

