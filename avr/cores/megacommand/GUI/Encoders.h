/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ENCODERS_H__
#define ENCODERS_H__

#include "GUI_private.h"
#include "helpers.h"
#include <inttypes.h>

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
class EncoderParent;

/**
 * Prototype for encoder handling functions. These functions are
 * called when the value of an encoder has changed. Examples for
 * encoder_handle_t functions are CCEncoderHandler or TempoEncoderHandle.
 **/
typedef void (*encoder_handle_t)(EncoderParent *enc);

/**
 * \addtogroup gui_encoder_class Encoder
 * @{
 **/

/** Encoder parent class. **/

class EncoderParent {
  /**
   * \addtogroup gui_encoder_class
   * @{
   **/

public:
  /** Old value (before move), current value. **/
  int old, cur;

  /** Counters for encoder pulses **/
  int8_t rot_counter_up = 0;
  int8_t rot_counter_down = 0;
  /** Number of encoder pulses before increasing/decreasing encoder cur value
   * **/
  /** Number of encoder pulses before increasing/decreasing encoder cur value
   * **/
  uint8_t rot_res = 1;

  /** Handling function. **/
  encoder_handle_t handler;

  /** Create a new encoder with short name and handling function. **/
  EncoderParent(encoder_handle_t _handler = NULL);

  void clear();

  /** Should the encoder be displayed again? **/
  bool redisplay;

  /**
   * Handle a modification of the encoder, the default version calls
   * the handling function handler if it is different from NULL.
   **/
  virtual void checkHandle();

  /** Returns true if the encoder value changed. **/
  virtual bool hasChanged();

  /** Return the current value. **/
  virtual int getValue() { return cur; }
  /** Return the old value. **/
  virtual int getOldValue() { return old; }
  /**
   * Set the value of the encoder to value. If handle is true,
   * checkHandle() is called after the modification of the encoder
   * value. This will make setValue() behave as if the user had moved
   * the encoder.
   **/
  virtual void setValue(int value, bool handle = false);

  /**
   * Display the encoder at index i on the screen. The index is a
   * multiple of 4 characters.  This can be overloaded to implement
   * custom and fancy ways of displaying encoders.
   **/
  virtual void displayAt(int i);

#ifdef HOST_MIDIDUINO
  virtual ~Encoder() {}
#endif

  /* @} */
};

class Encoder : public EncoderParent {
  /** Short name. **/
public:
  Encoder(const char *_name = NULL, encoder_handle_t _handler = NULL);
  // }
  //) : public EncoderParent(encoder_handle_t _handler);
  char name[4];
  /**
   * If this variable is set to true, and pressmode to false, an
   * encoder-turn with the encoder pressed down will lead to an
   * increment by 5 times the value (default true).
   *
   * This will work with the parent update() method, not if update()
   * is overloaded.
   **/
  bool fastmode;
  /**
   * If this variable is set to true, turning the encoder while the
   * button is pressed will have no effect on the encoder value.
   *
   * This will work with the parent update() method, not if update()
   * is overloaded.
   **/
  bool pressmode;
  /** Returns the encoder name. **/
  virtual char *getName() { return name; }
  /** Set the encoder name (max 3 characters). **/
  virtual void setName(const char *_name);
  /**
   * Updates the value of an encoder according to the movements of the
   * hardware (recorded in the encoder_t structure). The default
   * handler adds the normal increment, and handles pressing down the
   * encoder according to pressmode and fastmode.
   **/
  int update_rotations(encoder_t *enc);
  virtual int update(encoder_t *enc);
};
/** @} **/

#endif /* ENCODERS_H__ */
