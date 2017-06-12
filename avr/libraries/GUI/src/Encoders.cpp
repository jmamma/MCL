/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "Encoders.hh"
#include "MidiTools.h"
#include "Midi.h"

#include "GUI.h"

/* handlers */

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_encoders Encoder classes
 *
 * @{
 *
 * \file
 * Encoder classes
 **/
class Encoder;

/**
 * Handle a change in a CCEncoder by sending out the CC, using the
 * channel and cc out of the CCEncoder object.
 **/
void CCEncoderHandle(Encoder *enc) {
	CCEncoder *ccEnc = (CCEncoder *)enc;
	uint8_t channel = ccEnc->getChannel();
	uint8_t cc = ccEnc->getCC();
	uint8_t value = ccEnc->getValue();
	
	MidiUart.sendCC(channel, cc, value);
}

/**
 * Handle a change in a VarRangeEncoder by setting the variable pointed to by enc->var.
 **/
void VarRangeEncoderHandle(Encoder *enc) {
	VarRangeEncoder *rEnc = (VarRangeEncoder *)enc;
	if (rEnc->var != NULL) {
		*(rEnc->var) = rEnc->getValue();
	}
}

#ifndef HOST_MIDIDUINO
#include <MidiClock.h>

/**
 * Handle an encoder change by setting the MidiClock tempo to the encoder value.
 **/
void TempoEncoderHandle(Encoder *enc) {
	MidiClock.setTempo(enc->getValue());
}
#endif

Encoder::Encoder(const char *_name, encoder_handle_t _handler) {
	old = 0;
	cur = 0;
	redisplay = false;
	setName(_name);
	handler = _handler;
	fastmode = true;
	pressmode = false;
}

void Encoder::checkHandle() {
	if (cur != old) {
		if (handler != NULL)
			handler(this);
	}
	
	old = cur;
}

void Encoder::setName(const char *_name) {
	if (_name != NULL)
		m_strncpy_fill(name, _name, 4);
	name[3] = '\0';
}

void Encoder::setValue(int value, bool handle) {
	if (handle) {
		cur = value;
		checkHandle();
	} else {
		old = cur = value;
	}
	redisplay = true;
}

void Encoder::displayAt(int i) {
	GUI.put_value(i, getValue());
	redisplay = false;
}

bool Encoder::hasChanged() {
	return old != cur;
}

void Encoder::clear() {
	old = 0;
	cur = 0;
}

int Encoder::update(encoder_t *enc) {
	cur = cur + enc->normal + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
	return cur;
}

/* EnumEncoder */
void EnumEncoder::displayAt(int i) {
	GUI.put_string_at(i * 4, enumStrings[getValue()]);
	redisplay = false;
}

void PEnumEncoder::displayAt(int i) {
	//  GUI.put_p_string_at_fill(i * 4, (PGM_P)(pgm_read_word(enumStrings[getValue()])));
	GUI.put_p_string_at(i * 4, (PGM_P)(enumStrings[getValue()]));
	redisplay = false;
}


/* RangeEncoder */

int RangeEncoder::update(encoder_t *enc) {
	int inc = enc->normal + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
	
	cur = limit_value(cur, inc, min, max);
	return cur;
}

/* CharEncoder */
CharEncoder::CharEncoder()  : RangeEncoder(0, 37) {
}

char CharEncoder::getChar() {
	uint8_t val = getValue();
	if (val == 0) {
		return ' ';
	}
	if (val < 27) 
		return val - 1+ 'A';
	else
		return (val - 27) + '0';
}

void CharEncoder::setChar(char c) {
	if (c >= 'A' && c <= 'Z') {
		setValue(c - 'A' + 1);
	} else if (c >= '0' && c <= '9') {
		setValue(c - '0' + 26 + 1);
	} else {
		setValue(0);
	}
}

/* notePitchEncoder */
NotePitchEncoder::NotePitchEncoder(char *_name) : RangeEncoder(0, 127, _name) {
}

void NotePitchEncoder::displayAt(int i) {
	char name[5];
	getNotePitch(getValue(), name);
	GUI.put_string_at(i * 4, name);
}

void MidiTrackEncoder::displayAt(int i) {
	GUI.put_value(i, getValue() + 1);
}

void AutoNameCCEncoder::initCCEncoder(uint8_t _channel, uint8_t _cc) {
	CCEncoder::initCCEncoder(_channel, _cc);
	setCCName();
	GUI.redisplay();
	
}

