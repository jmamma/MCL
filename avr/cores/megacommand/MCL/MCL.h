/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCL_H__
#define MCL_H__

#include <string.h>
#include <midi-common.hh>

#include "WProgram.h"

#include "MD.h"
#include "A4.h"

#include "MCLGfx.h"
#include "MCLSysConfig.h"
#include "MCLSd.h"

#include "Project.h"

#include "MDExploit.h" //MDExploit should come before MCLSequencer for callback priority
#include "NoteInterface.h"
#include "Grid.h"
#include "MCLSeq.h"
#include "MCLActions.h"
#include "MDSysexCallbacks.h"
#include "TurboLight.h"
#include "MidiActivePeering.h"
#include "MidiSetup.h"

#include "GridPages.h"
#include "ProjectPages.h"
#include "SeqPages.h"
#include "AuxPages.h"
#include "MCLPages.h"

#include "GridEncoder.h"
#include "MCLEncoder.h"

#include "MDTrack.h"

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

extern uint8_t in_sysex;
extern uint8_t in_sysex2;
extern int8_t curpage;
extern uint8_t patternswitch;

extern MDPattern pattern_rec;
extern MDTrack temptrack;
extern MDSysexCallbacks md_callbacks;

class MCL {
public:
  void setup();
};

extern MCL mcl;

#endif /* MCL_H__ */
