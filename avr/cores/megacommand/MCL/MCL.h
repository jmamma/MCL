/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCL_H__
#define MCL_H__

#include "MD.h"
#include "A4.h"

#include "MCLGfx.h"
#include "MCLMacros.h"
#include "MCLSysConfig.h"
#include "MCLSd.h"
#include "MCLSequencer.h"
#include "MDExploit.h"
#include "MDSysexCallbacks.h"
#include "MidiActivePeering.h"
#include "MidiSetup.h"

#include "GridPages.h"
#include "ProjectPages.h"
#include "SeqPages.h"
#include "AuxPages.h"

#include "GridEncoder.h"
#include "MCLEncoder.h"

extern MCLEncoder options_param1(0, 5, ENCODER_RES_SYS);
extern MCLEncoder options_param2(0, 3, ENCODER_RES_SYS);
extern MCLSystemPage system_page(&options_param1, &options_param2);

extern uint8_t in_sysex;
extern uint8_t in_sysex;
extern uint8_t in_sysex2;
extern int8_t curpage;

extern float frames_fps;
extern uint16_t frames;
extern uint16_t frames_startclock;


class MCL {
public:
  void setup();
};

extern MCL mcl;

#endif /* MCL_H__ */
