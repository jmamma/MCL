/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef CCHANDLER_H__
#define CCHANDLER_H__

#include "Encoders.hh"
#include "Circular.hh"
#include "Midi.h"

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_tools Midi Tools
 *
 * @{
 **/

/**
 * \addtogroup midi_cc_handler CC Handler Class
 *
 * @{
 **/

typedef void (*midi_learn_cc_callback_t)(CCEncoder *enc);

void CCHandlerOnCCCallback(uint8_t *msg);

typedef struct incoming_cc_s {
	/**
	 * \addtogroup midi_cc_handler
	 *
	 * @{
	 **/
	
  uint8_t channel;
  uint8_t cc;
  uint8_t value;

	/* @} */
} incoming_cc_t;

class CCHandler : public MidiCallback {
	/**
	 * \addtogroup midi_cc_handler
	 *
	 * @{
	 **/
	
 public:
  CircularBuffer<incoming_cc_t, 4> incomingCCs;
  midi_learn_cc_callback_t callback;

  CCHandler() {
    callback = NULL;
    midiLearnEnc = NULL;
  }

  void setup();
  void destroy();
  void onCCCallback(uint8_t *msg);
  void onOutgoingCCCallback(uint8_t *msg);

  Vector<CCEncoder *, 64> encoders;
  CCEncoder *midiLearnEnc;
  void midiLearn(CCEncoder *enc) {
    midiLearnEnc = enc;
  }
  void addEncoder(CCEncoder *enc) {
    encoders.add(enc);
  }
  void removeEncoder(CCEncoder *enc) {
    encoders.remove(enc);
  }

  void setCallback(midi_learn_cc_callback_t _callback) {
    callback = _callback;
  }

	/* @} */
};

extern CCHandler ccHandler;

#endif /* CCHANDLER_H__ */
