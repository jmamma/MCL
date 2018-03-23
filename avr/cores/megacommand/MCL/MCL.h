/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCL_H__
#define MCL_H__

#include <string.h>
#include <midi-common.hh>

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

extern uint8_t in_sysex;
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
