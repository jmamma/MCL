/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef EVENTS_H__
#define EVENTS_H__

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_events GUI Events
 *
 * @{
 *
 * \file
 * GUI Event structures and macros
 **/

#include <inttypes.h>
#include "RingBuffer.h"

/** Mask for a button pressed. **/
#define EVENT_BUTTON_PRESSED  _BV(0)
/** Mask for a button released. **/
#define EVENT_BUTTON_RELEASED _BV(1)

/** Tests if button was pressed in the event structure. **/
#define EVENT_PRESSED(event, button) ((event)->mask & EVENT_BUTTON_PRESSED && (event)->source == button)
/** Tests if button was released in the event structure. **/
#define EVENT_RELEASED(event, button) ((event)->mask & EVENT_BUTTON_RELEASED && (event)->source == button)

/**
 * Stores a GUI event. The mask stores what event happened (button
 * released or pressed at the moment), and the source indicates which
 * button is was coming from.
 **/
typedef struct gui_event_s {
  uint8_t mask;
  uint8_t source;
} gui_event_t;

/**
 * Called in the IO polling interrupt routine to interpret the
 * hardware events and store gui_event_t objects in the Event
 * ringbuffer.
 **/
void pollEventGUI();

/**
 * Event ringbuffer storing the events as they happen on the IO.
 **/
extern volatile CRingBuffer<gui_event_t, 8> EventRB;

/** @} **/

/** @} **/

#endif /* EVENTS_H__ */
