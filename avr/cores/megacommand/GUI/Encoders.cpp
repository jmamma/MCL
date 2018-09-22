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
 * Handle a change in a VarRangeEncoder by setting the variable pointed to by
 *enc->var.
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
void TempoEncoderHandle(Encoder *enc) { MidiClock.setTempo(enc->getValue()); }
#endif

EncoderParent::EncoderParent(encoder_handle_t _handler = NULL) {
  old = 0;
  cur = 0;
  redisplay = false;
  handler = _handler;
}

void EncoderParent::checkHandle() {
  if (cur != old) {
    if (handler != NULL)
      handler(this);
  }

  old = cur;
}

void EncoderParent::setValue(int value, bool handle) {
  if (handle) {
    cur = value;
    checkHandle();
  } else {
    old = cur = value;
  }
  redisplay = true;
}

void EncoderParent::displayAt(int i) {
  GUI.put_value(i, getValue());
  redisplay = false;
}

bool EncoderParent::hasChanged() { return old != cur; }

void EncoderParent::clear() {
  old = 0;
  cur = 0;
}

Encoder::Encoder(const char *_name = NULL, encoder_handle_t _handler = NULL)
    : EncoderParent(_handler) {
  setName(_name);
  fastmode = true;
  pressmode = false;
}

void Encoder::setName(const char *_name) {
  if (_name != NULL)
    m_strncpy_fill(name, _name, 4);
  name[3] = '\0';
}

int Encoder::update(encoder_t *enc) {

  uint8_t amount = abs(enc->normal);
  int inc = 0;

  while (amount > 0) {
    if (enc->normal > 0) {
      rot_counter_up += 1;
      if (rot_counter_up > rot_res) {
        rot_counter_up = 0;
        inc += 1;
      }
      rot_counter_down = 0;
    }
    if (enc->normal < 0) {
      rot_counter_down += 1;
      if (rot_counter_down > rot_res) {
        rot_counter_down = 0;
        inc -= 1;
      }

      rot_counter_up = 0;
    }
    amount--;
  }
  inc = inc + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
  cur += inc;

  return cur;
}

/* EnumEncoder */
void EnumEncoder::displayAt(int i) {
  GUI.put_string_at(i * 4, enumStrings[getValue()]);
  redisplay = false;
}

void PEnumEncoder::displayAt(int i) {
  //  GUI.put_p_string_at_fill(i * 4,
  //  (PGM_P)(pgm_read_word(enumStrings[getValue()])));
  GUI.put_p_string_at(i * 4, (PGM_P)(enumStrings[getValue()]));
  redisplay = false;
}

/* RangeEncoder */

int RangeEncoder::update(encoder_t *enc) {

  uint8_t amount = abs(enc->normal);
  int inc = 0;

  while (amount > 0) {
    if (enc->normal > 0) {
      rot_counter_up += 1;
      if (rot_counter_up > rot_res) {
        rot_counter_up = 0;
        inc += 1;
      }
      rot_counter_down = 0;
    }
    if (enc->normal < 0) {
      rot_counter_down += 1;
      if (rot_counter_down > rot_res) {
        rot_counter_down = 0;
        inc -= 1;
      }

      rot_counter_up = 0;
    }
    amount--;
  }
  inc = inc + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));
  cur = limit_value(cur, inc, min, max);

  return cur;
}

/* CharEncoder */
CharEncoder::CharEncoder() : RangeEncoder(0, 37) {}

char CharEncoder::getChar() {
  uint8_t val = getValue();
  if (val == 0) {
    return ' ';
  }
  if (val < 27)
    return val - 1 + 'A';
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
NotePitchEncoder::NotePitchEncoder(char *_name) : RangeEncoder(0, 127, _name) {}

void NotePitchEncoder::displayAt(int i) {
  char name[5];
  getNotePitch(getValue(), name);
  GUI.put_string_at(i * 4, name);
}

void MidiTrackEncoder::displayAt(int i) { GUI.put_value(i, getValue() + 1); }

void AutoNameCCEncoder::initCCEncoder(uint8_t _channel, uint8_t _cc) {
  CCEncoder::initCCEncoder(_channel, _cc);
  setCCName();
  GUI.redisplay();
}
