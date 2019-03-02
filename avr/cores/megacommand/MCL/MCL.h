/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCL_H__
#define MCL_H__

#include <midi-common.hh>
#include <string.h>

#include "A4.h"
#include "MD.h"
#include "WProgram.h"

#ifdef MEGACOMMAND
  #define WAV_DESIGNER 1
#endif

#include "MCLGfx.h"
#include "MCLSd.h"
#include "MCLSysConfig.h"

#include "PolyPage.h"
#include "Project.h"

#include "SoundBrowserPage.h"
#include "Grid.h"
#include "GridChain.h"
#include "GridRowHeader.h"
#include "GridTask.h"
#include "MCLActions.h"
#include "MCLGUI.h"
#include "MCLMemory.h"
#include "MCLSeq.h"
#include "MDEvents.h"
#include "MDExploit.h"
#include "MDSound.h"
#include "Menu.h"
#include "MenuPage.h"
#include "MidiActivePeering.h"
#include "MidiID.hh"
#include "MidiIDSysex.hh"
#include "MidiSDS.hh"
#include "MidiSDSSysex.hh"
#include "MidiSetup.h"
#include "NoteInterface.h"
#include "TurboLight.h"

#include "AuxPages.h"
#include "GridPages.h"
#include "MCLMenus.h"

#ifdef WAV_DESIGNER
#include "Osc.h"
#include "OscMixerPage.h"
#include "OscPage.h"
#include "Wav.h"
#include "WavDesigner.h"
#include "DSP.h"
#endif

#include "PageSelectPage.h"
#include "ProjectPages.h"
#include "SeqPages.h"

#include "GridEncoder.h"
#include "MCLEncoder.h"

#include "MDTrack.h"
//#include "EmptyTrack.h"

#include "Shared.h"

//MCL Fonts
#ifdef OLED_DISPLAY
#include "Fonts/TomThumb.h"
#include "Fonts/Elektrothic.h"
#endif

#define VERSION 2021

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

#endif /* MCL_H__ */
