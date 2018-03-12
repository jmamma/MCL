/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MDSYSEXCALLBACKS_H__
#define MDSYSEXCALLBACKS_H__
#include "MCL.h"

class MDSysexCallbacks : public MDCallback {

public:
  /*Tell the MIDI-CTRL framework to execute the following methods when callbacks
    for Pattern and Kit messages are received.*/
  void setup();
  void onStatusResponseCallback(uint8_t type, uint8_t value);
  void onPatternMessages();
  void onKitMessages();
};
extern;

#endif /* MDSYSEXCALLBACKS_H__ */
