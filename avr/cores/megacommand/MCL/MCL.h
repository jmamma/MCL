/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCL_H__
#define MCL_H__

#include "MCLGfx.h"
#include "MCLMacros.h"
#include "MCLSysConfig.h"
#include "MD.h"
#include "MDExploit.h"
#include "MidiActivePeering.h"
#include "MidiSetup.h"
#include "SeqPages.h"

extern uint8_t in_sysex;
extern uint8_t in_sysex2;
extern int8_t curpage;

class MCL {
public:
  void setup();
}

extern MCL mcl;

#endif /* MCL_H__ */
