/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCL_H__
#define MCL_H__

#include "MCLGfx.hh"
#include "MCLMacros.hh"
#include "MCLSysConfig.hh"
#include "MD.h"
#include "MDExploit.hh"
#include "MidiActivePeering.hh"
#include "MidiSetup.hh"
#include "SeqPages.hh"

extern uint8_t in_sysex;
extern uint8_t in_sysex2;
extern int8_t curpage;

class MCL {
public:
  void setup();
}

extern MCL mcl;

#endif /* MCL_H__ */
